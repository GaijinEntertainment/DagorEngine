//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <math/integer/dag_IPoint2.h>

#include <EASTL/optional.h>

class DataBlock;

namespace auto_graphics
{

namespace auto_antialiasing
{
/**
 * \brief Get auto-selected antialiasing method based on hardware
 * \details This method works in two cases:
 *   - on first launch and if auto_antialiasing/enableOnFirstLaunch:b=yes in config
 *   - when force_enable_and_use_upscaling_method is true. This case ignores first launch and config option.
 * By default, only TSR and FXAA are available.
 * FXAA is used only for some iGPU families, see code for default criteria and configuration options. Otherwise, TSR is used.
 * Vendor-specific methods (DLSS, FSR, XeSS) are used if it is allowed in config and method is available for the GPU.
 * \param force_enable_and_use_upscaling_method If true, upscaling method (TSR, DLSS, FSR, XeSS) is selected.
 * \return nullptr if auto selection is disabled, otherwise the name of selected method
 */
const char *get_auto_selected_method(bool force_enable_and_use_upscaling_method);

/**
 * \brief Check if upscaling method should be used based on rendering resolution
 * \param rendering_resolution The rendering resolution selected by auto graphics preset
 * \return true if sqrt(screen_area / rendering_area) >= settings.getReal("auto_antialiasing/scale_factor_threshold")
 */
bool should_use_upscaling_method(IPoint2 rendering_resolution);
} // namespace auto_antialiasing

namespace auto_resolution
{
/**
 * \brief Get antialiasing settings (upscaling or fxaa quality) for current method if resolution is auto
 * or rendering_resolution is used for auto graphics preset.
 * \details This function returns non-empty blk only if resolution is auto and
 * overrideAAForAutoResolution is true in config or "rendering_resolution is used" for auto graphics preset.
 * When overrideAAForAutoResolution is true and resolution is auto:
 *   The upscaling value is based on display resolution and DPI scaling.
 *   If the display area exceeds FullHD by <video/autoResolutionUpscalerThreshold> times,
 *   upscaling value is selected for current AA method via DPI. Otherwise, native resolution is used (upscaling=100).
 *   If fxaa method is selected, fxaa quality is selected instead of upscaling via config value.
 * When "rendering_resolution is used" for auto graphics preset:
 *   The upscaling value is calculated as sqrt(rendering_area / display_area) * 100, rounded to nearest 5.
 * "Rendering resolution is used" means it is non-zero point.
 * \param antialiasing_method The method for which the settings will be returned. Incorrect methods are ignored (returns empty blk).
 * \return non-empty blk with antialiasing settings (with video/antialiasing_mode:t=antialiasing_method).
 * Use this blk as patch to override settings.
 */
//
DataBlock get_antialiasing_settings(const char *antialiasing_method, IPoint2 rendering_resolution);
} // namespace auto_resolution

namespace auto_quality_preset
{
struct PresetData
{
  IPoint2 resolution;
  String preset;
  int targetFps;
};

using AutoPreset = eastl::optional<PresetData>;

/**
 * \brief Get auto graphics preset based on current hardware, driver and display.
 * \details Actually combines user info initialization and auto preset evaluation,
 * because you need to call it only once on startup.
 * User info runs cpu benchmark and reads GPU score and vram size. It may fail to read info for unknown GPU.
 * \return Non-empty PresetData or nullopt
 * if database is empty/user info initialization has failed/auto preset is disabled in config.
 */
AutoPreset get_auto_preset();

/**
 * \brief Check if user info is initialized
 */
bool is_valid();

/**
 * \brief Get auto graphics preset for each available target FPS based on current hardware, driver and display.
 * \details get_auto_preset must be called before to initialize user info.
 * \return Vector of PresetData for each available target FPS or empty vector if user info is not initialized.
 * Target FPSs are defined in database and may differ for different drivers, but usually include 30, 60, 120.
 */
dag::Vector<PresetData> get_preset_for_each_available_target_fps();
} // namespace auto_quality_preset

// project-specific part, call it on startup to apply auto settings and update config
namespace startup
{
void try_apply_auto_antialiasing_and_resolution_settings();
void try_apply_auto_graphics_settings();
} // namespace startup

} // namespace auto_graphics
