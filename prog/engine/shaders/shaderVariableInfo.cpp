// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderVariableInfo.h>

ShaderVariableInfo *ShaderVariableInfo::head;

void ShaderVariableInfo::resolveAll()
{
  for (ShaderVariableInfo *i = head; i; i = i->next)
    i->resolve();
}

ShaderVariableInfo &ShaderVariableInfo::operator=(int v_id)
{
  data = nullptr;
  var_id = v_id;
  resolve();
  return *this;
}

void ShaderVariableInfo::deleteFromLinkedList()
{
  for (ShaderVariableInfo *i = head, *prev = nullptr; i; prev = i, i = i->next)
  {
    if (i == this)
    {
      if (prev)
        prev->next = i->next;
      break;
    }
  }
}