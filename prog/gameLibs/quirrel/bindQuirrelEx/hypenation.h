#pragma once

#include <util/dag_simpleString.h>

SimpleString process_chinese_string(const char *str, wchar_t sep_char = '\t');
SimpleString process_japanese_string(const char *str, wchar_t sep_char = '\t');
