// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shlexterm.h"
#include "commonUtils.h"

#include <util/dag_safeArg.h>

#define DSA_OVERLOADS_PARAM_DECL Parser &parser, const Symbol *s,
#define DSA_OVERLOADS_PARAM_PASS parser, s,
DECLARE_DSA_OVERLOADS_FAMILY(inline void report_error, void report_error, report_error);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL Parser &parser, const Symbol &s,
#define DSA_OVERLOADS_PARAM_PASS parser, s,
DECLARE_DSA_OVERLOADS_FAMILY(inline void report_warning, void report_warning, report_warning);
DECLARE_DSA_OVERLOADS_FAMILY(inline void report_message, void report_message, report_message);
DECLARE_DSA_OVERLOADS_FAMILY(inline void report_debug_message, void report_debug_message, report_debug_message);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

// @NOTE: this is one function with optional symbol rather than two separate overloads for a historical reason:
// it has been done so before, and debugging whether call sites pass nulls sometimes would be unnecessarily hard.
inline void report_error(Parser &parser, const Symbol *s, const char *fmt, const DagorSafeArg *arg, int anum)
{
  auto msg = string_dsa(fmt, arg, anum);
  if (s)
  {
    if (!s->macro_call_stack.empty())
      msg.append("\nCall stack:\n");
    for (auto it = s->macro_call_stack.rbegin(); it != s->macro_call_stack.rend(); ++it)
      msg.append_sprintf("  %s()\n    %s(%i)\n", it->name, parser.get_lexer().get_filename(it->file), it->line);
    parser.get_lexer().set_error(s->file_start, s->line_start, s->col_start, msg.c_str());
  }
  else
    parser.get_lexer().set_error(msg.c_str());
}

inline void report_warning(Parser &parser, const Symbol &s, const char *fmt, const DagorSafeArg *arg, int anum)
{
  parser.get_lexer().set_warning(s.file_start, s.line_start, s.col_start, string_dsa(fmt, arg, anum).c_str());
}

inline void report_message(Parser &parser, const Symbol &s, const char *fmt, const DagorSafeArg *arg, int anum)
{
  parser.get_lexer().set_message(s.file_start, s.line_start, s.col_start, string_dsa(fmt, arg, anum).c_str());
}

inline void report_debug_message(Parser &parser, const Symbol &s, const char *fmt, const DagorSafeArg *arg, int anum)
{
  parser.get_lexer().set_debug_message(s.file_start, s.line_start, s.col_start, string_dsa(fmt, arg, anum).c_str());
}