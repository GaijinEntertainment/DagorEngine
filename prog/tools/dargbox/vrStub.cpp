#include "vr.h"
#include "vrInput.h"

void vr::apply_all_user_preferences() {}
void vr::destroy() {}
bool vr::is_configured() { return false; }
bool vr::is_enabled_for_this_frame() { return false; }

Driver3dPerspective vr::get_projection(StereoIndex index) { return {}; }

void vr::begin_frame() {}
void vr::acquire_frame_data(CameraSetup &) {}
void vr::finish_frame() {}

Texture *vr::get_frame_target(StereoIndex) { return nullptr; }
Texture *vr::get_gui_texture() { return nullptr; }
void vr::render_panels_and_gui_surface_if_needed(darg::IGuiScene &, int) {}
void vr::tear_down_gui_surface() {}

void vr::handle_controller_input() {}
void vr::render_controller_poses() {}
TMatrix vr::get_aim_pose(int) { return TMatrix::IDENT; }
TMatrix vr::get_grip_pose(int) { return TMatrix::IDENT; }
