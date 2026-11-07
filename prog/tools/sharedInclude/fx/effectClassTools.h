// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <scriptHelpers/scriptHelpers.h>

class IEffectClassTools
{
public:
  virtual const char *getClassName() = 0;

  virtual ScriptHelpers::TunedElement *createTunedElement() = 0;
};


void register_effect_class_tools(IEffectClassTools *);
void unregister_effect_class_tools(IEffectClassTools *);

IEffectClassTools *get_effect_class_tools_interface(const char *class_name);

// Returns warnings about risky parameter combinations; params is the block ScriptHelpers::save_helpers() writes.
ScriptHelpers::ParamWarnings get_effect_param_warnings(const char *class_name, const DataBlock &params);
