#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "parser/json_generic.h"
#include "image/image.h"

static PARSER_CB(keys);

int end_index(ImageDownload *index) {
  if (index->match) {
    printf("MATCHED DIGEST: %s\n", index->digest);

    new_request(index->host, index->name, "manifest", index->digest, (write_callback)&parse_block, &index->p);

    // Clear the parser so we can reuse it for the next request.
    init_json_parser(&(index->p), index);
    index->p.stack[0].process_key = &keys;

    return 10;
  }

  return 0;
}

static PARSER_CB(start_manifest) {
  ImageDownload *index = p->data;
  index->digest[0] = 0;
  index->match = 1;

  p->func = &start_document;

  return 0;
}

/**
 * Checks that the str_buffer matches the expected
 */
static PARSER_CB(check_str) {
  if (strcmp(p->str_buffer, ((ImageDownload*)p->data)->desired) != 0) {
    ((ImageDownload*)p->data)->match = 0;
  }
  p->func = &finish_line;
  return 0;
}

static PARSER_CB(platform_keys) {
  p->func = &unknown_key;
  if (strcmp(p->str_buffer, "os") == 0) {
    p->func = &slurp_string;
    p->str_target = p->str_buffer;
    p->then = &check_str;
    strcpy(((ImageDownload*)p->data)->desired, "linux");
    EXPECT('"');
  } else if (strcmp(p->str_buffer, "architecture") == 0) {
    p->func = &slurp_string;
    p->str_target = p->str_buffer;
    p->then = &check_str;
    strcpy(((ImageDownload*)p->data)->desired, "amd64");
    EXPECT('"');
  } 
  return 0;
}

static PARSER_CB(manifest_keys) {
  if (strcmp(p->str_buffer, "mediaType") == 0) {
    //p->func = &slurp_string;
    //p->str_target = p->str_buffer;
    //p->then = &check_media;
    //EXPECT('"');
    p->func = &unknown_key;
  } else if (strcmp(p->str_buffer, "digest") == 0) {
    p->func = &slurp_string;
    p->str_target = ((ImageDownload*)p->data)->digest;
    p->then = &finish_line;
    EXPECT('"');
  } else if (strcmp(p->str_buffer, "platform") == 0) {
    p->func = &skip_whitespace;
    p->then = &start_document;
    p->stack_i++;
    p->stack[p->stack_i].type = '{';
    p->stack[p->stack_i].process_key = platform_keys;
  } else {
    p->func = &unknown_key;
  }

  return 0;
}

static PARSER_CB(manifest_list_keys) {
  if (strcmp(p->str_buffer, "manifests") == 0) {
    p->func = &skip_whitespace;
    p->then = &start_manifest;
    p->stack_i++;
    p->stack[p->stack_i].type = '[';
    p->stack[p->stack_i].process_key = &manifest_keys;
    p->stack[p->stack_i].start_obj = &start_manifest;
    p->stack[p->stack_i].end_obj = (end_obj_cb)&end_index;

    EXPECT('[');
  } else {
    p->func = &unknown_key;
  }

  return 0;
}

static PARSER_CB(layer_keys) {
  if (strcmp(p->str_buffer, "digest") == 0) {
    ImageDownload *image = (ImageDownload*)p->data;

    p->func = &slurp_string;
    p->str_target = image->digests[image->layers];
    p->then = &finish_line;

    EXPECT('"');
  //} else if (strcmp(p->str_buffer, "mediaType") == 0) {
  //  p->func = &slurp_string;
  //  p->str_target = p->str_buffer;
  //  p->then = &check_media;

  //  EXPECT('"');
  } else {
    p->func = &unknown_key;
  }

  return 0;
}

size_t write_download(char *ptr, size_t nmemb, struct Download *d) {
  write(d->rawFd, ptr, nmemb);

  int ret = parse_block(ptr, nmemb, &d->gzip);

  if (ret == 0) {
    close(d->rawFd);
    close(d->tar_info.dirfd);
    free(d);
    printf("LAYER COMPLETE\n");
  }

  return nmemb;
}

size_t write_unzipped(unsigned char *buf, size_t size, struct Download *d) {
  return parse_block(buf, size, &d->tar);
}

int end_layer(ImageDownload *data) {
  char *digest = data->digests[data->layers++];
  printf("Download Layer: %s\n", digest);

  struct Download *d = malloc(sizeof(struct Download));

  new_request(data->host, data->name, "blob", digest, (write_callback)&write_download, d);

  init_gzip_parser(&d->gzip, &d->gzip_info);
  init_tar_parser(&d->tar, &d->tar_info);
  d->gzip_info.cb = (write_callback)&write_unzipped;
  d->gzip_info.data = d;

  char PATH[500];
  sprintf(PATH, "/tmp/%s", digest);
  d->rawFd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

  sprintf(PATH, "/tmp/%s.out", digest);
  mkdir(PATH, S_IRUSR | S_IWUSR | S_IXUSR);
  d->tar_info.dirfd = open(PATH, O_DIRECTORY);

  return 0;
}

PARSER_CB(keys) {
  if (strcmp(p->str_buffer, "layers") == 0) {
    p->func = &skip_whitespace;
    p->then = &start_document;
    p->stack_i++;
    p->stack[p->stack_i].type = '[';
    p->stack[p->stack_i].process_key = &layer_keys;
    p->stack[p->stack_i].end_obj = (end_obj_cb)&end_layer;

    EXPECT('[');
  } else {
    p->func = &unknown_key;
  }

  return 0;
}

ImageDownload* download_image(char *registry, char *image, char *tag) {
  ImageDownload *i = malloc(sizeof(ImageDownload));

  init_json_parser(&(i->p), i);
  i->p.stack[0].process_key = &manifest_list_keys;
  i->layers = 0;

  i->host = new_host(registry);
  strcpy(i->name, image);
  new_request(i->host, i->name, "manifest", tag, (write_callback)&parse_block, &(i->p));

  return i;
}
