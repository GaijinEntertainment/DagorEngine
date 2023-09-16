#pragma once
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <dag/dag_vector.h>

class DynamicRenderableSceneInstance;
class IGenViewportWnd;
class IObjEntity;

class PixelPerfectEntitySelector
{
public:
  class Hit
  {
  public:
    IObjEntity *entityForSelection;
    int rendInstExtraResourceIndex;
    DynamicRenderableSceneInstance *sceneInstance;
    TMatrix transform;
    float z;
  };

  void getHitsAt(IGenViewportWnd &wnd, int pickX, int pickY, dag::Vector<Hit> &hits);

private:
  void init();
  bool isInited() const { return rendinstMatrixBuffer.get() != nullptr; }
  bool getHitFromDepthBuffer(float &hitZ);

  static TMatrix4 makeProjectionMatrixForViewRegion(int viewWidth, int viewHeight, float fov, float zNear, float zFar, int regionLeft,
    int regionTop, int regionWidth, int regionHeight);

  TextureIDHolderWithVar depthRt;
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> rendinstMatrixBuffer;

  static int global_frame_block_id;
  static int rendinst_scene_block_id;
};
