#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "image/tar.h"

#define membersize(s, i) sizeof(((s*)0)->i)
#define sizeto(s, i) offsetof(s, i) + membersize(s, i)

static PARSER_CB(read_octal);
static PARSER_CB(save_data);
static PARSER_CB(start_save_data);
static PARSER_CB(start_header);

#define oct(t) fromoctal(t, sizeof(t) - 1)

static int fromoctal(char* str, int bytes) {
  int ret = 0;

  for (int i = 0; i < bytes; i++) {
    ret = ret * 8 + str[i] - 48;
  }

  return ret;
}

static PARSER_CB(save_data) {
  DATA_IS(Tar);

  int left = data->size - p->str_i;

  // If we are at the end of the file data, just skip to the end of the
  // 512-byte block.
  if (left == 0) {
    close(data->fd);

    int blocks = data->size / 512;
    if (blocks * 512 < data->size) {
      blocks++;
    }

    p->target = blocks * 512;
    p->func = &skip_n;
    p->then = &start_header;

    return 0;
  }

  int len = max - *i + 1;

  if (left < len) {
    len = left;
  }
  write(data->fd, &ptr[*i], len);

  p->str_i += len;

  *i += len;
  return *i > max ? 1 : 0;
}

static PARSER_CB(start_save_data) {
  p->str_i = 0;
  p->func = &save_data;

  return 0;
}

static void copy(char *to, char *ptr, size_t bytes) {
  for (int i = 0; i < bytes; i++) {
    to[i] = ptr[i];
  }
}

static PARSER_CB(start_header) {
  DATA_IS(Tar);

  int in_buffer = p->str_target - p->str_buffer;
  int remaining = max - *i + 1;
  int want = sizeto(tar_header_t, linkname) - in_buffer;

  tar_header_t *header = (tar_header_t*)(in_buffer ? p->str_buffer : &ptr[*i]);

  // If the first byte of the block is null, then there is no name and
  // we assume it is an empty trailer at the end of the stream. Exit
  // immediately so that gzip and http can start skipping bytes.
  if (!header->name[0]) {
    printf("DEAD BLOCK. Leaving early\n");
    return -1;
  } else if (want > remaining) {
    int smallerWant = want - membersize(tar_header_t, linkname);
    if (header->typeflag == '2' || smallerWant > remaining) {
      copy(p->str_target, &ptr[*i], remaining);
      p->str_target += remaining;
      p->func = &start_header;
      return 1;
    }
  }

  p->str_i = 0;
  p->target = 512;
  p->func = &skip_n;
  p->then = &start_header;

  // If there is data in the buffer we need to copy over what we have
  // in the stream into it to recreate a complete header block.
  if (in_buffer) {
    copy(p->str_target, &ptr[*i], want);
    p->str_target = p->str_buffer;
    p->target -= in_buffer;
  }

  switch (header->typeflag) {
    case REGTYPE:
    case AREGTYPE:
    case XHDTYPE:
    case XGLTYPE:
      data->fd = openat(data->dirfd, header->name, O_CREAT|O_WRONLY|O_TRUNC, oct(header->mode));
      data->size = oct(header->size);
      p->then = &start_save_data;
      break;
    case DIRTYPE:
      mkdirat(data->dirfd, header->name, oct(header->mode));
      break;
    case SYMTYPE:
      symlinkat(header->linkname, data->dirfd, header->name);
      break;
    default:
      printf("ERRR: %s\n", header->name);
      return -1;
  }

  return 0;
}

void init_tar_parser(Parser *p, Tar *tar) {
  memset(p, 0, sizeof(*p));
  memset(tar, 0, sizeof(*tar));
  p->data = tar;

  p->func = &start_header;
  p->str_target = p->str_buffer;
}
