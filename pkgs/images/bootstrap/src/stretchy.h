
#ifndef _STRETCHY_H
#define _STRETCHY_H 1

struct StretchyString {
  char *value;
  size_t size;
  size_t length;
};

void stretchy_init(struct StretchyString *str);

void stretchy_add(struct StretchyString *str, char c);

char* stretchy_value(struct StretchyString *str);

void stretchy_destroy(struct StretchyString *str);

#endif /* stretchy.h */
