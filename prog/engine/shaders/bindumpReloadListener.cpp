// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_bindumpReloadListener.h>

IShaderBindumpReloadListener *IShaderBindumpReloadListener::head = nullptr;

void IShaderBindumpReloadListener::resolveAll()
{
  for (IShaderBindumpReloadListener *i = head; i; i = i->next)
    i->resolve();
}

void IShaderBindumpReloadListener::deleteFromLinkedList()
{
  for (IShaderBindumpReloadListener *i = head, *prev = nullptr; i; prev = i, i = i->next)
  {
    if (i == this)
    {
      if (prev)
        prev->next = i->next;
      break;
    }
  }
}
