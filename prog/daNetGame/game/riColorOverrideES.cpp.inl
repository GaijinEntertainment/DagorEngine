// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/riShaderConstBuffers.h>
#include <render/renderEvent.h>
#include <EASTL/string_map.h>

struct ColorPair
{
  E3DCOLOR from;
  E3DCOLOR to;
};

static eastl::string_map<ColorPair> originalColors;

extern bool is_level_loaded();

ECS_ON_EVENT(on_appear, OnLevelLoaded)
ECS_TAG(render)
static void init_ri_color_override_es(const ecs::Event &,
  const ecs::string &ri_color_override__name,
  const E3DCOLOR &ri_color_override__from,
  const E3DCOLOR &ri_color_override__to)
{
  if (is_level_loaded())
  {
    E3DCOLOR prevFrom, prevTo;
    rendinst::render::updateRiColorByName(ri_color_override__name.c_str(), ri_color_override__from, ri_color_override__to, &prevFrom,
      &prevTo);
    originalColors[ri_color_override__name.c_str()] = {prevFrom, prevTo};
  }
}

ECS_TAG(render)
ECS_TRACK(*)
static void edit_ri_color_override_es(const ecs::Event &,
  const ecs::string &ri_color_override__name,
  const E3DCOLOR &ri_color_override__from,
  const E3DCOLOR &ri_color_override__to)
{
  rendinst::render::updateRiColorByName(ri_color_override__name.c_str(), ri_color_override__from, ri_color_override__to);
}

ECS_ON_EVENT(on_disappear)
ECS_TAG(render)
static void destr_ri_color_override_es(const ecs::Event &, const ecs::string &ri_color_override__name)
{
  if (!rendinst::isRiExtraLoaded()) // they may be already cleared on level unloading
    return;
  const ColorPair &treeColor = originalColors[ri_color_override__name.c_str()];
  rendinst::render::updateRiColorByName(ri_color_override__name.c_str(), treeColor.from, treeColor.to);
}
