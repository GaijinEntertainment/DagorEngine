// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <dag/dag_vector.h>
#include <de3_pixelPerfectSelectionService.h>

class DynamicRenderableSceneInstance;
class IGenViewportWnd;
class IObjEntity;

class PixelPerfectSelection
{
public:
  void getHitsAt(IGenViewportWnd &wnd, int pickX, int pickY, dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

private:
  void init();
  bool isInited() const { return rendinstMatrixBuffer.get() != nullptr; }
  bool getHitFromDepthBuffer(float &hitZ);
  void renderLodResource(const RenderableInstanceLodsResource &riLodResource, const TMatrix &transform);

  static TMatrix4 makeProjectionMatrixForViewRegion(int viewWidth, int viewHeight, float fov, float zNear, float zFar, int regionLeft,
    int regionTop, int regionWidth, int regionHeight);

  TextureIDHolderWithVar depthRt;
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> rendinstMatrixBuffer;

  static int global_frame_block_id;
  static int rendinst_render_pass_var_id;
  static int rendinst_scene_block_id;
};
