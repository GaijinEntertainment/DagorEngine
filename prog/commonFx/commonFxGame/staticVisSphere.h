// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <fx/dag_baseFxClasses.h>

#include <staticVisSphere_decl.h>

class StaticVisSphere
{
public:
  StaticVisSphereParams par;

  StaticVisSphere() {}

  virtual void load(const char *&ptr, int len, BaseParamScriptLoadCB *load_cb) { par.load(ptr, len, load_cb); }

  virtual void setParam(unsigned /*id*/, void * /*value*/) {}
  virtual void *getParam(unsigned /*id*/, void * /*value*/) { return NULL; }
};
