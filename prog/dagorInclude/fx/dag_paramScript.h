//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BaseParamScriptLoadCB
{
public:
  virtual void *getReference(int id) = 0;
  virtual int getSelfGameResId() = 0;
  virtual const char *getBrokenRefName(int) { return nullptr; };
};


// Base class for C++ scripts with tuned parameters.
// Should be used with ScriptHelpers tool library.
class BaseParamScript
{
public:
  BaseParamScript() = default;
  BaseParamScript(const BaseParamScript &) = default;
  BaseParamScript(BaseParamScript &&) = default;
  BaseParamScript &operator=(const BaseParamScript &) = default;
  BaseParamScript &operator=(BaseParamScript &&) = default;
  virtual ~BaseParamScript() {}

  virtual BaseParamScript *clone() = 0;

  // Load all parameter values from raw binary data.
  virtual void loadParamsData(const char *ptr, int len, BaseParamScriptLoadCB *load_cb) = 0;

  // Change value of the specified parameter.
  // NOTE: Not all parameters may be supported, use hashed names as ids.
  virtual void setParam(unsigned id, void *value) = 0;

  // Get value of the specified parameter.
  // NOTE: Not all parameters may be supported, use hashed names as ids.
  // Return value and usage of 'value' pointer depends on the specific parameter.
  // NULL should be returned when unsupported id is passed to this method.
  virtual void *getParam(unsigned id, void *value) = 0;

  // Basic RTTI, use hashed class names as ids.
  virtual bool isSubOf(unsigned /*id*/) { return false; }
};


// Factory for BaseParamScript.
class ParamScriptFactory
{
public:
  virtual BaseParamScript *createObject() = 0;
  virtual void destroyFactory() = 0;
  virtual const char *getClassName() = 0;

  // Basic RTTI, use hashed class names as ids.
  virtual bool isSubOf(unsigned /*id*/) { return false; }
};
