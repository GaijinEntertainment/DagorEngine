// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiBindings.h"

#include "bhv/bhvZonePointers.h"
#include "bhv/bhvProjection.h"
#include "bhv/bhvDistToPriority.h"
#include "bhv/bhvPlaceOnCompassStrip.h"
#include "bhv/bhvPlaceOnRoundedCompassStrip.h"
#include "bhv/bhvDistToEntity.h"
#include "bhv/bhvDistToBorder.h"
#include "bhv/bhvDistToSphere.h"
#include "bhv/bhvTouchScreenButton.h"
#include "bhv/bhvElementEditor.h"
#include "bhv/bhvTouchScreenHoverButton.h"
#include "bhv/bhvTouchScreenStick.h"
#include "bhv/BhvRotateRelativeToDir.h"
#include "bhv/BhvRotateByComponent.h"
#include "bhv/bhvOpacityByComponent.h"
#include "bhv/bhvMenuCameraControl.h"
#include "bhv/bhvActivateActionSet.h"
#include "bhv/bhvReplayFreeCameraControl.h"
#include "bhv/bhvViewAzimuth.h"
#include "bhv/bhvUiStateControl.h"
#include <sqModules/sqModules.h>


namespace ui
{

void bind_ui_behaviors(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::ConstTable constTbl(vm);
  constTbl.Const("S_CLIPPED", S_CLIPPED);


#define REG_BHV_DATA(name) Sqrat::Class<name, Sqrat::NoConstructor<name>> sq##name(vm, #name);
  REG_BHV_DATA(TouchStickState)
  REG_BHV_DATA(TouchButtonState)
  REG_BHV_DATA(MenuCameraControlData)
  REG_BHV_DATA(ElementEditorState)
  REG_BHV_DATA(BhvUiStateControlData)
  REG_BHV_DATA(BhvDistToEntityData)
#undef REG_BHV_DATA

  Sqrat::Table tblBhv(vm);
  tblBhv //
    .SetValue("ZonePointers", (darg::Behavior *)&bhv_zone_pointers)
    .SetValue("Projection", (darg::Behavior *)&bhv_projection)
    .SetValue("DistToPriority", (darg::Behavior *)&bhv_dist_to_priority)
    .SetValue("PlaceOnCompassStrip", (darg::Behavior *)&bhv_place_on_compass_strip)
    .SetValue("PlaceOnRoundCompassStrip", (darg::Behavior *)&bhv_place_on_round_compass_strip)
    .SetValue("DistToEntity", (darg::Behavior *)&bhv_dist_to_entity)
    .SetValue("DistToBorder", (darg::Behavior *)&bhv_dist_to_border)
    .SetValue("DistToSphere", (darg::Behavior *)&bhv_dist_to_sphere)
    .SetValue("TouchScreenHoverButton", (darg::Behavior *)&bhv_touch_screen_hover_button)
    .SetValue("TouchScreenButton", (darg::Behavior *)&bhv_touch_screen_button)
    .SetValue("ElementEditor", (darg::Behavior *)&bhv_element_editor)
    .SetValue("TouchScreenStick", (darg::Behavior *)&bhv_touch_screen_stick)
    .SetValue("RotateRelativeToDir", (darg::Behavior *)&bhv_rotate_relative_to_dir)
    .SetValue("RotateByComponent", (darg::Behavior *)&bhv_rotate_by_component)
    .SetValue("OpacityByComponent", (darg::Behavior *)&bhv_opacity_by_component)
    .SetValue("MenuCameraControl", (darg::Behavior *)&bhv_menu_camera_control)
    .SetValue("ActivateActionSet", (darg::Behavior *)&bhv_activate_action_set)
    .SetValue("ReplayFreeCameraControl", (darg::Behavior *)&bhv_replay_free_camera_control)
    .SetValue("ViewAzimuth", (darg::Behavior *)&bhv_view_azimuth)
    .SetValue("UiStateControl", (darg::Behavior *)&bhv_ui_state_control)
    /**/;

#define CONST(x) tblBhv.SetValue(#x, x)

  CONST(MARKER_SHOW_ONLY_IN_VIEWPORT);
  CONST(MARKER_SHOW_ONLY_WHEN_CLAMPED);
  CONST(MARKER_ARROW);
  CONST(MARKER_KEEP_SCALE);

#undef CONST

  module_mgr->addNativeModule("dng.behaviors", tblBhv);
}

} // namespace ui
