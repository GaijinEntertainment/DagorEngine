// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <startup/dag_globalSettings.h>
#include <render/hdrRender.h>
#include "renderSettings.h"
#include "rendererFeatures.h"
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>

ECS_REGISTER_EVENT(OnRenderSettingsReady)

extern const char *const EMPTY_LEVEL_NAME;

namespace ecs
{
extern bool ecs_is_in_init_phase;
}

template <class CB>
static void gather_components_from_blk(const DataBlock &blk, CB cb, const char *prefix)
{
  dblk::iterate_params(blk, [&](int param_idx, int param_name_id, int param_type) {
    eastl::string prefix_name(eastl::string::CtorSprintf(), "%s%s", prefix, blk.getName(param_name_id));
    auto hashName = ECS_HASH_SLOW(prefix_name.c_str());
    switch (param_type)
    {
      case DataBlock::TYPE_REAL: cb(hashName, blk.getReal(param_idx)); break;

      case DataBlock::TYPE_BOOL: cb(hashName, blk.getBool(param_idx)); break;

      case DataBlock::TYPE_INT: cb(hashName, blk.getInt(param_idx)); break;

      case DataBlock::TYPE_STRING: cb(hashName, blk.getStr(param_idx)); break;

      case DataBlock::TYPE_POINT2: cb(hashName, blk.getPoint2(param_idx)); break;

      case DataBlock::TYPE_POINT3: cb(hashName, blk.getPoint3(param_idx)); break;

      case DataBlock::TYPE_POINT4: cb(hashName, blk.getPoint4(param_idx)); break;

      case DataBlock::TYPE_IPOINT2: cb(hashName, blk.getIPoint2(param_idx)); break;

      case DataBlock::TYPE_IPOINT3: cb(hashName, blk.getIPoint3(param_idx)); break;

      case DataBlock::TYPE_E3DCOLOR: cb(hashName, blk.getE3dcolor(param_idx)); break;

      case DataBlock::TYPE_MATRIX: cb(hashName, blk.getTm(param_idx)); break;

      case DataBlock::TYPE_INT64: cb(hashName, blk.getInt64(param_idx)); break;

      default: logerr("unsupported [%d] type for conversion, add more convertors", param_type); break;
    }
  });
}

extern const eastl::pair<FeatureRenderFlags, const char *> *get_feature_render_flags_str_table(size_t &cnt);

template <class CB>
static void gather_components_from_render_features(FeatureRenderFlagMask val, CB cb, const char *prefix)
{
  size_t cnt;
  auto features = get_feature_render_flags_str_table(cnt);
  for (auto i = features, e = features + cnt; i != e; ++i)
  {
    eastl::string prefix_name(eastl::string::CtorSprintf(), "%s%s", prefix, i->second);
    cb(ECS_HASH_SLOW(prefix_name.c_str()), val.test(i->first));
  }
}

template <class CB>
static void gather_components(const DataBlock *level_override, CB cb)
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const DataBlock *videoBlk = ::dgs_get_settings()->getBlockByNameEx("video");

  gather_components_from_render_features(get_current_render_features(), cb, "render_settings__");

  gather_components_from_blk(*graphicsBlk, cb, "render_settings__");
  gather_components_from_blk(*videoBlk, cb, "render_settings__");
  if (level_override)
    gather_components_from_blk(*level_override, cb, "render_settings__");
  cb(ECS_HASH("render_settings__is_hdr_enabled"), hdrrender::is_hdr_enabled());
  bool bare_minimum = false;
#if _TARGET_PC_WIN || _TARGET_APPLE
  bare_minimum = strcmp(graphicsBlk->getStr("preset", "medium"), "bareMinimum") == 0;
#elif _TARGET_XBOXONE || _TARGET_C1
  bare_minimum = strcmp(graphicsBlk->getStr("consolePreset", "HighFPS"), "bareMinimum") == 0;
#endif
  cb(ECS_HASH("render_settings__bare_minimum"), bare_minimum);
}

template <class Value>
static void set_component_ref(ecs::EntityId eid, ecs::HashedConstString hash, Value v)
{
  if (Value *rf = g_entity_mgr->getNullableRW<Value>(eid, hash))
    *rf = v;
}

static void set_component_ref(ecs::EntityId eid, ecs::HashedConstString hash, const char *v)
{
  if (ecs::string *rf = g_entity_mgr->getNullableRW<ecs::string>(eid, hash))
    *rf = v;
}

bool update_settings_entity(const DataBlock *level_override)
{
  if (auto eid = g_entity_mgr->getSingletonEntity(ECS_HASH("render_settings")))
  {
    gather_components(level_override, [eid](const ecs::HashedConstString &s, auto v) { set_component_ref(eid, s, v); });

    if (bool *render_settings__inited = ECS_GET_COMPONENT(bool, eid, render_settings__inited))
    {
      if (!*render_settings__inited)
      {
        g_entity_mgr->broadcastEvent(OnRenderSettingsReady());
        *render_settings__inited = true;
      }
    }
    if (!ecs::ecs_is_in_init_phase) // FIXME: this is ugly but otherwise it might crashes on unresolved sq ESes
      g_entity_mgr->performTrackChanges(true);
    return true;
  }

  return false;
}

template <class T>
static void prepare_united_vdata_setup(T &unitedVdata, const DataBlock *levelBlk, const char *blk_name, const char *def_blk_name)
{
  bool streaming_enabled = T::ResType::on_higher_lod_required != nullptr;
  const DataBlock *b = levelBlk ? levelBlk->getBlockByName(blk_name) : nullptr;
  if (const DataBlock *b2 = b ? b->getBlockByName(get_platform_string_id()) : nullptr)
    b = b2;
  if (b && !streaming_enabled)
    if (const DataBlock *b2 = b->getBlockByName("streaming_disabled"))
      b = b2;

  if (!b)
  {
    b = ::dgs_get_game_params()->getBlockByName(def_blk_name);
    if (const DataBlock *b2 = b ? b->getBlockByName(get_platform_string_id()) : nullptr)
      b = b2;
  }
  if (b)
  {
    debug("using %s (streaming=%d, %s):", b->getBlockName(), streaming_enabled, blk_name);
    unitedVdata.setHints(*b);
  }
}

void prepare_ri_united_vdata_setup(const DataBlock *level_blk)
{
  prepare_united_vdata_setup(unitedvdata::riUnitedVdata, level_blk, "unitedVdata.rendInst", "unitedVdata.rendInst.def");
}

void prepare_dynm_united_vdata_setup(const DataBlock *level_blk)
{
  prepare_united_vdata_setup(unitedvdata::dmUnitedVdata, level_blk, "unitedVdata.dynModel", "unitedVdata.dynModel.def");
}

void apply_united_vdata_settings(const DataBlock *scene_blk)
{
  if (!scene_blk)
  {
    prepare_united_vdata_setup();
    return;
  }
  bool applied = false;
  int entityNid = scene_blk->getNameId("entity"), levelBlkNid = scene_blk->getNameId("level__blk");
  auto searchAndApplyToLevelEntity = [&](const DataBlock &blk) {
    for (uint32_t i = 0, e = blk.blockCount(); i < e && !applied; ++i)
    {
      const DataBlock *eblk = blk.getBlock(i);
      if (eblk->getBlockNameId() != entityNid)
        continue;
      dblk::iterate_params_by_name_id_and_type(*eblk, levelBlkNid, DataBlock::TYPE_STRING, [&](int pid) {
        if (!applied)
        {
          applied = true;
          DataBlock levelBlk;
          const char *levelPath = eblk->getStr(pid);
          bool ok = strcmp(levelPath, EMPTY_LEVEL_NAME) != 0 && dblk::load(levelBlk, levelPath, dblk::ReadFlag::ROBUST_IN_REL);
          prepare_united_vdata_setup(ok ? &levelBlk : nullptr);
        }
      });
    }
  };
  searchAndApplyToLevelEntity(*scene_blk);
  if (applied)
    return;
  // Level entity still not found? Try iterating imports then
  // To consider: this probably need to be refactored to be method of io/datablock lib
  int importNid = scene_blk->getNameId("import"), sceneNid = scene_blk->getNameId("scene");
  for (uint32_t i = 0, e = scene_blk->blockCount(); i < e && !applied; ++i)
  {
    const DataBlock *blk = scene_blk->getBlock(i);
    if (blk->getBlockNameId() == importNid)
      for (int j = 0, pe = blk->paramCount(); j < pe && !applied; ++j)
        if (blk->getParamNameId(j) == sceneNid)
        {
          DataBlock importBlk;
          if (dblk::load(importBlk, blk->getStr(j), dblk::ReadFlag::ROBUST_IN_REL))
            searchAndApplyToLevelEntity(importBlk);
        }
  }

  if (!applied)
    prepare_united_vdata_setup();
}
