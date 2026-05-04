// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/autoGraphics.h>
#include <render/antialiasing.h>

#include <startup/dag_globalSettings.h>
#include "main/settings.h"

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>

#include <util/dag_delayedAction.h>

#include <drv/3d/dag_resetDevice.h>

#include "render/renderer.h"

namespace auto_graphics::startup
{

void try_apply_auto_graphics_settings()
{
  G_ASSERT(d3d::is_inited());

  auto autoPreset = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("autoPreset");
  if (!autoPreset->paramExists("targetFps") || autoPreset->getBool("forceDisable", false))
  {
    debug("skipping auto graphical settings");
    return;
  }

  if (auto autoPreset = auto_quality_preset::get_auto_preset(); autoPreset.has_value())
  {
    auto &[selectedResolution, preset, targetFps] = autoPreset.value();

    debug("selecting {preset='%s', resolution=%dx%d, targetFps=%d}", preset.c_str(), selectedResolution.x, selectedResolution.y,
      targetFps);

    auto graphicsOverride = get_settings_override_blk()->addBlock("graphics");
    graphicsOverride->setStr("preset", preset.c_str());

    auto autoPresetOverride = graphicsOverride->addBlock("autoPreset");
    autoPresetOverride->setInt("targetFps", targetFps);
    autoPresetOverride->setIPoint2("renderingResolution", selectedResolution);

    dgs_apply_essential_pc_preset_params(*get_settings_override_blk(), preset.c_str());
    save_settings(nullptr);
  }
}

static FastNameMap changed_fields;

static void fill_changes_name_map(const DataBlock &curr, const DataBlock &changed, const String &path_so_far)
{
  for (int i = 0; i < changed.blockCount(); ++i)
  {
    const DataBlock &childChangedBlk = *changed.getBlock(i);
    const DataBlock &childCurrBlk = *curr.getBlockByNameEx(childChangedBlk.getBlockName());
    fill_changes_name_map(childCurrBlk, childChangedBlk, path_so_far + childChangedBlk.getBlockName() + "/");
  }

  for (int i = 0; i < changed.paramCount(); ++i)
  {
    G_ASSERTF(DataBlock::ParamType::TYPE_STRING == changed.getParamType(i), "auto_graphics: only string params are supported");

    const auto name = changed.getParamName(i);
    if (strcmp(changed.getStr(name), curr.getStr(name, "")) != 0)
      changed_fields.addNameId(path_so_far + name);
  }
}

void try_apply_auto_antialiasing_and_resolution_settings()
{
  // requires antialiasing::get_available_methods!

  const auto renderingResolution =
    dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("autoPreset")->getIPoint2("renderingResolution", IPoint2{0, 0});

  const auto currMethod = render::antialiasing::get_method();
  const bool upscalingMethodIsUsed =
    currMethod == render::antialiasing::AntialiasingMethod::DLSS || currMethod == render::antialiasing::AntialiasingMethod::FSR ||
    currMethod == render::antialiasing::AntialiasingMethod::XeSS || currMethod == render::antialiasing::AntialiasingMethod::TSR;

  // true if rendering resolution is significantly lower than screen resolution
  const bool forceUseUpscaler = auto_antialiasing::should_use_upscaling_method(renderingResolution);

  auto antialiasingMethod = render::antialiasing::get_method_name(currMethod);
  if (auto autoAntialiasingMethod = auto_antialiasing::get_auto_selected_method(forceUseUpscaler))
  {
    // update antialiasingMethod if auto method was proposed and we don't already use an upscaling method
    if (!(forceUseUpscaler && upscalingMethodIsUsed))
      antialiasingMethod = autoAntialiasingMethod;
  }

  // autoSettingsBlk is a patch for dgs settings, contains antialiasing_mode and upscaling/fxaaQuality
  const auto autoSettingsBlk = auto_resolution::get_antialiasing_settings(antialiasingMethod, renderingResolution);
  if (autoSettingsBlk.isEmpty())
    return;

  {
    String path;
    fill_changes_name_map(*dgs_get_settings(), autoSettingsBlk, path);
  }

  if (changed_fields.nameCount() == 0)
    return;

  bool resetRequired = false;
  if (changed_fields.getNameId("video/antialiasing_mode") >= 0)
  {
    // Some methods are initialized when the driver starts, so changing them requires restarting the driver.
    // render::antialising dummy only works with available AA methods (regardless of whether they are ready),
    // so we need to disable/enable them manually.

    using AntialiasingMethod = render::antialiasing::AntialiasingMethod;

    const auto desiredMethod =
      render::antialiasing::get_method_from_name(autoSettingsBlk.getBlockByNameEx("video")->getStr("antialiasing_mode"));

    if (currMethod != desiredMethod)
    {
      const auto initializedOnDriverInit = [](AntialiasingMethod method) {
        switch (method)
        {
          case AntialiasingMethod::DLSS: [[fallthrough]];
          case AntialiasingMethod::XeSS: [[fallthrough]];
          case AntialiasingMethod::FSR: return true;
          default: return false;
        }
      };

      resetRequired = initializedOnDriverInit(currMethod) || initializedOnDriverInit(desiredMethod);
    }
  }

  // update settings, they will be used by real render::antialising manager later (and it expects the selected method to be ready!)
  merge_data_block(*get_settings_override_blk(), autoSettingsBlk);
  save_settings(nullptr, /*apply_settings*/ true);

  if (resetRequired)
  {
    execute_delayed_action_on_main_thread(make_delayed_action([] {
      bool _ = false;
      change_driver_reset_request(_, /* mode_reset */ true);
    }));
  }
}

// If you are loading the game without menu, world renderer may be initialized before device reset.
// This means world renderer's AA state (e.g. FG nodes for dlss) will be initialized for old AA mode.
// We need to notify world renderer about AA mode change after device reset.
static void after_device_reset(bool full_reset)
{
  // TODO: maybe we should skip this call on full reset?
  G_UNUSED(full_reset);

  const bool ignore = changed_fields.nameCount() == 0;
  if (ignore)
    return;

  if (get_world_renderer())
    get_world_renderer()->onSettingsChanged(changed_fields, /* apply_after_reset */ true);
  /* else world renderer will be initialized after device reset, nothing to do */

  // ignore further calls
  changed_fields.clear();
}

REGISTER_D3D_AFTER_RESET_FUNC(after_device_reset);

} // namespace auto_graphics::startup
