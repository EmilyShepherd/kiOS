#include <string.h>
#include <stdio.h>
#include "parser/parser.h"

/**
 * Increments the buffer while the head is a whitespace character.
 *
 * Once it encounters a non-whitespace character, it leaves it at the
 * head and sets the state to the next_state.
 */
PARSER_CB(skip_whitespace) {
  while (ptr[*i] == ' ' || ptr[*i] == '\n' || ptr[*i] == '\t') {
    INC();
  }

  p->func = p->then;
  return 0;
}

/**
 * Reads a string into the str_target pointer until it comes across a
 * closing quote. Callers should set str_target to the desired location.
 *
 * Once it encouters a quote, it pops it and sets the state to
 * skip_whitespace.
 */
PARSER_CB(slurp_string) {
  while (ptr[*i] != '"') {
    p->str_target[p->str_i++] = ptr[*i];
    INC();
  }

  p->str_target[p->str_i] = 0;
  p->str_i = 0;
  p->func = &skip_whitespace;
  INC();
  return 0;
}

/**
 * Reads an integer into the target until it comes across a non digit
 * character.
 *
 * Once it encouters a non digit, it leaves it at the head and sets the
 * state to skip_whitespace.
 */
PARSER_CB(read_int) {
  while (48 <= ptr[*i] && ptr[*i] <= 57) {
    p->target *= 10;
    p->target += ptr[*i] - 48;
    INC();
  }

  p->func = &skip_whitespace;
  return 0;
}

/**
 * Indescriminately skips the target number of characters, then updates
 * state to next_state.
 */
PARSER_CB(skip_n) {
  int left = max - *i + 1;
  int toskip = p->target - p->str_i;

  if (left < toskip) {
    p->target -= left;
    return 1;
  } else if (left == toskip) {
    p->func = p->then;
    return 1;
  } else {
    p->func = p->then;
    *i += toskip;

    return 0;
  }
}

PARSER_CB(expect_token) {
  while (p->str_buffer[p->str_i]) {
    if (ptr[*i] != p->str_buffer[p->str_i++]) {
      return -1;
    }
    INC();
  }

  p->func = p->then;
  return 0;
}


/**
 * A push based event based block parser
 */
size_t parse_block(char *ptr, size_t nmemb, Parser *parser) {
  size_t i = 0;
  size_t max = nmemb - 1;

  while (1) {
    int ret = parser->func(ptr, &i, max, parser);

    if (ret < 0) {
      return 0;
    } else if (ret > 0) {
      return nmemb;
    }
  }

  return nmemb;
}
