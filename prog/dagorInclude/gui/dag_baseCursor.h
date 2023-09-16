//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <3d/dag_texMgr.h>
#include <gui/dag_guiGlobNameMap.h>

class GuiScreen;

class IGenGuiCursor
{
public:
  virtual ~IGenGuiCursor() {}
  virtual void render(const Point2 &at_pos, GuiScreen *screen = NULL) = 0;
  virtual void timeStep(real dt) = 0;

  virtual void setMode(const GuiNameId &mode_name) = 0;

  virtual void setVisible(bool vis) = 0;
  virtual bool getVisible() const = 0;

  virtual bool setModeCursor(const GuiNameId &mode_name, float w, float h, float hotspotx, float hotspoty, const char *texfname) = 0;
  virtual bool setModeCursor(const GuiNameId &mode_name, float w, float h, float hotspotx, float hotspoty, TEXTUREID _tid) = 0;
};


// main gui cursor object; must be used for rendering and for hide/show
extern IGenGuiCursor *gui_cursor;

// creates simple cursor capable of handling setModeCursor and rendering textured quad
IGenGuiCursor *create_gui_cursor_lite();
