// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "elementTree.h"
#include "renderList.h"
#include "inputStack.h"
#include "touchPointers.h"
#include <daRg/dag_guiScene.h>
#include <3d/dag_textureIDHolder.h>
#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>

namespace darg_panel_renderer
{
enum class RenderPass : int;
}

namespace darg
{

class Cursor;


enum class PanelAnchor
{
  None = -1,
  Scene,
  VRSpace,
  Head,
  LeftHand,
  RightHand,
  Entity,
};

enum class PanelRotationConstraint
{
  None = 0,
  FaceLeftHand,
  FaceRightHand,
  FaceHead,
  FaceHeadLockY,
  FaceEntity,
  FaceHeadLockPlayspaceY,
};

enum class PanelGeometry
{
  None = -1,
  Rectangle,
};

struct PanelRenderInfo
{
  bool isValid = false;
  TMatrix transform = TMatrix::IDENT;
  TEXTUREID texture = BAD_TEXTUREID;

  static constexpr float def_brightness = 0.05f;
  static constexpr float def_smoothness = 0.172f;
  static constexpr float def_reflectance = 0.498f;
  static constexpr float def_metalness = 0;

  // The default is not 1 here, because dagor scenes are quite dark before
  // tone mapping, and by default, we are adapting for that.
  float brightness = def_brightness;
  float smoothness = def_smoothness;
  float reflectance = def_reflectance;
  float metalness = def_metalness;
  int worldRenderFeatures = 0;
};


struct PanelSpatialInfo
{
  PanelAnchor anchor = PanelAnchor::None;
  PanelRotationConstraint constraint = PanelRotationConstraint::None;
  PanelGeometry geometry = PanelGeometry::None;

  bool canBePointedAt = false;
  bool canBeTouched = false;
  bool visible = false;
  bool renderRedirection = false;
  bool shouldBlockVrHandInteractions = false;
  bool allowDisplayCursorProjection = false;

  Point3 position = Point3(0, 0, 0);
  Point3 angles = Point3(0, 0, 0);
  Point3 size = Point3(0, 0, 0);
  float headDirOffset = 0.f;
  uint32_t anchorEntityId = 0;
  uint32_t facingEntityId = 0;

  mutable ViewDependentResource<eastl::optional<TMatrix>, 2> lastTransform;

  eastl::string anchorNodeName;
};


struct PanelPointer
{
  bool isPresent = false;
  Point2 pos = Point2(-1, -1);
  Cursor *cursor = nullptr;
};


class Panel
{
private:
  Panel(const Panel &) = delete;
  Panel &operator=(const Panel &) = delete;
  Panel(Panel &&) = delete;
  Panel &operator=(Panel &&) = delete;

public:
  Panel(GuiScene &scene, const Sqrat::Object &object, int panelIndex);
  ~Panel();

  void rebuildStacks();
  void clear();
  void update(float dt);
  void updatePanelParamsFromScript(const Sqrat::Table &desc);
  void updateHover();
  void updateActiveCursors();
  bool hasActiveCursors() const;
  void onRemoveCursor(Cursor *cursor);
  void setCursorState(int hand, bool enabled, Point2 pos = {-100, -100});

  bool needRenderInWorld() const;
  bool getCanvasSize(IPoint2 &size) const;
  void syncCanvas();
  void updateTexture(GuiScene &gui_scene, BaseTexture *target);
  bool isAutoUpdated() const;
  bool isInThisPass(darg_panel_renderer::RenderPass render_pass) const;

private:
  void init(int screen_id, const Sqrat::Object &markup);

public:
  GuiScene *scene = nullptr;
  Screen *screen = nullptr;

  PanelSpatialInfo spatialInfo;
  PanelRenderInfo renderInfo;

  static constexpr int MAX_POINTERS = IGuiScene::SpatialSceneData::AimOrigin::Total;

  PanelPointer pointers[MAX_POINTERS];
  TouchPointers touchPointers;

  eastl::unique_ptr<TextureIDHolder> canvas;
  int index = -1;
  bool isDirty = true;
};

} // namespace darg
