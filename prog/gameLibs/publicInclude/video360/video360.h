//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <3d/dag_texMgr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>

class PostFxRenderer;
class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
struct DagorCurView;
struct Driver3dPerspective;

struct CameraSetupPerspPair
{
  CameraSetup camera;
  Driver3dPerspective persp;
};

class Video360
{
public:
  Video360(int cube_size, int convergence_frames, float z_near, float z_far, TMatrix view_itm);
  void update(ManagedTexView tex);
  bool isFinished();
  eastl::optional<CameraSetupPerspPair> getCamera() const;
  dag::Span<UniqueTex> getFaces();
  int getCubeSize();
  void renderSphericalProjection();

private:
  void copyFrame(TEXTUREID renderTargetTexId, int cube_face);

  int cubemapFace;
  int stabilityFrameIndex;

  TMatrix savedCameraTm;

  PostFxRenderer copyTargetRenderer;
  PostFxRenderer cubemapToSphericalProjectionRenderer;

  float zNear;
  float zFar;
  int cubeSize;
  int convergenceFrames;

  UniqueTexHolder envCubeTex;
  UniqueTex faces[6];
};
