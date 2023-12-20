
#ifndef _H_JSON_GENERIC
#define _H_JSON_GENERIC 1

#include "parser.h"

PARSER_CB(skip_to_eol);
PARSER_CB(read_int);
PARSER_CB(mid_list);
PARSER_CB(unknown_key);
PARSER_CB(start_key);
PARSER_CB(finish_key);
PARSER_CB(finish_line);
PARSER_CB(start_document);

void init_json_parser(Parser *p, void *data);

#endif
