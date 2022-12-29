
#include <stdlib.h>
#include "stretchy.h"

void stretchy_init(struct StretchyString *str) {
  str->size = 50;
  str->value = malloc(sizeof(char *) * str->size);
  str->length = 0;
}

void stretchy_add(struct StretchyString *str, char c) {
  if (str->length == str->size - 2) {
    str->size *= 2;
    char *newBuff = malloc(sizeof(char *) * str->size);

    // We copy manually rather than with strcpy because the current path
    // is not null terminated during its construction.
    for (int i = 0; i < str->length; i++) {
      newBuff[i] = str->value[i];
    }
    free(str->value);
    str->value = newBuff;
  }

  str->value[str->length++] = c;
}

char* stretchy_value(struct StretchyString *str) {
  str->value[str->length] = 0;

  return str->value;
}

void stretchy_destroy(struct StretchyString *str) {
  free(str->value);
}
