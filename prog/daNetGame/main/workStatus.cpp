// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/workStatus.h"

namespace workstatus
{
namespace
{
Callback g_cb = nullptr;
}

void set_callback(Callback cb) { g_cb = cb; }

Scope::Scope(const char *msg) : ptr((msg && msg[0]) ? msg : nullptr)
{
  if (ptr && g_cb)
    g_cb(true, ptr);
}

Scope::~Scope()
{
  if (ptr && g_cb)
    g_cb(false, ptr);
}

} // namespace workstatus
