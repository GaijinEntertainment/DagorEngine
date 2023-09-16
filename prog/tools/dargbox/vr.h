#pragma once

#include <math/dag_TMatrix4.h>
#include <drivers/dag_vr.h>
#include <EASTL/optional.h>
#include <3d/dag_stereoIndex.h>

namespace darg
{
struct IGuiScene;
}

enum FovMode
{
  EFM_HOR_PLUS,
  EFM_HYBRID
};

struct CameraSetup
{
  TMatrix transform = TMatrix::IDENT;
  DPoint3 accuratePos = {0, 0, 0};

  float fov = 90.f;
  float znear = 0.1f;
  float zfar = 5000.f;

  FovMode fovMode = EFM_HOR_PLUS;
};


namespace vr
{
void apply_all_user_preferences();

void get_view_resolution(int &width, int &height);

VRDevice::FrameData::ViewData &get_view(StereoIndex index);

bool is_configured();
bool is_enabled_for_this_frame();

TMatrix4 get_projection_matrix(StereoIndex index);
Driver3dPerspective get_projection(StereoIndex index);

void begin_frame();
void acquire_frame_data(CameraSetup &cam);
void finish_frame();

Texture *get_frame_target(StereoIndex index);

Texture *get_gui_texture();
void render_panels_and_gui_surface_if_needed(darg::IGuiScene &gui_scene, int global_frame);
void tear_down_gui_surface();

void destroy();
} // namespace vr
