// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/scene/scene.h>
#include <EditorCore/ec_rendEdObject.h>
#include <EASTL/memory.h>
#include <ioSys/dag_dataBlock.h>

static const int CID_ECSSceneObject = 0xDAC17B47u; // ECSSceneObject

class ECSSceneObject : public RenderableEditableObject
{
  using Base = RenderableEditableObject;

public:
  EO_IMPLEMENT_RTTI_EX(CID_ECSSceneObject, RenderableEditableObject)

  explicit ECSSceneObject(ecs::Scene::SceneId scene_id);

  ecs::Scene::SceneId getSceneId() const { return sceneId; }

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;

  bool setPos(const Point3 &p) override;
  void setWtm(const TMatrix &wtm) override;

  void update(float dt) override;
  void beforeRender() override;
  void render() override;
  void renderTrans() override;

  bool getWorldBox(BBox3 &box) const override;

  bool mayDelete() override;
  bool mayRename() override { return false; }

  void onRemove(ObjectEditor *editor) override;
  void onAdd(ObjectEditor *editor) override;

  void hideObject(bool hide = true) override;
  void lockObject(bool lock = true) override;

  void createEntityObjects(ObjectEditor *editor);

private:
  void updateSceneTransform();
  bool canBeMoved() const;
  void init();

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  struct UndoData
  {
    ecs::Scene::SceneId parentId = ecs::Scene::C_INVALID_SCENE_ID;
    eastl::string path;
    uint32_t order = ecs::Scene::C_INVALID_SCENE_ID;
    DataBlock sceneData;
  };

  eastl::unique_ptr<UndoData> undoData;

  ecs::Scene::SceneId sceneId;
  bool addEntities = false;
};
