// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <math/dag_Point2.h>


namespace darg
{

class Element;

int get_next_char_size(const char *start_str);
int get_prev_char_size(const char *str, int len);
int get_next_text_pos_u(const wchar_t *str, int len, int pos, bool is_increment, bool is_jump_word, bool ignore_spaces = false);
int wchar_to_symbol(wchar_t wc, char *buffer, int buffer_sz);
int utf_calc_bytes_for_symbols(const char *str, int utf_len, int n_symbols);

int font_mono_width_from_sq(int font_id, int font_ht, const Sqrat::Object &obj);
Point2 calc_text_size(const char *str, int len, int font_id, int spacing, int mono_width, int font_ht);
Point2 calc_text_size_u(const wchar_t *str, int len, int font_id, int spacing, int mono_width, int font_ht);

bool is_text_input(const Element *elem);

} // namespace darg
