// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "elementTree.h"
#include "renderList.h"


namespace darg
{

class Cursor
{
public:
  Cursor(GuiScene *scene_);
  ~Cursor();

  void init(const Sqrat::Object &markup);

  void rebuildStacks();
  void clear();
  void update(float dt);
  void readHotspot();

  static void bind_script(Sqrat::Table &exports);


public:
  GuiScene *scene = nullptr;
  ElementTree etree;
  RenderList renderList;
  Point2 hotspot;
};

} // namespace darg
