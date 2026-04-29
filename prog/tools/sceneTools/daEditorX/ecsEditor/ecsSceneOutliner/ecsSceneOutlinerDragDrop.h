// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/scene/scene.h>
#include <propPanel/control/dragAndDropHandler.h>
#include "ecsSceneOutlinerPanel.h"

class ECSSceneOutlinerPanel::DragHandler final : public PropPanel::ITreeDragHandler
{
public:
  explicit DragHandler(ECSSceneOutlinerPanel &owner) noexcept;
  void onBeginDrag(PropPanel::TLeafHandle leaf) override;

private:
  DragAndDropPayload collectPayload(PropPanel::TLeafHandle current_leaf);
  bool verifyPayload(const DragAndDropPayload &payload);

  ECSSceneOutlinerPanel &owner;
};

class ECSSceneOutlinerPanel::DropHandler final : public PropPanel::ITreeDropHandler
{
public:
  explicit DropHandler(ECSSceneOutlinerPanel &owner) noexcept;

  PropPanel::DragAndDropResult onDropTargetDirect(PropPanel::TLeafHandle leaf) override;
  PropPanel::DragAndDropResult onDropTargetBetween(PropPanel::TLeafHandle leaf, int idx) override;

  bool canDropBetween() override;

private:
  bool canSceneBeReparented(ecs::Scene::SceneId source_id, ecs::Scene::SceneId dest_id) const;
  bool verifyPayloadDirect(const DragAndDropPayload &payload, PropPanel::TLeafHandle leaf);
  bool verifyPayloadBetween(const DragAndDropPayload &payload, PropPanel::TLeafHandle leaf);

  ECSSceneOutlinerPanel &owner;
};
