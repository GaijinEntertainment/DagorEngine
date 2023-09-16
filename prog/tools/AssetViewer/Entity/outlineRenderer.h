#pragma once
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <generic/dag_span.h>
#include <shaders/dag_postFxRenderer.h>

class DynamicRenderableSceneInstance;
class IGenViewportWnd;

class OutlineRenderer
{
public:
  class RenderElement
  {
  public:
    TMatrix transform;
    int rendInstExtraResourceIndex;
    DynamicRenderableSceneInstance *sceneInstance;
  };

  void render(IGenViewportWnd &wnd, dag::ConstSpan<RenderElement> elements);

private:
  void init();
  bool isInited() const { return rendinstMatrixBuffer.get() != nullptr; }
  void initResolution(int width_, int height_);

  int width = 0;
  int height = 0;
  PostFxRenderer finalRender;
  TextureIDHolderWithVar colorRt;
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> rendinstMatrixBuffer;

  static int simple_outline_colorVarId;
  static int simple_outline_color_rtVarId;
  static int global_frame_block_id;
  static int rendinst_scene_block_id;
  static const E3DCOLOR outline_color;
};
