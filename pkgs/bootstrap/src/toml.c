#include <stdlib.h>
#include "toml.h"

#define IS_ALPHA ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_NUMERIC ((c >= '0' && c <= '9'))
#define IS_NAME_START (IS_ALPHA || c == '"' || c == '\'')
#define IS_NAME_CHAR (IS_ALPHA || IS_NUMERIC || c == '.' || c == '_')
#define IS_WHITESPACE (c == ' ' || c == '\t')
#define IS_NEWLINE (c == '\n' || c == '\r')
#define READ c = (char)fgetc(p->source)
#define SCAN_OVER_CHARS(COND) do { READ; } while(COND)
#define SKIP_WS SCAN_OVER_CHARS(IS_WHITESPACE)
#define EXPECT(v)      \
  if (c == -1) {       \
    return EEOF;       \
  } else if (c != v) { \
    return EUNEXP;     \
  }

#define EOL                                     \
  SKIP_WS;                                      \
  if (c == '#') {                               \
    SCAN_OVER_CHARS(!IS_NEWLINE && c != -1);    \
  } else if (!IS_NEWLINE) {                     \
    return EUNEXP;                              \
  }

const char *CONSTS[] = {
  "true",
  "false",
  "inf",
  "nan",
  NULL
};

struct TomlParser* toml_parser_init(const char *file) {
  struct TomlParser *p = malloc(sizeof(struct TomlParser));
  p->source = fopen(file, "r");
  stretchy_init(&p->currentPath);

  return p;
}

char read_quoted(struct TomlParser *p, struct StretchyString *str) {
  char c;
  READ; // Move past the starting "
  while (c != '"') {
    stretchy_add(str, c);
    READ;
  }

  return c;
}

char read_name(struct TomlParser *p, char c) {
  while (1) {
    if (IS_ALPHA) {
      do {
        stretchy_add(&p->currentPath, c);
        READ;
      } while(IS_NAME_CHAR);
    } else if (c == '"') {
      c = read_quoted(p, &p->currentPath);
      READ; // Move past the final "
    } else {
      // err
    }

    while (IS_WHITESPACE) READ;

    if (c == '.') {
      stretchy_add(&p->currentPath, '.');
      SKIP_WS;
      continue;
    }

    return c;
  }
}

void toml_parse(struct TomlParser *p) {
  while (1) {
    enum Status s = read_next(p);

    if (feof(p->source)) {
      return;
    }

    if (s == MORE) {
      continue;
    } else if (s == EEOF) {
      printf("[ERROR] Unexpected End of File\n");
    } else if (s == EUNEXP) {
      printf("[ERROR] Unexpected Character\n");
    }
  }
}

void parse_number(struct TomlParser *p, char c) {
  int number = 0;
  if (c == '0') {
    READ;
    if (c == 'x') {
      // hex
    } else if (c == 'o') {
      // octal
    } else if (c == 'b') {
      // binary
    } else {
      // decimal
    }
  }
  do {
    if (c != '_') {
      number = number * 10 + (c + '0');
    }
    READ;
  } while (IS_NUMERIC || c == '_');
}

enum Status parse_value(struct TomlParser *p, char c, char *path) {
  struct Value value;
  value.key = path;

  if (c == '"') {
    stretchy_init(&value.string);

    c = read_quoted(p, &value.string);
    EOL;

    value.type = STRING;
    stretchy_value(&value.string);

    (*p->callback)(&value);

    stretchy_destroy(&value.string);
  } else if (IS_NUMERIC) {
    parse_number(p, c);
    EOL;
  } else if (c == '+' || c == '-') {
    // pos / neg numberic or inf
  } else {
    for (int i = 0; CONSTS[i] != NULL; i++) {
      if (c == CONSTS[i][0]) {
        for (int j = 1; CONSTS[i][j] != 0; j++) {
          READ;
          if (c == -1) {
            return EEOF;
          } else if (c != CONSTS[i][j]) {
            return EUNEXP;
          }
        }
        if (CONSTS[i][0] == 't') {
          value.type = BOOLEAN;
          value.integer = 1;
        } else if (CONSTS[i][0] == 'f') {
          value.type = BOOLEAN;
          value.integer = 0;
        }

        EOL;
        (*p->callback)(&value);

        return MORE;
      }
    }
    return EUNEXP;
  }
}

enum Status read_next(struct TomlParser *p) {
  char c;

  SCAN_OVER_CHARS(IS_WHITESPACE || IS_NEWLINE);
  if (c == -1) {
    return END;
  } else if (c == '#') {
    SCAN_OVER_CHARS(!IS_NEWLINE && c != -1);
  } else if (c == '[') {
    READ;
    int isArrayObject = 0;
    if (c == '[') {
      SKIP_WS;
      isArrayObject = 1;
    }

    p->currentPath.length = 0;
    c = read_name(p, c);

    EXPECT(']');

    if (isArrayObject) {
      READ;
      EXPECT(']');
      EOL;
      p->callback = p->objCallback(stretchy_value(&p->currentPath));
      p->currentPath.length = 0;
    } else {
      stretchy_add(&p->currentPath, '.');
      EOL;
    }
  } else if (IS_NAME_START) {
    // Save the current path length so that we can reset it afterwards
    // without losing our place in the table.
    size_t tableP = p->currentPath.length;
    c = read_name(p, c);
    char *path = stretchy_value(&p->currentPath);

    // Reset the length back to the table name.
    p->currentPath.length = tableP;

    EXPECT('=');
    SKIP_WS;

    return parse_value(p, c, path);
  } else {
    return EUNEXP;
  }
}
