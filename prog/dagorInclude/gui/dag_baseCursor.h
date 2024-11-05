//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <3d/dag_texMgr.h>
#include <util/dag_globNameMap.h>

DECLARE_SMART_NAMEID(GuiNameId);

class IGenGuiCursor
{
public:
  virtual ~IGenGuiCursor() {}
  virtual void render(const Point2 &at_pos) = 0;
  virtual void timeStep(real dt) = 0;

  virtual void setMode(const GuiNameId &mode_name) = 0;

  virtual void setVisible(bool vis) = 0;
  virtual bool getVisible() const = 0;

  virtual bool setModeCursor(const GuiNameId &mode_name, float w, float h, float hotspotx, float hotspoty, const char *texfname) = 0;
};


// creates simple cursor capable of handling setModeCursor and rendering textured quad
IGenGuiCursor *create_gui_cursor_lite();
