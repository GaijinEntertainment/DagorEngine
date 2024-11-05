// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class ShaderMaterial;
class Sbuffer;
typedef Sbuffer Ibuffer;
typedef Sbuffer Vbuffer;
class Point2;
namespace dynrender
{
struct RElem;
};

class GridRender
{
public:
  GridRender() : gridMaterial(NULL), gridRendElem(NULL), gridVb(NULL), gridIb(NULL) {}
  ~GridRender() { close(); }
  bool init(const char *shader_name, float ht, int subdiv, const Point2 &min, const Point2 &max);
  void close();
  void render();

protected:
  ShaderMaterial *gridMaterial;
  dynrender::RElem *gridRendElem;
  Vbuffer *gridVb;
  Ibuffer *gridIb;
};
