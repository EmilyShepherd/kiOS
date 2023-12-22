#include "parser/parser.h"
#include "http/http.h"
#include "http/http2.h"

const char FIRST_DATA[] = {
  HTTP2_CONNECTION_PREFACE,

  HTTP2_SETTINGS_ACK,

  HTTP2_WINDOW_UPDATE,
  0x00, 0x00, 0xff, 0xff,
};

const char *settings_frame = &FIRST_DATA[24];
const char *window_frame = &FIRST_DATA[24 + HTTP2_FRAME_HEADER_SIZE];

void init_for_structure(Parser *p, int target, Op then);
void init_for_frame(Parser *p);

PARSER_CB(process_frame);
PARSER_CB(process_setting);
PARSER_CB(read_structure);

void add_frame(char *buf, char type, int size, char flags, int stream) {
  char *size_b = (char*)&size;
  char *stream_b = (char*)&stream;
  buf[0] = size_b[2];
  buf[1] = size_b[1];
  buf[2] = size_b[0];
  buf[3] = type;
  buf[4] = flags;
  buf[5] = stream_b[3];
  buf[6] = stream_b[2];
  buf[7] = stream_b[1];
  buf[8] = stream_b[0];
}

void ack_settings(conn_t *conn) {
  printf("SENDING ACK\n");
  if (conn->status == STATUS_NEEDS_PREFACE) {
    printf("Sending first data\n");
    conn->status = STATUS_OK;
    wolfSSL_write(conn->ssl_session, FIRST_DATA, sizeof(FIRST_DATA));
  } else {
    printf("Sending ACK\n");
    wolfSSL_write(conn->ssl_session, settings_frame, HTTP2_FRAME_HEADER_SIZE);
  }
}

void update_window(conn_t *conn) {
  char data[HTTP2_FRAME_HEADER_SIZE + 4];
  add_frame(data, HTTP2_FRAME_TYPE_WINDOW_UPDATE, 4, 0, 0);
  wolfSSL_write(conn->ssl_session, data, sizeof(data));
}

char FRAME_H_TRANSFORM[] = {
  frameb(length, 2), frameb(length, 1), frameb(length, 0),
  offsetof(frame_header_t, type),
  offsetof(frame_header_t, flags),
  t_ntoh32(frame_header_t, stream_id)
};

char SETTING_TRANSFORM[] = {
  t_ntoh16(setting_t, type),
  t_ntoh32(setting_t, value)
};

char WINDOW_TRANSFORM[] = { t_ntoh32(setting_t, value) };

PARSER_CB(read_structure) {
  int target = p->target;
  int remaining = max - *i + 1;
  if (target > remaining) {
    target = remaining;
  }

  char *transform;
  switch (p->target) {
    case 4:
      transform = WINDOW_TRANSFORM;
      break;
    case 6:
      transform = SETTING_TRANSFORM;
      break;
    case 9:
      transform = FRAME_H_TRANSFORM;
      break;
  }

  do {
    p->str_target[transform[p->str_i]] = ptr[(*i)++];
  } while (++(p->str_i) < target);

  p->func = p->then;
  return 0;
}

PARSER_CB(process_setting) {
  DATA_IS(conn_t);
  
  printf("  Setting: [ Type: 0x%04x, Data: %d ]\n", data->last_setting.type, data->last_setting.value);

  if (!(data->last_frame.length -= 6)) {
    ack_settings(data);
    init_for_frame(p);
  } else {
    p->str_i = 0;
    p->func = &read_structure;
  }

  return *i > max;
}

PARSER_CB(process_window_update) {
  DATA_IS(conn_t);

  printf("  Window Update (0x%08x): %d\n", data->last_frame.stream_id, data->last_setting.value);

  init_for_frame(p);

  return *i > max;
}

PARSER_CB(goaway) {
  while (1) {
    printf("%c\n", ptr[*i]);
    INC();
  }

  return 0;
}

PARSER_CB(process_frame) {
  DATA_IS(conn_t);
  frame_header_t *f = &(data->last_frame);

  printf("Frame 0x%02x (length: %d): Flags: %02x, Stream ID: 0x%08x\n", f->type, f->length, f->flags, f->stream_id);

  switch (f->type) {
    case HTTP2_FRAME_TYPE_DATA:
      //
      break;
    case HTTP2_FRAME_TYPE_SETTINGS:
      p->str_target = (char *)&(data->last_setting);
      init_for_structure(p, 6, &process_setting);
      break;
    case HTTP2_FRAME_TYPE_WINDOW_UPDATE:
      p->str_target = (char *)&(data->last_setting);
      init_for_structure(p, 4, &process_window_update);
      break;
    case HTTP2_FRAME_TYPE_GOAWAY:
      p->func = &goaway;
      break;
    default:
      printf("Unknown\n");
  }

  return *i > max;
}

void init_for_structure(Parser *p, int target, Op then) {
  p->str_i = 0;
  p->target = target;
  p->func = &read_structure;
  p->then = then;
}

void init_for_frame(Parser *p) {
  init_for_structure(p, 9, &process_frame);
  p->str_target = (char *)&(((conn_t*)p->data)->last_frame);
}

void open_http2(conn_t *conn) {
  memset(&conn->p, 0, sizeof(Parser));
  conn->p.data = conn;
  init_for_frame(&(conn->p));

  conn->status = STATUS_NEEDS_PREFACE;
}
 
