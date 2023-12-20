
#ifndef _H_GZIP
#define _H_GZIP 1

#include <string.h>
#include <zlib.h>

#include "parser/parser.h"

#define FLG_FTEXT 0x01
#define FLG_FHCRC 0x02
#define FLG_FEXTRA 0x04
#define FLG_FNAME 0x08
#define FLG_FCOMMENT 0x0F

#define IS_SET(n) p->str_buffer[0] & n
#define UNSET_FLAG(n) p->str_buffer[0] &= ~n

#define CHUNK 16384

typedef struct GzipRequest {
  write_callback cb;
  z_stream z;
  void* data;
} GzipRequest;

void init_gzip_parser(Parser*, GzipRequest*);

#endif /* ifndef _H_GZIP */
