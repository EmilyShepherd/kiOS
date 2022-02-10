#include <string.h>
#include <unistd.h>
#include "toml.h"

#define REQUIRE(expected)                                     \
  if (val->type != expected) {                                \
    printf("[ERROR] %s has an unexpected value\n", val->key); \
  }

void callback(struct Value *val) {
  if (strcmp(val->key, "hostname") == 0) {
    REQUIRE(STRING);
    sethostname(val->string.value, val->string.length);
  }

  if (val->type == STRING) {
    printf("\"%s\" has value \"%s\"\n", val->key, val->string.value);
  } else if (val->type == BOOLEAN) {
    if (val->integer) {
      printf("\"%s\" has value true\n", val->key);
    } else {
      printf("\"%s\" has value false\n", val->key);
    }
  } else {
    printf("\"%s\" has unexpected value\n", val->key);
  }
}

cb obj(char *name) {
  printf("OBJECT STARTED: %s\n", name);

  return &callback;
}

int main(int argc, char **argv) {
  char *file = argv[1];

  struct TomlParser *parser = toml_parser_init(file);
  parser->callback = &callback;
  parser->objCallback = &obj;

  toml_parse(parser);

  return 0;
}
