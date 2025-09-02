// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vr.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_stereoIndex.h>
#include <shaders/dag_shaderBlock.h>
#include <vr/vrGuiSurface.h>
#include <daRg/dag_panelRenderer.h>

namespace vr
{


constexpr int view_config_count = 3;

static VRDevice::FrameData view_configs[view_config_count];
static int active_view_config = 0;
static bool enabled_for_this_frame = false;

static bool gui_surface_initialized = false;

static UniqueTex gui_texture;


VRDevice::FrameData::ViewData &get_view(StereoIndex index)
{
  if (index == StereoIndex::Left)
    return view_configs[active_view_config].views[0];
  else if (index == StereoIndex::Right)
    return view_configs[active_view_config].views[1];

  G_ASSERT(index == StereoIndex::Mono);
  return view_configs[active_view_config].boundingView;
}

void apply_all_user_preferences() { VRDevice::applyAllFromSettings(); }

void begin_frame() { VRDevice::getInstance()->beginFrame(view_configs[active_view_config]); }

void destroy() { VRDevice::deleteInstance(); }

void acquire_frame_data(CameraSetup &cam)
{
  if (VRDevice *vrDevice = VRDevice::getInstance())
  {
    auto dummyCallback = [](VRDevice::Session) { return true; };
    vrDevice->tick(dummyCallback);
    if (!enabled_for_this_frame)
    {
      active_view_config = (active_view_config + 1) % view_config_count;
      enabled_for_this_frame = vrDevice->prepareFrame(view_configs[active_view_config], cam.znear, cam.zfar);
    }

    if (enabled_for_this_frame)
    {
      auto &vrViewConfig = view_configs[active_view_config];

      TMatrix4D camTransformD = cam.transform;
      camTransformD.setrow(3, cam.accuratePos.x, cam.accuratePos.y, cam.accuratePos.z, 1);

      auto setUpView = [&camTransformD](VRDevice::FrameData::ViewData &view) {
        view.cameraTransform *= camTransformD;
        view.viewTransform = orthonormalized_inverse(view.cameraTransform);
      };

      for (int viewIx = 0; viewIx < 2; ++viewIx)
        setUpView(vrViewConfig.views[viewIx]);

      setUpView(vrViewConfig.boundingView);

      cam.transform = TMatrix(vrViewConfig.boundingView.cameraTransform);
      cam.accuratePos.x = vrViewConfig.boundingView.cameraTransform._41;
      cam.accuratePos.y = vrViewConfig.boundingView.cameraTransform._42;
      cam.accuratePos.z = vrViewConfig.boundingView.cameraTransform._43;
    }
  }
}

void finish_frame()
{
  if (enabled_for_this_frame)
  {
    VRDevice::getInstance()->renderMirror(view_configs[active_view_config]);

    auto &vc = view_configs[active_view_config];
    BaseTexture *sa[] = {vc.frameTargets[0].getBaseTex(), vc.frameTargets[1].getBaseTex(), vc.frameDepths[0].getBaseTex(),
      vc.frameDepths[1].getBaseTex()};
    d3d::driver_command(Drv3dCommand::PREPARE_TEXTURES_FOR_VR_CONSUMPTION, sa, (void *)4);
  }

  enabled_for_this_frame = false;
}

TMatrix4 get_projection_matrix(StereoIndex index)
{
  auto &persp = get_view(index).projection;
  TMatrix4 projTm = matrix_perspective_reverse(persp.wk, persp.hk, persp.zn, persp.zf);
  projTm(2, 0) += persp.ox;
  projTm(2, 1) += persp.oy;

  return projTm;
}

Driver3dPerspective get_projection(StereoIndex index) { return get_view(index).projection; }

void get_view_resolution(int &width, int &height)
{
  if (auto instance = VRDevice::getInstance())
    instance->getViewResolution(width, height);
}

bool is_configured() { return !!VRDevice::getInstance(); }

bool is_enabled_for_this_frame() { return enabled_for_this_frame; }

void render_panels_and_gui_surface_if_needed(darg::IGuiScene &gui_scene, int global_frame)
{
  if (enabled_for_this_frame)
  {
    auto &vrViewConfig = view_configs[active_view_config];

    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    TMatrix rotYTm = TMatrix::IDENT;
    d3d::settm(TM_WORLD, rotYTm);

    for (int viewIx = 0; viewIx < 2; ++viewIx)
    {
      auto &view = vrViewConfig.views[viewIx];

      d3d::set_render_target(vrViewConfig.frameTargets[viewIx].getBaseTex(), 0);

      auto persp = view.projection;
      persp.zn = 1;
      persp.zf = 100;
      d3d::setpersp(persp);

      TMatrix viewOrigin;
      viewOrigin.makeTM(view.orientation);
      viewOrigin.setcol(3, view.position);

      TMatrix viewNew = orthonormalized_inverse(viewOrigin);
      d3d::settm(TM_VIEW, viewNew);

      ShaderGlobal::setBlock(global_frame, ShaderGlobal::LAYER_FRAME);
      darg_panel_renderer::render_panels_in_world(gui_scene, darg_panel_renderer::RenderPass::Translucent, view.position, viewNew);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

      TMatrix vrOrientation;
      vrOrientation.rotyTM(0.5f * PI);
      vrOrientation = vrOrientation * viewOrigin;

      d3d::settm(TM_VIEW, orthonormalized_inverse(vrOrientation));

      vrgui::render_to_surface(gui_texture.getTexId());
    }
  }
}

Texture *get_gui_texture()
{
  if (VRDevice::getInstance() && !gui_surface_initialized)
  {
    int width, height;
    d3d::get_screen_size(width, height);
    // width = 1920;
    // height = 1080;
    // ^ using non-screen resolution breaks surface positioning
    G_VERIFY(vrgui::init_surface(width, height));

    gui_texture = dag::create_tex(nullptr, width, height, TEXCF_RTARGET, 1, "VRGuiTarget");

    gui_surface_initialized = true;
  }

  return gui_texture.getTex2D();
}

Texture *get_frame_target(StereoIndex index)
{
  G_ASSERT(index == StereoIndex::Left || index == StereoIndex::Right);
  return view_configs[active_view_config].frameTargets[index == StereoIndex::Right ? 1 : 0].getBaseTex();
}

void tear_down_gui_surface()
{
  if (gui_surface_initialized)
  {
    vrgui::destroy_surface();
    gui_texture.close();
    gui_surface_initialized = false;
  }
}


} // namespace vr
