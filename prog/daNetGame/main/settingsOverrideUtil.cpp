// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "settingsOverrideUtil.h"
#include <ecs/core/entityManager.h>
#include <ioSys/dag_dataBlockUtils.h>

static void find_diff_in_config_blk_impl(
  FastNameMap &result, const eastl::string &base_name, const DataBlock &game, const DataBlock &overridden)
{
  for (int i = 0; i < overridden.blockCount(); i++)
  {
    const DataBlock *overriddenDescendant = overridden.getBlock(i);
    eastl::string descendantName =
      eastl::string(eastl::string::CtorSprintf{}, "%s%s/", base_name.c_str(), overriddenDescendant->getBlockName());
    // In the root ignore everything but graphics, video and cinematicEffects block
    if (base_name.empty() && descendantName != "graphics/" && descendantName != "video/" && descendantName != "cinematicEffects/")
      continue;
    const DataBlock *gameDescendant = game.getBlockByNameEx(overriddenDescendant->getBlockName());
    find_diff_in_config_blk_impl(result, descendantName, *gameDescendant, *overriddenDescendant);
  }
  // Do not add parameters in the root
  if (base_name.empty())
    return;
  for (int i = 0; i < overridden.paramCount(); i++)
  {
    DataBlock::ParamType paramType = static_cast<DataBlock::ParamType>(overridden.getParamType(i));
    const char *paramName = overridden.getParamName(i);
#define ADD()                                  \
  do                                           \
  {                                            \
    eastl::string tmp = base_name + paramName; \
    result.addNameId(tmp.c_str());             \
  } while (0)
#define DIFFERENT(getter) !game.paramExists(paramName) || game.getter(paramName) != overridden.getter(i)
    switch (paramType)
    {
      case DataBlock::ParamType::TYPE_STRING:
        if (!game.paramExists(paramName) || strcmp(game.getStr(paramName), overridden.getStr(i)) != 0)
          ADD();
        break;
      case DataBlock::ParamType::TYPE_INT:
        if (DIFFERENT(getInt))
          ADD();
        break;
      case DataBlock::ParamType::TYPE_REAL:
        if (DIFFERENT(getReal))
          ADD();
        break;
      case DataBlock::ParamType::TYPE_BOOL:
        if (DIFFERENT(getBool))
          ADD();
        break;
      case DataBlock::ParamType::TYPE_POINT2:
        if (DIFFERENT(getPoint2))
          ADD();
        break;
      case DataBlock::ParamType::TYPE_POINT3:
        if (DIFFERENT(getPoint3))
          ADD();
        break;
      case DataBlock::ParamType::TYPE_POINT4:
        if (DIFFERENT(getPoint4))
          ADD();
        break;
      default: logwarn("%s has a type different from string, int, float, bool, Point(N)", paramName); break;
    }
#undef DIFFERENT
#undef ADD
  }
}

FastNameMap find_difference_in_config_blk(const DataBlock &game, const DataBlock &overridden)
{
  FastNameMap result;
  find_diff_in_config_blk_impl(result, eastl::string(), game, overridden);
  return result;
}

// Applies object on top of game_settings to properly revert to the old settings.
DataBlock convert_settings_object_to_blk(const ecs::Object &object, const DataBlock *game_settings)
{
  DataBlock result = game_settings ? *game_settings : DataBlock{};
  for (const auto &entry : object)
  {
    if (entry.second.is<float>())
      blk_create_path_and_set_float(&result, entry.first.c_str(), entry.second.get<float>());
    else if (entry.second.is<int>())
      blk_create_path_and_set_int(&result, entry.first.c_str(), entry.second.get<int>());
    else if (entry.second.is<bool>())
      blk_create_path_and_set_bool(&result, entry.first.c_str(), entry.second.get<bool>());
    else if (entry.second.is<ecs::string>())
      blk_create_path_and_set_str(&result, entry.first.c_str(), entry.second.get<ecs::string>().c_str());
    else if (entry.second.is<Point2>())
      blk_create_path_and_set_point2(&result, entry.first.c_str(), entry.second.get<Point2>());
    else if (entry.second.is<Point3>())
      blk_create_path_and_set_point3(&result, entry.first.c_str(), entry.second.get<Point3>());
    else if (entry.second.is<Point4>())
      blk_create_path_and_set_point4(&result, entry.first.c_str(), entry.second.get<Point4>());
    else
      logerr("%s has a type different from string, int, float, bool, Point(N)", entry.first.c_str());
  }
  return result;
}