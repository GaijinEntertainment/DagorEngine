// Copyright (C) Gaijin Games KFT.  All rights reserved.

// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <lightfxShadow_decl.h>
#include <lightFx_decl.h>


ScriptHelpers::TunedElement *LightFxColor::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("allow_game_override", false));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 10));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("rFunc", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("gFunc", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("bFunc", E3DCOLOR(0, 0, 255)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("aFunc", E3DCOLOR(255, 255, 255)));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *LightFxSize::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 10));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("sizeFunc", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *LightFxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_real_param("phaseTime", 1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("burstMode", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("cloudLight", false));
  elems.push_back(LightFxColor::createTunedElement("color"));
  elems.push_back(LightFxSize::createTunedElement("size"));
  elems.push_back(LightfxShadowParams::createTunedElement("shadow"));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


class LightFxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "LightFx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(1);

    elems.push_back(LightFxParams::createTunedElement("LightFxParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static LightFxEffectTools tools;


void register_light_fx_tools() { ::register_effect_class_tools(&tools); }
