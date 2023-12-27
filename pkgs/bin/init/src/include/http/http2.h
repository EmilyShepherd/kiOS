
#ifndef _H_HTTP2
#define _H_HTTP2 1

#define HTTP2_FRAME_TYPE_DATA 0x00
#define HTTP2_FRAME_TYPE_HEADERS 0x01
#define HTTP2_FRAME_TYPE_RST_STREAM 0x03
#define HTTP2_FRAME_TYPE_SETTINGS 0x04
#define HTTP2_FRAME_TYPE_PUSH_PROMISE 0x05
#define HTTP2_FRAME_TYPE_PING 0x06
#define HTTP2_FRAME_TYPE_GOAWAY 0x07
#define HTTP2_FRAME_TYPE_WINDOW_UPDATE 0x08
#define HTTP2_FRAME_TYPE_CONTINUATION 0x09

#define HTTP2_FLAG_END_HEADERS 0x04
#define HTTP2_FLAG_ACK 0x01
#define HTTP2_FLAG_NONE 0x00

#define HTTP2_FRAME_HEADER_SIZE 9

#define HTTP2_INDEX_AUTHORITY 1
#define HTTP2_INDEX_METHOD_GET 2
#define HTTP2_INDEX_METHOD_POST 3
#define HTTP2_INDEX_PATH 4
#define HTTP2_INDEX_SCHEME_HTTP 6
#define HTTP2_INDEX_SCHEME_HTTPS 7
#define HTTP2_INDEX_ACCEPT 19

#define HTTP2_HEADER_INDEXED 0x80
#define HTTP2_HEADER_ADD_INDEX 0x40

typedef struct setting {
  short type;
  int value;
} setting_t;

typedef struct frame_header {
  int length;
  char type;
  char flags;
  int stream_id;
} frame_header_t;

typedef struct http2_frame {
  char size[3];
  char type;
  char flags;
  char stream_id[4];
  char data[];
} http2_frame_t;

typedef char index_t;

#define frameb(member, n) offsetof(frame_header_t, member) + n
#define t_ntoh16(s, m) offsetof(s, m) + 1, offsetof(s, m)
#define t_ntoh32(s, m) \
  offsetof(s, m) + 3, \
  offsetof(s, m) + 2, \
  offsetof(s, m) + 1, \
  offsetof(s, m)

#define HTTP2_SYSTEM_STREAM 0x00, 0x00, 0x00, 0x00

#define HTTP2_CONNECTION_PREFACE \
  /* PRI *     */ 0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, \
  /* HTTP/2.0  */ 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30, \
  /* CRLF CRLF */ 0x0d, 0x0a, 0x0d, 0x0a, \
  /* SM        */ 0x53, 0x4d, \
  /* CRLF CRLF */ 0x0d, 0x0a, 0x0d, 0x0a

#define HTTP2_SETTINGS_ACK \
  0, 0, 0, \
  HTTP2_FRAME_TYPE_SETTINGS, \
  HTTP2_FLAG_ACK, \
  HTTP2_SYSTEM_STREAM

#define HTTP2_WINDOW_UPDATE \
  0, 0, 4, \
  HTTP2_FRAME_TYPE_WINDOW_UPDATE, \
  HTTP2_FLAG_NONE, \
  HTTP2_SYSTEM_STREAM

#endif /* ifndef _H_HTTP2 */
