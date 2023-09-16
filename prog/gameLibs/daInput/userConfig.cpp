#include <daInput/config_api.h>
#include "actionData.h"
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <startup/dag_inpDevClsDrv.h>
#include <util/dag_string.h>

static constexpr int MAX_BINDING_COLS = 4;
static String def_preset_nm("config/default");
static int defColNum = 4;
static String current_preset_nm;
static DataBlock current_preset_props;
static DataBlock invariant_preset_complementary_cfg;

static DataBlock originalBindingsFromPreset[MAX_BINDING_COLS];
static DataBlock originalPropsFromPreset;


void dainput::set_default_preset_prefix(const char *preset_name_prefix)
{
  def_preset_nm = preset_name_prefix;
  defColNum = 0;
  for (int c = 0; c < MAX_BINDING_COLS; c++)
    if (dd_file_exists(String(0, "%s.c%d.preset.blk", def_preset_nm, c)))
      defColNum = c + 1;
}
const char *dainput::get_default_preset_prefix() { return def_preset_nm; }
int dainput::get_default_preset_column_count() { return defColNum; }

void dainput::set_invariant_preset_complementary_config(const DataBlock &preset_cfg)
{
  invariant_preset_complementary_cfg = preset_cfg;
}

bool dainput::load_user_config(const DataBlock &cfg)
{
  WinAutoLock joyGuard(global_cls_drv_update_cs);
  current_preset_nm = cfg.getStr("preset", def_preset_nm);

  if (defColNum != dainput::get_actions_binding_columns())
  {
    dainput::reset_actions_binding();
    for (int c = 0; c < defColNum; c++)
      dainput::append_actions_binding(DataBlock());
  }

  for (int c = 0; c < dainput::get_actions_binding_columns(); c++)
  {
    // load default config
    String bindingsPath(0, "%s.c%d.preset.blk", current_preset_nm, c);
    dainput::clear_actions_binding(c);

    DataBlock preset_cfg;
    if (dd_file_exists(bindingsPath))
      dblk::load(preset_cfg, bindingsPath);
    dblk::iterate_child_blocks(*invariant_preset_complementary_cfg.getBlockByNameEx(String(0, "c%d", c)), [&](const DataBlock &b) {
      if (!preset_cfg.blockExists(b.getBlockName()))
        preset_cfg.addNewBlock(&b);
    });
    dainput::load_actions_binding(preset_cfg, c);

    // store copy of default preset (to avoid loading later)
    originalBindingsFromPreset[c].clearData();
    dainput::save_actions_binding(originalBindingsFromPreset[c], c);

    // update with user config
    dainput::load_actions_binding(*cfg.getBlockByNameEx(String(0, "c%d", c)), c);
  }

  const DataBlock &blk = *cfg.getBlockByNameEx("props");
  String propsPath(0, "%s.props.preset.blk", current_preset_nm);
  if (dd_file_exists(propsPath))
    originalPropsFromPreset.load(propsPath);
  else
    originalPropsFromPreset.clearData();
  current_preset_props.clearData();
#define LOAD_PROP(TYPE, GETF, SETF)                                                                           \
  if (customPropsScheme.getParamType(i) == TYPE)                                                              \
  {                                                                                                           \
    const char *nm = customPropsScheme.getParamName(i);                                                       \
    current_preset_props.SETF(nm, blk.GETF(nm, originalPropsFromPreset.GETF(nm, customPropsScheme.GETF(i)))); \
  }

  for (int i = 0; i < customPropsScheme.paramCount(); i++)
    LOAD_PROP(DataBlock::TYPE_INT, getInt, setInt)
  else LOAD_PROP(DataBlock::TYPE_REAL, getReal, setReal) else LOAD_PROP(DataBlock::TYPE_BOOL, getBool, setBool)
#undef LOAD_PROP
    return true;
}
bool dainput::save_user_config(DataBlock &cfg, bool save_all_bindings_if_any_changed)
{
  cfg.clearData();
  cfg.setStr("preset", current_preset_nm.empty() ? def_preset_nm : current_preset_nm);

  for (int c = 0; c < dainput::get_actions_binding_columns(); c++)
  {
    DataBlock &blk = *cfg.addBlock(String(0, "c%d", c));
    blk.clearData();
    if (dainput::save_actions_binding_ex(blk, c, &originalBindingsFromPreset[c]))
      if (!save_all_bindings_if_any_changed)
        for (int b = blk.blockCount() - 1; b >= 0; b--)
          if (equalDataBlocks(*blk.getBlock(b), *originalBindingsFromPreset[c].getBlockByNameEx(blk.getBlock(b)->getBlockName())))
            blk.removeBlock(b);
  }

  DataBlock &blk = *cfg.addBlock("props");
#define SAVE_PROP(TYPE, GETF, SETF)                                             \
  if (customPropsScheme.getParamType(i) == TYPE)                                \
  {                                                                             \
    const char *nm = customPropsScheme.getParamName(i);                         \
    auto def_val = originalPropsFromPreset.GETF(nm, customPropsScheme.GETF(i)); \
    auto val = current_preset_props.GETF(nm, def_val);                          \
    if (def_val != val)                                                         \
      blk.SETF(nm, val);                                                        \
  }

  for (int i = 0; i < customPropsScheme.paramCount(); i++)
    SAVE_PROP(DataBlock::TYPE_INT, getInt, setInt)
  else SAVE_PROP(DataBlock::TYPE_REAL, getReal, setReal) else SAVE_PROP(DataBlock::TYPE_BOOL, getBool, setBool)
#undef SAVE_PROP
    if (!blk.paramCount()) cfg.removeBlock("props");
  return true;
}

const char *dainput::get_user_config_base_preset() { return current_preset_nm; }

bool dainput::is_user_config_customized()
{
  DataBlock blk;
  for (int c = 0; c < dainput::get_actions_binding_columns(); c++)
    if (dainput::save_actions_binding_ex(blk, c, &originalBindingsFromPreset[c]))
      return true;
    else
      blk.clearData();
  return false;
}


void dainput::reset_user_config_to_preset(const char *preset_name_prefix, bool move_changes)
{
  WinAutoLock joyGuard(global_cls_drv_update_cs);
  DataBlock cfg;
  if (move_changes)
    dainput::save_user_config(cfg, false);
  cfg.setStr("preset", preset_name_prefix);
  dainput::load_user_config(cfg);
}

void dainput::reset_user_config_to_currest_preset()
{
  if (current_preset_nm.empty())
  {
    logerr("dainput: current preset not set, reset is skipped");
    return;
  }

  WinAutoLock joyGuard(global_cls_drv_update_cs);
  DataBlock cfg;
  cfg.setStr("preset", current_preset_nm);
  dainput::load_user_config(cfg);
}

DataBlock &dainput::get_user_props() { return current_preset_props; }
bool dainput::is_user_props_customized()
{
#define CHECK_PROP(TYPE, GETF)                                                  \
  if (customPropsScheme.getParamType(i) == TYPE)                                \
  {                                                                             \
    const char *nm = customPropsScheme.getParamName(i);                         \
    auto def_val = originalPropsFromPreset.GETF(nm, customPropsScheme.GETF(i)); \
    if (def_val != current_preset_props.GETF(nm, def_val))                      \
      return true;                                                              \
  }

  for (int i = 0; i < customPropsScheme.paramCount(); i++)
    CHECK_PROP(DataBlock::TYPE_INT, getInt)
  else CHECK_PROP(DataBlock::TYPE_REAL, getReal) else CHECK_PROP(DataBlock::TYPE_BOOL, getBool)
#undef CHECK_PROP

    return false;
}

void dainput::term_user_config()
{
  current_preset_props.resetAndReleaseRoNameMap();
  invariant_preset_complementary_cfg.resetAndReleaseRoNameMap();
  for (int i = 0; i < MAX_BINDING_COLS; ++i)
    originalBindingsFromPreset[i].resetAndReleaseRoNameMap();
  originalPropsFromPreset.resetAndReleaseRoNameMap();
}
