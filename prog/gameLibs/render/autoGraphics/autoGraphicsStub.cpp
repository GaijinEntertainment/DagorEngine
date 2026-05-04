// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/autoGraphics.h>
#include <ioSys/dag_dataBlock.h>

namespace auto_graphics
{
namespace auto_antialiasing
{
const char *get_auto_selected_method(bool /* force_enable_and_use_upscaler */) { return nullptr; }
bool should_use_upscaling_method(IPoint2 /* rendering_resolution */) { return false; }
} // namespace auto_antialiasing

namespace auto_resolution
{
DataBlock get_antialiasing_settings(const char * /* antialiasing_method */, IPoint2 /* rendering_resolution */) { return {}; }
} // namespace auto_resolution

namespace auto_quality_preset
{
AutoPreset get_auto_preset() { return eastl::nullopt; }
bool is_valid() { return false; }
dag::Vector<PresetData> get_preset_for_each_available_target_fps() { return {}; }
} // namespace auto_quality_preset
} // namespace auto_graphics
