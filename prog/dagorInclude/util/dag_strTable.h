//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <string.h>

struct StrTable
{
  const char *data;
  int len;

  StrTable() : data(0), len(0) {}
  ~StrTable() { clear(); }

  void clear()
  {
    if (data)
    {
      memfree((void *)data, strmem);
      data = NULL;
      len = 0;
    }
  }

  template <bool replace, class V, class Pred, typename T = typename V::value_type>
  void build(const V &in_out_table, Pred pred, IMemAlloc *ex_mem = strmem)
  {
    char *old_data = (char *)data;
    int old_len = len;
    int l = 0;
    for (int i = 0; i < in_out_table.size(); i++)
      l += (int)strlen(pred(in_out_table[i])) + 1;
    len = l;

    char *m = (char *)memalloc(l, strmem);
    data = m;
    for (int i = 0; i < in_out_table.size(); ++i)
    {
      char *&s = pred(in_out_table[i]);
      int ll = (int)strlen(s) + 1;
      strcpy(m, s);
      if (replace)
      {
        if (!old_data || (s < old_data || s >= old_data + old_len))
          memfree(s, ex_mem);
        s = m;
      }
      m += ll;
    }
    G_ASSERT(m == (data + l));
    if (replace && old_data)
      memfree((void *)old_data, strmem);
  }

  bool is_belong(const char *s) const { return data ? (s >= data && s < (data + len)) : false; }
};
