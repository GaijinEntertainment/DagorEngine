//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/unique_ptr.h>
#include <ecs/camera/getActiveCameraSetup.h>

class PostFxRenderer;
class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
struct DagorCurView;
struct Driver3dPerspective;

class Video360
{
public:
  Video360();

  void init(int cube_size);

  // enabled state means that we are ready to write 360 video or make screenshot
  void enable(bool enable_video360);
  bool isEnabled();

  // active state means that we are writing, processing frame now
  void activate(float z_near, float z_far, Texture *render_target_tex, TEXTUREID render_target_tex_id, TMatrix view_itm);
  bool isActive();

  void beginFrameRender(int frame_id);
  void endFrameRender(int frame_id);
  void renderResultOnScreen();
  void finishRendering();

  bool getCamera(DagorCurView &cur_view, Driver3dPerspective &persp);
  bool getCamera(CameraSetup &cam, Driver3dPerspective &persp);
  bool useFixedDt();
  float getFixedDt();
  int getCubeSize();

private:
  void copyFrame(int cube_face);
  void renderSphericalProjection();

  bool enabled;
  bool captureFrame;
  int frameIndex;
  int fixedFramerate;

  TMatrix savedCameraTm;

  TextureIDHolderWithVar envCubeTex;

  eastl::unique_ptr<PostFxRenderer> copyTargetRenderer;
  eastl::unique_ptr<PostFxRenderer> cubemapToSphericalProjectionRenderer;

  float zNear;
  float zFar;
  int cubeSize;
  TMatrix curViewItm;

  Texture *renderTargetTex;
  TEXTUREID renderTargetTexId;
};
