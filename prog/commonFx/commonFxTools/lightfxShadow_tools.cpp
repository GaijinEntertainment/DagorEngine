// Copyright (C) Gaijin Games KFT.  All rights reserved.

// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <lightfxShadow_decl.h>


ScriptHelpers::TunedElement *LightfxShadowParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_bool_param("enabled", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("is_dynamic_light", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("shadows_for_dynamic_objects", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("shadows_for_gpu_objects", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("sdf_shadows", false));
  elems.push_back(ScriptHelpers::create_tuned_int_param("quality", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("shrink", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("priority", 0));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}
