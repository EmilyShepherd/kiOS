#include "image/gzip.h"

static PARSER_CB(expect_magic);
static PARSER_CB(decide_header);
static PARSER_CB(read_xlen);
static PARSER_CB(skip_to_null);
static PARSER_CB(do_inflate);

/**
 * Expects the given magic string then moves onto decide_header
 */
static PARSER_CB(expect_magic) {
  while (p->str_i < p->target) {
    if (ptr[*i] != p->str_buffer[p->str_i++]) {
      return -1;
    }
    INC();
  }

  p->str_buffer[0] = ptr[*i];
  p->func = &skip_n;
  p->then = &decide_header;
  p->target = 6;
  p->str_i = 0;

  INC();
  return 0;
}

/**
 * Decides the next state depending on the header we are at
 */
static PARSER_CB(decide_header) {
  if (IS_SET(FLG_FEXTRA)) {
    UNSET_FLAG(FLG_FEXTRA);
    p->func = &read_xlen;
    p->then = &skip_n;
    p->target = 0;
  } else if (IS_SET(FLG_FNAME) || IS_SET(FLG_FCOMMENT)) {
    UNSET_FLAG(FLG_FNAME); // Doesn't matter if we leave FCOMMENT set
                           // as we only use the flag to check if we
                           // need to run twice.
    p->func = &skip_to_null;
  } else if (IS_SET(FLG_FHCRC)) {
    p->func = &read_xlen;
    p->then = &do_inflate;
  } else {
    p->func = &do_inflate;
  }

  return 0;
}

/**
 * Reads a gzip XLEN - which is two bytes.
 */
static PARSER_CB(read_xlen) {
  if (!p->target) {
    p->target = ptr[*i];
    INC();
  }

  p->target += ptr[*i] << 8;
  p->func = &skip_n;
  p->then = &decide_header;

  INC();
  return 0;
}

/**
 * Reads until we encouter a null byte and then returns to decide_header
 */
static PARSER_CB(skip_to_null) {
  while(ptr[*i]) {
    INC();
  }

  p->func = &decide_header;
  return 0;
}

/**
 * Passes the data to zlib's inflate
 */
static PARSER_CB(do_inflate) {
  GzipRequest *g = (GzipRequest*)p->data;

  int ret;

  g->z.next_in = &ptr[*i];
  g->z.avail_in = max - *i + 1;

  unsigned char out[CHUNK];

  do {
    g->z.avail_out = CHUNK;
    g->z.next_out = out;

    ret = inflate(&g->z, Z_NO_FLUSH);

    int have = CHUNK - g->z.avail_out;

    if (have) {
      if (!g->cb(out, have, g->data)) {
        ret = Z_STREAM_END;
        break;
      }
    }
  } while (g->z.avail_out == 0);

  if (ret != Z_STREAM_END) {
    return 1;
  } else {
    inflateEnd(&g->z);
    return -1;
  }
}

void init_gzip_parser(Parser *p, GzipRequest* g) {
  memset(p, 0, sizeof(*p));
  p->data = g;

  g->z.zalloc = Z_NULL;
  g->z.zfree = Z_NULL;
  g->z.opaque = Z_NULL;
  inflateInit2(&g->z, -MAX_WBITS);

  p->func = &expect_magic;
  p->target = 3;
  p->str_buffer[0] = 0x1f;
  p->str_buffer[1] = 0x8b;
  p->str_buffer[2] = 0x08;
}
