//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaders.h>

#include <3d/dag_sbufferIDHolder.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <hash/wyhash.h>
#include <debug/dag_debug3dStates.h>
#include <util/dag_simpleString.h>


class DebugPrimitivesVbuffer
{
public:
  DebugPrimitivesVbuffer(const char *name);
  ~DebugPrimitivesVbuffer();

  int addVertex(const Point3 &p);
  void addLine(const Point3 &p0, const Point3 &p1, E3DCOLOR c);
  void addBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color);
  void addBox(const BBox3 &box, E3DCOLOR color);
  void addSolidBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color);
  void addTriangle(const Point3 p[3], E3DCOLOR color);
  void invalidate();
  void beginCache();
  void endCache();
  void renderOverrideColor(Color4 color_multiplier);
  void render();
  void renderEx(bool z_test, bool z_write, bool z_func_less, Color4 color_multiplier);
  void setTm(const TMatrix &tm);

  inline bool isValid() { return vbuffer != nullptr; }

private:
  void renderEx(const E3DCOLOR *color);
  SimpleString name;

  TMatrix tm;

  Sbuffer *vbuffer = nullptr;
  Sbuffer *ibuffer = nullptr;

  Ptr<ShaderMaterial> shaderMat;
  Ptr<ShaderElement> shaderElem;
  debug3d::CachedStates renderStates;

  struct Pass
  {
    E3DCOLOR color;
    int firstPrimitive;
    int lastPrimitive;
  };
  Tab<Pass> linesPasses, trianglesPasses;

  struct Line
  {
    int id0;
    int id1;
    E3DCOLOR c;
  };

  struct Triangle
  {
    int id0;
    int id1;
    int id2;
    E3DCOLOR c;
  };

  Tab<Line> linesCache;
  Tab<Triangle> trianglesCache;
  int trianglesStartIndex = 0;

  struct VertexHasher
  {
    size_t __forceinline operator()(const Point3 &p) const { return wyhash(&p.x, sizeof(Point3), 0); }
  };

  ska::flat_hash_map<Point3, int, VertexHasher> vertexMap;
  Tab<Point3> vertexList;
};
