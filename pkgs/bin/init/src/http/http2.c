#include "parser/parser.h"
#include "http/http.h"
#include "http/http2.h"

PARSER_CB(read_hpack_index);

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
  if (conn->status != STATUS_NEEDS_PREFACE) {
    wolfSSL_write(conn->ssl_session, settings_frame, HTTP2_FRAME_HEADER_SIZE);
  } else {
    mark_available(conn);
    wolfSSL_write(conn->ssl_session, FIRST_DATA, sizeof(FIRST_DATA));
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

PARSER_CB(read_vint) {
  while (ptr[*i] & 0x80) {
    p->target += (ptr[*i] & ~0x80) << (8 * p->str_i++);
    INC();
  }
    
  p->func = p->then;

  return 0;
}

PARSER_CB(inflate_header);
PARSER_CB(read_slen) {
  p->target = ptr[*i] & ~0x80;

  printf("Length: %d\n", p->target);
  
  if (ptr[*i] & 0x80) {
    printf("(HUFFMAN) ");
  }

  p->func = p->then;

  INC();
  return 0;
}

PARSER_CB(read_status) {
  DATA_IS(conn_t);

  while (p->str_i++ < p->target) {
    data->status *= 10;
    data->status += ptr[*i] - 48;
    INC();
  }

  printf("Status: %d\n", data->status);
  p->func = &read_hpack_index;

  return 0;
}

PARSER_CB(skip_header_value) {
  p->str_i = 0;
  p->func = &skip_n;
  p->then = &read_hpack_index;

  return 0;
}

PARSER_CB(find_header) {
  DATA_IS(conn_t);
  printf("\n[Indexed Header %x]: ", p->target);

  p->func = &read_slen;
  p->str_i = 0;

  switch (p->target) {
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      p->then = &read_status;
      data->status = 0;
      break;
    default:
      p->then = &skip_header_value;
  }

  return 0;
}

PARSER_CB(skip_header_name) {
  printf("Skipping\n");
  p->str_i = 0;
  p->func = &skip_n;
  p->then = &find_header;

  return 0;
}

PARSER_CB(read_hpack_index) {
  unsigned char c = ptr[*i];
  unsigned char mask;

  printf("\nREADING: %02x\n", c);

  if (c & HTTP2_HEADER_INDEXED) {
    mask = ~0x80;
  } else if (c & HTTP2_HEADER_ADD_INDEX) {
    mask = ~0xc0;
  }

  p->target = c & mask;

  if (!p->target) {
    printf("Skipping unknown header\n");
    p->func = &read_slen;
    p->then = &skip_header_name;
  } else if (p->target == mask) {
    p->func = &read_vint;
    p->then = &find_header;
  } else {
    p->func = &find_header;
  }

  INC();
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
    case HTTP2_FRAME_TYPE_HEADERS:
      for (int j = 0; j < max - *i;) {
        printf("%02x ", ptr[j + *i], ptr[j + *i]);
        j++;
        if (j % 8 == 0) {
          printf("\n");
        } else if (j % 4 == 0) {
          printf("\t");
        }
      }
      printf("\n");

      p->func = &read_hpack_index;

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
 
void do_http2_req(conn_t *conn, req_t *req) {
  unsigned char buf[2000];

  int i = 9;
  buf[i + 0] = HTTP2_HEADER_ADD_INDEX | HTTP2_INDEX_AUTHORITY;
  buf[i + 1] = strlen(conn->host->name);
  strcpy(&buf[i + 2], conn->host->name);

  i += 2 + buf[i + 1];
  buf[i + 0] = HTTP2_HEADER_INDEXED | HTTP2_INDEX_METHOD_GET;
  buf[i + 1] = HTTP2_HEADER_INDEXED | HTTP2_INDEX_SCHEME_HTTPS;
  buf[i + 2] = HTTP2_HEADER_ADD_INDEX | HTTP2_INDEX_PATH;
  buf[i + 3] = strlen(req->path);
  strcpy(&buf[i + 4], req->path);

  i += 4 + buf[i + 3];

  printf("LEN: %d\n", i);

  add_frame(buf, HTTP2_FRAME_TYPE_HEADERS, i - 9, HTTP2_FLAG_END_HEADERS, conn->host->next_stream_id);
  conn->host->next_stream_id += 2;

  wolfSSL_write(conn->ssl_session, buf, i);

  for (int j = 0; j < i;) {
    printf("%x (%c) ", buf[j], buf[j]);
    j++;
    if (j % 8 == 0) {
      printf("\n");
    } else if (j % 4 == 0) {
      printf("\t");
    }
  }
  printf("\n");
}

typedef enum {
  /* FSA accepts this state as the end of huffman encoding
     sequence. */
  NGHTTP2_HUFF_ACCEPTED = 1 << 14,
  /* This state emits symbol */
  NGHTTP2_HUFF_SYM = 1 << 15,
} nghttp2_huff_decode_flag;

typedef struct {
  /* fstate is the current huffman decoding state, which is actually
     the node ID of internal huffman tree with
     nghttp2_huff_decode_flag OR-ed.  We have 257 leaf nodes, but they
     are identical to root node other than emitting a symbol, so we
     have 256 internal nodes [1..255], inclusive.  The node ID 256 is
     a special node and it is a terminal state that means decoding
     failed. */
  uint16_t fstate;
  /* symbol if NGHTTP2_HUFF_SYM flag set */
  uint8_t sym;
} nghttp2_huff_decode;

typedef nghttp2_huff_decode huff_decode_table_type[16];

#include "huffman_data.h"

/**
 * Decompresses input from the buffer
 *
 * Code adapted from nghttp2.
 */
PARSER_CB(inflate_header) {
  nghttp2_huff_decode node = {p->str_i, 0};
  const nghttp2_huff_decode *t = &node;
  uint8_t c;

  size_t target = *i + p->target - 1;
  if (target > max) {
    target = max;
  }

  do {
    c = ptr[*i];
    t = &huff_decode_table[t->fstate & 0x1ff][c >> 4];
    if (t->fstate & NGHTTP2_HUFF_SYM) {
      //*p->str_target++ = t->sym;
      printf("%c", t->sym);
    }

    t = &huff_decode_table[t->fstate & 0x1ff][c & 0xf];
    if (t->fstate & NGHTTP2_HUFF_SYM) {
      //*p->str_target++ = t->sym;
      printf("%c", t->sym);
    }
  } while (++(*i) <= target);


  printf("\n");

  p->func = &read_hpack_index;

  return 0;
}

