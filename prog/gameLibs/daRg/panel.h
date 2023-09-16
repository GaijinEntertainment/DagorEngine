#pragma once

#include "elementTree.h"
#include "renderList.h"
#include "inputStack.h"
#include "touchPointers.h"
#include <3d/dag_textureIDHolder.h>
#include <math/dag_TMatrix4.h>

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

  TEXTUREID pointerTexture[2] = {BAD_TEXTUREID, BAD_TEXTUREID};
  Point2 pointerUVLeftTop[2] = {Point2(-1, -1), Point2(-1, -1)};
  Point2 pointerUVInvSize[2] = {Point2(0, 0), Point2(0, 0)};
  E3DCOLOR pointerColor[2] = {E3DCOLOR(0, 0, 0, 0), E3DCOLOR(0, 0, 0, 0)};

  void resetPointer();
};


struct PanelSpatialInfo
{
  PanelAnchor anchor = PanelAnchor::None;
  PanelRotationConstraint constraint = PanelRotationConstraint::None;
  PanelGeometry geometry = PanelGeometry::None;

  bool canBePointedAt = false;
  bool canBeTouched = false;
  bool visible = false;

  Point3 position = Point3(0, 0, 0);
  Point3 angles = Point3(0, 0, 0);
  Point3 size = Point3(0, 0, 0);
  uint32_t anchorEntityId = 0;
  uint32_t facingEntityId = 0;

  mutable TMatrix lastTransform = TMatrix::IDENT;

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
public:
  Panel(GuiScene *scene_);
  ~Panel();

  void init(const Sqrat::Object &markup);

  void rebuildStacks();
  void clear();
  void update(float dt);
  void updateSpatialInfoFromScript();
  void updateRenderInfoParamsFromScript();
  void updateHover();
  void updateActiveCursors();
  void onRemoveCursor(Cursor *cursor);

public:
  GuiScene *scene = nullptr;
  ElementTree etree;
  RenderList renderList;
  InputStack inputStack;
  InputStack cursorStack;

  PanelSpatialInfo spatialInfo;
  PanelRenderInfo renderInfo;

  static constexpr int MAX_POINTERS = 2;

  PanelPointer pointers[MAX_POINTERS];
  TouchPointers touchPointers;
};


struct PanelData
{
  eastl::unique_ptr<Panel> panel;
  eastl::unique_ptr<TextureIDHolder> canvas;
  TEXTUREID pointerTex = BAD_TEXTUREID;
  int index = -1;
  eastl::string pointerPath;

  bool isDirty = true;

  void init(GuiScene &scene, const Sqrat::Object &object, int panelIndex);
  void close();
  bool isPanelInited() const { return !!panel; }

  bool needRenderInWorld() const;
  bool getCanvasSize(IPoint2 &size) const;
  void syncCanvas();

  void updateTexture(GuiScene &scene);

  eastl::string getPointerPath() const;
  Point2 getPointerSize() const;
  E3DCOLOR getPointerColor() const;
  bool isAutoUpdated() const;

  bool isInThisPass(darg_panel_renderer::RenderPass render_pass) const;

  void setCursorStatus(int hand, bool enabled);
};

} // namespace darg
