#ifndef _TOML_H
#define _TOML_H 1

#include <stdio.h>
#include "stretchy.h"

enum ValueType {
  BOOLEAN,
  STRING,
  INTEGER,
  FLOAT,
  DATE
};

enum Status {
  END,
  MORE,
  EEOF,
  EUNEXP
};

struct Value {
  char *key;
  enum ValueType type;
  union {
    struct StretchyString string;
    int integer;
  };
};

typedef void (*cb)(struct Value*);

struct TomlParser {
  FILE *source;
  struct StretchyString currentPath;
  cb callback;
  cb (*objCallback)(char*);
};

struct TomlParser* toml_parser_init(const char *file);

char read_quoted(struct TomlParser *p, struct StretchyString *str);

char read_name(struct TomlParser *p, char c);

char scan_to_end_of_line(struct TomlParser *p, char c);

enum Status read_next(struct TomlParser *p);

void toml_parse(struct TomlParser *p);

#endif /* toml.h */
