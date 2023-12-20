
#ifndef _H_IMAGE
#define _H_IMAGE 1

#include "parser/parser.h"
#include "gzip.h"
#include "tar.h"
#include "http/http.h"

typedef struct ImageDownload {
  char desired[80];
  char digest[80];
  char digests[10][80];
  size_t layers;
  size_t completed_layers;
  int match;
  host_t *host;
  char name[IMAGE_NAME_MAX];
  Parser p;
} ImageDownload;

struct Download {
  int rawFd;
  Parser gzip;
  Parser tar;
  Tar tar_info;
  GzipRequest gzip_info;
};

ImageDownload* download_image(char *registry, char *image, char *tag);

#endif /* ifndef _H_IMAGE */
