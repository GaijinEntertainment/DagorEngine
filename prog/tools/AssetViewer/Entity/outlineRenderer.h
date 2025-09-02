// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <generic/dag_span.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/vector_multimap.h>

class DynamicRenderableSceneInstance;
class IGenViewportWnd;
struct RiGenVisibility;

class OutlineRenderer
{
public:
  using RIElementsCache = eastl::vector_multimap<int, TMatrix>;

  ~OutlineRenderer();

  void render(IGenViewportWnd &wnd, const RIElementsCache &riElements,
    dag::ConstSpan<DynamicRenderableSceneInstance *> dynmodelElements);

private:
  void init();
  bool isInited() const { return globalVisibility != nullptr; }
  void initResolution(int width_, int height_);

  int width = 0;
  int height = 0;
  PostFxRenderer finalRender;
  TextureIDHolderWithVar colorRt;

  RiGenVisibility *globalVisibility = nullptr;
  RiGenVisibility *filteredVisibility = nullptr;

  static int simple_outline_colorVarId;
  static int simple_outline_color_rtVarId;
  static int global_frame_block_id;
  static int rendinst_scene_block_id;
  static const E3DCOLOR outline_color;
};
