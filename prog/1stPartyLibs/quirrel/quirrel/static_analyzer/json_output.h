#pragma once

#include <string>
#include "quirrel_lexer.h"
#include "quirrel_parser.h"

bool tokens_to_json(const char * file_name, Lexer & lexer);
bool ast_to_json(const char * file_name, Node * node, Lexer & lexer);
bool compiler_messages_to_json(const char * file_name);
bool json_write_files();
