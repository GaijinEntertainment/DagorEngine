//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>


class ParamScriptFactory;


class ParamScriptsPool
{
public:
  ParamScriptsPool(IMemAlloc *mem);
  ~ParamScriptsPool();

  bool registerFactory(ParamScriptFactory *);

  void unloadAll();

  int factoryCount() const { return factories.size(); }
  ParamScriptFactory *getFactory(int index) const { return factories[index].factory; }

  ParamScriptFactory *getFactoryByName(const char *class_name);

protected:
  struct Module
  {
    ParamScriptFactory *factory;

    Module();
    Module(const Module &) = delete;
    ~Module();

    void clear();
  };

  Tab<Module> factories;
};
DAG_DECLARE_RELOCATABLE(ParamScriptsPool::Module);
