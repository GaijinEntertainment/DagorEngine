#pragma once

#include <math/dag_math2d.h>
#include <humanInput/dag_hiDecl.h>

namespace StdGuiRender
{
class GuiContext;
}

namespace darg
{

class PointerPosition
{
public:
  void initMousePos();
  void onMouseMoveEvent(const Point2 &p);
  void setMousePos(const Point2 &p, bool reset_target, bool force_drv_update = false);
  bool moveMouseToTarget(float dt);
  void requestNextFramePos(const Point2 &p);
  void requestTargetPos(const Point2 &p);
  HumanInput::IGenPointing *getMouse();
  void onShutdown();
  void update(float dt);
  void onActivateSceneInput();
  void onAppActivate();
  void debugRender(StdGuiRender::GuiContext *ctx);

private:
  void syncMouseCursorLocation();

public:
  Point2 mousePos = Point2(0, 0), targetMousePos = Point2(0, 0);
  bool isSettingMousePos = false;
  bool mouseWasRelativeOnLastUpdate = false;
  Point2 nextFramePos = Point2(0, 0);
  bool needToMoveOnNextFrame = false;
};

} // namespace darg
