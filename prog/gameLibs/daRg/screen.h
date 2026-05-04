// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "elementTree.h"
#include "renderList.h"
#include "inputStack.h"

#include <EASTL/vector.h>


namespace darg
{

class Panel;
class GuiScene;


struct DebugRenderBox
{
  BBox2 box;
  E3DCOLOR fillColor, borderColor;
  float lifeTime, timeLeft;
};


class Screen
{
private:
  Screen(const Screen &) = delete;
  Screen &operator=(const Screen &) = delete;
  Screen(Screen &&) = delete;
  Screen &operator=(Screen &&) = delete;

protected:
  friend class GuiScene;

  Screen(GuiScene *gui_scene);
  ~Screen() { clear(); }
  void clear();

public:
  ElementTree etree;
  RenderList renderList;
  InputStack inputStack;
  InputStack cursorStack;
  InputStack eventHandlersStack;
  Panel *panel = nullptr;

  bool isMainScreen = false;

  ElementTree *getElementTree() { return &etree; }
  InputStack &getInputStack() { return inputStack; }

  void rebuildXmb();
  void rebuildStacks();

  bool localToDisplay(const Point2 &panel_pos, const TMatrix &camera_tm, const Frustum &camera_frustum, Point2 &out_display_pos) const;
  bool displayToLocal(const Point2 &display_pos, const TMatrix &camera_tm, const Frustum &camera_frustum, Point2 &out_panel_pos) const;
  IPoint2 getCanvasSize() const;

  void spawnDebugRenderBox(const BBox2 &box, E3DCOLOR fillColor, E3DCOLOR borederColor, float life_time);
  void updateDebugRenderBoxes(float dt);

  void renderDebugInfo();

private:
  void renderXmbDebug();
  void renderDirPadNavDebug();
  void renderInputStackDebug();
  void drawDebugRenderBoxes();

private:
  eastl::vector<DebugRenderBox> debugRenderBoxes;
};

} // namespace darg
