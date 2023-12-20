#include <string.h>
#include <stdio.h>
#include "parser/json_generic.h"

/**
 * State in the middle of an array. Only valid heads are a closing list
 * character ("]") or a next item character ",".
 *
 * "]" -> pops the stack. If we were in a second list, we set next_state
 * to mid_list again. If we weren't next_state is finish_line.
 *
 * "," -> expects a new object
 *
 * TODO List of non-objects.
 */
PARSER_CB(mid_list) {
  p->func = &skip_whitespace;

  if (ptr[*i] == ']') {
    p->stack_i--;
    if (p->stack[p->stack_i].type == '[') {
      p->then = &mid_list;
    } else {
      p->then = &finish_line;
    }
  } else if (ptr[*i] == ',') {
    if (p->stack[p->stack_i].start_obj) {
      p->then = p->stack[p->stack_i].start_obj;
    } else {
      p->then = &start_document;
    }
  } else {
    // err
  }

  INC();
  return 0;
}

/**
 * State when a field as been completely finished within an object. We
 * expect either a new field "," or the object to be closed ",".
 *
 * "," -> next_state is set to start_key
 *
 * "}" -> If an end_obj callback is defined, this is called. If this
 * returns non-zero, parsing is immediately halted.
 * Otherwise, if we are in a list, next_state is mid_list. Otherwise we
 * pop the stack and move to finish_line.
 */
PARSER_CB(finish_line) {
  p->func = &skip_whitespace;
  if (ptr[*i] == ',') {
    p->then = &start_key;
  } else if (ptr[*i] == '}') {
    if (p->stack[p->stack_i].end_obj) {
      if (p->stack[p->stack_i].end_obj(p->data)) {
        return -1;
      }
    }

    if (p->stack[p->stack_i].type == '{') {
      p->stack_i--;
      p->then = &finish_line;
    } else {
      p->then = &mid_list;
    }
  } else {
    // err
  }

  INC();
  return 0;
}

/**
 * Processes generic keys by interpreting their type
 *
 * Throws away the value.
 */
PARSER_CB(unknown_key) {
  //printf("Unknown key: %s. Ignoring...\n", p->str_buffer);
  p->then = &finish_line;

  if (ptr[*i] == '"') {
    p->func = &slurp_string;
    p->str_target = p->str_buffer;

    INC();
  } else if (48 <= ptr[*i] && ptr[*i] <= 57) {
    p->func = &read_int;
  } else if (ptr[*i] == '{') {
    p->func = &skip_whitespace;
    p->then = &start_document;
    p->stack_i++;
    p->stack[p->stack_i].type = '{';
    p->stack[p->stack_i].process_key = &unknown_key;
  }

  return 0;
}

/**
 * Called when the key of a field has been finished read. next_state is
 * set to the current stack's process_key cb.
 *
 * Expects the next character to be ":".
 */
PARSER_CB(finish_key) {
  p->func = &skip_whitespace;
  p->then = p->stack[p->stack_i].process_key;

  EXPECT(':');
  return 0;
}

/**
 * Starts reading a field.
 *
 * Expects the next character to be '"'.
 */
PARSER_CB(start_key) {
  p->func = &slurp_string;
  p->str_target = p->str_buffer;
  p->then = &finish_key;

  EXPECT('"');
  return 0;
}

/**
 * Starts reading an object
 *
 * Expects the next characteer to be '{'
 */
PARSER_CB(start_document) {
  p->func = &skip_whitespace;
  p->then = &start_key;

  EXPECT('{');
  return 0;
}

PARSER_CB(skip_over_string) {
  while (ptr[*i] != '"') {
    INC();
  }

  p->func = &skip_to_eol;
  return 0;
}

PARSER_CB(skip_to_eol) {
  while (ptr[*i] != ',') {
    if (ptr[*i] == '"') {
      p->func = &skip_over_string;
      INC();
      return 0;
    }

    INC();
  }

  return 0;
}

PARSER_CB(expect_key) {
  while (1) {
    if (ptr[*i] == p->str_buffer[p->str_i]) {
      INC();
    } else if (ptr[*i] == '"') {
      if (p->str_buffer[p->str_i] == 0) {
        p->func = &skip_whitespace;
        p->then = &finish_key;
      } else {
        p->func = &skip_to_eol;
      }
      return 0;
    } else {
      p->func = &skip_over_string;
      return 0;
    }
  }
}


void init_json_parser(Parser *p, void *data) {
  memset(p, 0, sizeof(*p));

  p->func = &skip_whitespace;
  p->then = &start_document;
  p->data = data;
  p->stack[0].type = '{';
}
