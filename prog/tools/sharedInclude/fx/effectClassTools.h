#ifndef __GAIJIN_FX_EFFECTCLASSTOOLS_H__
#define __GAIJIN_FX_EFFECTCLASSTOOLS_H__
#pragma once


namespace ScriptHelpers
{
class TunedElement;
};


class IEffectClassTools
{
public:
  virtual const char *getClassName() = 0;

  virtual ScriptHelpers::TunedElement *createTunedElement() = 0;
};


void register_effect_class_tools(IEffectClassTools *);
void unregister_effect_class_tools(IEffectClassTools *);

IEffectClassTools *get_effect_class_tools_interface(const char *class_name);


#endif
