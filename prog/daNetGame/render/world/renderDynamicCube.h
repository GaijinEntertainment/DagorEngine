// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>


class DeferredRenderTarget;

class RenderDynamicCube
{
  eastl::unique_ptr<DeferredRenderTarget> target;
  UniqueTex shadedTarget;
  PostFxRenderer copy;

public:
  enum class RenderMode
  {
    ONLY_SKY,
    WHOLE_SCENE
  };

  RenderDynamicCube();
  ~RenderDynamicCube();
  void close();

  void init(int cube_size);
  TMatrix4_vec4 getGlobTmForFace(int face_num, const Point3 &pos);
  void update(const ManagedTex *cubeTarget, const Point3 &pos, int face_num, RenderMode mode);
};
