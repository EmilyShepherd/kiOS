
#ifndef _H_PARSER
#define _H_PARSER 1

#include <stddef.h>

typedef struct Parser Parser;

typedef size_t (*write_callback)(unsigned char *ptr, size_t, void*);

typedef int (*Op)(unsigned char *ptr, size_t *i, size_t max, Parser *parser);

typedef int (*end_obj_cb)(void *data);

typedef struct StackItem {
  char type;
  Op process_key;
  Op start_obj;
  end_obj_cb end_obj;
} StackItem;

struct Parser {
  Op func;
  Op then;
  int target;
  char str_buffer[500];
  char *str_target;
  size_t str_i;
  int *ptr;
  int *end;
  StackItem stack[10];
  size_t stack_i;
  void *data;
};

#define INC() if (*i == max) { return 1; } else { (*i)++; }
#define EXPECT(c) if (ptr[*i] != c) { printf("(ERR) %c %d %c %d\n", c, c, ptr[*i], ptr[*i]); return -1; } else { INC(); }
#define PARSER_CB(name) int name(unsigned char *ptr, size_t *i, size_t max, Parser *p)
#define DATA_IS(type) type *data = (type*)p->data

PARSER_CB(skip_whitespace);
PARSER_CB(slurp_string);
PARSER_CB(read_int);
PARSER_CB(skip_n);
PARSER_CB(expect_token);

size_t parse_block(unsigned char *ptr, size_t nmemb, Parser *parser);

#endif /* ifndef _H_PARSER */
