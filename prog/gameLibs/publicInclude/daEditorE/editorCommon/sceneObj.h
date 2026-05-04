//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/de_object.h>
#include <daECS/scene/scene.h>
#include <EASTL/memory.h>
#include <ioSys/dag_dataBlock.h>

static constexpr unsigned CID_SceneObj = 0x1D62D5D7u; // SceneObj

class SceneObj : public EditableObject
{
public:
  EO_IMPLEMENT_RTTI(CID_SceneObj, EditableObject)

  explicit SceneObj(ecs::Scene::SceneId scene_id);

  ecs::Scene::SceneId getSceneId() const { return sceneId; }

  bool isValid() const override;
  bool isSelectedByPointClick([[maybe_unused]] int x, [[maybe_unused]] int y) const override { return false; }
  bool isSelectedByRectangle([[maybe_unused]] const IBBox2 &rect) const override { return false; }

  bool setPos(const Point3 &p) override;
  void setWtm(const TMatrix &wtm) override;

  void update(float dt) override;
  void beforeRender() override;
  void render() override;
  void renderTrans() override;

  bool getWorldBox(BBox3 &box) const override;
  BBox3 getLbb() const override;

  bool mayDelete() override;
  bool mayRename() override { return false; }

  void onRemove(ObjectEditor *editor) override;
  void onAdd(ObjectEditor *editor) override;

private:
  void updateSceneTransform();
  bool canBeMoved() const;
  void init();

  struct UndoData
  {
    ecs::Scene::SceneId parentId = ecs::Scene::C_INVALID_SCENE_ID;
    eastl::string path;
    uint32_t order = ecs::Scene::C_INVALID_SCENE_ID;
    DataBlock sceneData;
  };

  eastl::unique_ptr<UndoData> undoData;

  ecs::Scene::SceneId sceneId;
};