// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riDestrProps.h"
#include "namedPropsTable.h"
#include "gamePhys/phys/destructableProps.h"

#include <rendInst/rendInstProps.h>
#include <rendInst/rendInstExtra.h>
#include <anim/dag_animChannels.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/deque.h>
#include <EASTL/fixed_string.h>

namespace rendinstdestr
{

void FrxDestrProps::loadFromBlk(const DataBlock &blk)
{
  if (const DataBlock *shatterBlk = blk.getBlockByName("shatter"))
  {
    flags |= FrxDestrFlag::Shatter;
    shatterProfile.loadFromBlk(*shatterBlk);
  }
  if (const char *matName = blk.getStr("interiorMat", nullptr))
    interiorMat = get_or_load_ri_destr_material(matName);
}

void RiDestrParams::loadFromBlk(const DataBlock &) {}

void RiNearbyGroupsProps::loadFromBlk(const char *ri_name, const DataBlock &ri_blk)
{
  dblk::iterate_child_blocks_by_name(ri_blk, "nearbyRiGroup", [&](const DataBlock &blk) {
    Group group;
    dblk::iterate_params_by_name_and_type(blk, "action", DataBlock::TYPE_STRING, [&](int param_idx) {
      const char *action = blk.getStr(param_idx);
      if (strcmp(action, "reveal") == 0)
        group.actions.reveal = true;
      else if (strcmp(action, "frxInclude") == 0)
        group.actions.frxInclude = true;
      else if (strcmp(action, "frxInvert") == 0)
        group.actions.frxInvert = true;
      else if (strcmp(action, "innerSurface") == 0)
        group.actions.reveal = group.actions.frxInclude = group.actions.frxInvert = true;
      else
        logerr("rendinst <%s>: unknown nearbyRiGroup action <%s>", ri_name, action);
    });
    if (group.actions.frxInvert && !group.actions.frxInclude)
      logerr("rendinst <%s>: nearbyRiGroup action frxInvert requires frxInclude", ri_name);
    group.extent = blk.getPoint3("extent", group.extent);
    group.poolsBegin = pools.size();
    dblk::iterate_params_by_name_and_type(blk, "name", DataBlock::TYPE_STRING, [&](int param_idx) {
      const char *poolName = blk.getStr(param_idx);
      if (const int pool = rendinst::getRIGenExtraResIdx(poolName); pool >= 0)
        pools.push_back(pool);
      else
        logerr("rendinst <%s>: unknown nearbyRiGroup ri pool <%s>", ri_name, poolName);
    });
    group.poolsEnd = pools.size();
    if (group.poolsEnd > group.poolsBegin)
      groups.push_back(group);
  });
}

struct RiDestrRegistry
{
  static constexpr const char *DEFAULT_PRESET_NAME = "__default";

  eastl::deque<RiDestrProps> riTypes; // indexed by rendinst custom props id
  NamedPropsTable<RiDestrProps> presets;
  NamedPropsTable<FrxDestrProps> frxDestrProps;
  NamedPropsTable<destructables::DestrObjProps> destrObjProps;
  NamedPropsTable<destructables::DestrAnimData> destrAnimData;
  NamedPropsTable<RiNearbyGroupsProps> nearbyGroupsProps;
  NamedPropsTable<RiDestrMaterial> materials;
  FastNameMap resolvingPresets;
  const RiDestrProps *defaultPreset = nullptr;
  CallbackToken loadCb, clearCb;

  RiDestrRegistry()
  {
    loadCb = rendinst::props::custom_props_load_cb.subscribe(
      [this](int props_id, const char *ri_name, const DataBlock *ri_blk, const DataBlock *cfg_blk) {
        loadRiType(props_id, ri_name, ri_blk ? *ri_blk : DataBlock::emptyBlock, *cfg_blk);
      });
    clearCb = rendinst::props::custom_props_clear_all_cb.subscribe([this](bool) { clear(); });
  }

  void loadRiType(int props_id, const char *ri_name, const DataBlock &ri_blk, const DataBlock &cfg_blk)
  {
    if (riTypes.size() <= props_id)
      riTypes.resize(props_id + 1);
    RiDestrProps &props = riTypes[props_id];
    props = getOrLoadPreset(ri_blk.getStr("destrPreset", ""), cfg_blk);
    props.params.loadFromBlk(ri_blk);
    eastl::fixed_string<char, 128> inlineName("@"); // rendinst names share the tables with preset names
    inlineName += ri_name;
    loadFeatures(props, inlineName.c_str(), ri_blk, cfg_blk);
    if (ri_blk.blockExists("nearbyRiGroup"))
      props.nearbyGroups = nearbyGroupsProps.getOrAdd(inlineName.c_str(), [&](RiNearbyGroupsProps &p) {
        p.loadFromBlk(ri_name, ri_blk);
        return !p.groups.empty();
      });
    const int riExPoolId = rendinst::getRIGenExtraResIdx(ri_name);
    if (props.nearbyGroups && riExPoolId >= 0)
      for (const int nearbyPoolId : props.nearbyGroups->pools)
        rendinst::addRIGenExtraDestrLodPool(riExPoolId, nearbyPoolId);
  }

  const RiDestrProps &getOrLoadPreset(const char *name, const DataBlock &cfg_blk)
  {
    const DataBlock &presetsBlk = *cfg_blk.getBlockByNameEx("DestructionPresets");
    if (!defaultPreset)
      defaultPreset = presets.getOrAdd(DEFAULT_PRESET_NAME, [&](RiDestrProps &preset) {
        loadPreset(preset, DEFAULT_PRESET_NAME, *presetsBlk.getBlockByNameEx(DEFAULT_PRESET_NAME), cfg_blk);
        return true;
      });
    if (!name || !*name)
      return *defaultPreset;
    if (resolvingPresets.getNameId(name) >= 0)
    {
      logerr("destrPreset <%s> references itself through _use", name);
      return *defaultPreset;
    }
    const RiDestrProps *preset = presets.getOrAdd(name, [&](RiDestrProps &p) {
      const DataBlock *blk = presetsBlk.getBlockByName(name);
      G_LOGERR_AND_DO(blk, return false, "unknown destrPreset <%s>", name);
      p = *defaultPreset;
      loadPreset(p, name, *blk, cfg_blk);
      return true;
    });
    return preset ? *preset : *defaultPreset;
  }

  void loadPreset(RiDestrProps &preset, const char *name, const DataBlock &blk, const DataBlock &cfg_blk)
  {
    const int resolvingId = resolvingPresets.addNameId(name);
    if (blk.blockExists("nearbyRiGroup"))
      logerr("destrPreset <%s>: nearbyRiGroup is allowed only in rendinst blocks", name);
    if (const char *base = blk.getStr("_use", nullptr))
      preset = *base ? getOrLoadPreset(base, cfg_blk) : RiDestrProps{};
    loadParams(preset, blk, cfg_blk);
    loadFeatures(preset, name, blk, cfg_blk);
    resolvingPresets.erase(resolvingId);
  }

  // params{} in a preset follows the feature chain rules; a rendinst block keeps its params flat in the root
  void loadParams(RiDestrProps &preset, const DataBlock &blk, const DataBlock &cfg_blk)
  {
    if (const DataBlock *paramsBlk = blk.getBlockByName("params"))
    {
      if (const char *use = paramsBlk->getStr("_use", nullptr))
        preset.params = *use ? getOrLoadPreset(use, cfg_blk).params : RiDestrParams{};
      preset.params.loadFromBlk(*paramsBlk);
    }
    else if (const char *ref = blk.getStr("params", nullptr))
      preset.params = *ref ? getOrLoadPreset(ref, cfg_blk).params : RiDestrParams{};
  }

  void loadFeatures(RiDestrProps &dst, const char *inline_name, const DataBlock &blk, const DataBlock &cfg_blk)
  {
    loadFeature(dst, &RiDestrProps::frxDestr, "frxDestr", frxDestrProps, inline_name, blk, cfg_blk);
    loadFeature(dst, &RiDestrProps::destrObj, "destrObj", destrObjProps, inline_name, blk, cfg_blk);
    loadFeature(dst, &RiDestrProps::destrAnim, "destrAnim", destrAnimData, inline_name, blk, cfg_blk);
  }

  template <typename T>
  void loadFeature(RiDestrProps &dst, const T *RiDestrProps::*member, const char *feature, NamedPropsTable<T> &table,
    const char *inline_name, const DataBlock &blk, const DataBlock &cfg_blk)
  {
    if (const DataBlock *featureBlk = blk.getBlockByName(feature))
    {
      const T *parent = dst.*member;
      if (const char *use = featureBlk->getStr("_use", nullptr))
        parent = *use ? getOrLoadPreset(use, cfg_blk).*member : nullptr;
      dst.*member = table.getOrAdd(inline_name, [&](T &f) {
        if (parent)
          f = *parent;
        f.loadFromBlk(*featureBlk);
        return true;
      });
    }
    else if (const char *ref = blk.getStr(feature, nullptr))
      dst.*member = *ref ? getOrLoadPreset(ref, cfg_blk).*member : nullptr;
  }

  const RiDestrMaterial *getOrLoadMaterial(const char *name)
  {
    if (!name || !*name)
      return nullptr;
    return materials.getOrAdd(name, [&](RiDestrMaterial &mat) {
      mat.matData = (MaterialData *)get_one_game_resource_ex(name, MaterialGameResClassId);
      G_LOGERR_AND_DO(mat.matData, return false, "can't load material resource <%s>", name);
      mat.matData.delRef(); // Ptr<>::operator= calls addRef
      mat.shMat = ::new_shader_material(*mat.matData);
      G_LOGERR_AND_DO(mat.shMat, return false, "can't make ShaderMaterial for material <%s>", name);
      mat.shElem = mat.shMat->make_elem();
      G_LOGERR_AND_DO(mat.shElem, return false, "can't make ShaderElement for material <%s>", name);
      return true;
    });
  }

  void clear()
  {
    riTypes.clear();
    presets.clear();
    frxDestrProps.clear();
    destrObjProps.clear();
    destrAnimData.clear();
    nearbyGroupsProps.clear();
    materials.clear();
    defaultPreset = nullptr;
  }
};

static InitOnDemand<RiDestrRegistry> ri_destr_registry;

void init_ri_destr_props() { ri_destr_registry.demandInit(); }
void shutdown_ri_destr_props() { ri_destr_registry.demandDestroy(); }

const RiDestrProps *get_ri_destr_props(const rendinst::RendInstDesc &desc)
{
  const RiDestrRegistry &reg = *ri_destr_registry;
  const int propsId = rendinst::props::get_custom_props_id(desc);
  return unsigned(propsId) < reg.riTypes.size() ? &reg.riTypes[propsId] : nullptr;
}

const RiDestrMaterial *get_or_load_ri_destr_material(const char *name) { return ri_destr_registry->getOrLoadMaterial(name); }

} // namespace rendinstdestr
