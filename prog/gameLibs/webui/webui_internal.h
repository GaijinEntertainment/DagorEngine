// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <webui/helpers.h>
#include <util/dag_string.h>

// Warning : do not put this macroses in header, at least not with this names
#define TD  buf.printf("<td>")
#define _TD buf.printf("</td>")
#define TCOL(...)            \
  do                         \
  {                          \
    TD;                      \
    buf.printf(__VA_ARGS__); \
    _TD;                     \
  } while (0)

static inline void print_table_header(webui::YAMemSave &buf, const char **hs, int hs_count)
{
  buf.printf("  <tr align='center'>");
  for (int i = 0; i < hs_count; ++i)
    buf.printf("<th>%s</th>", hs[i]);
  buf.printf("</tr>\n");
}

String get_name_of_executable();
