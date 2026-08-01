//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <EASTL/bitset.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/vector_map.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <dag/dag_vector.h>
#include <generic/dag_carray.h>


class ShaderElement;
class ShaderMaterial;

namespace frx
{

struct RenderMaterial;


struct DestrMesh
{
  struct alignas(16) Vertex
  {
    Point3 pos;
    Point2 tc;
    Point3 norm;
  };

  struct alignas(16) Face
  {
    carray<uint32_t, 3> idx;
    uint16_t mat;
  };

  TMatrix tm = TMatrix::IDENT;
  dag::Vector<Face> faces;
  dag::Vector<Vertex> verts;

  void clear()
  {
    faces.clear();
    verts.clear();
  }
};

struct DestrSystem
{
  eastl::vector_map<int, DestrMesh> pieces;
};


// RenderMaterial is incomplete type, declared by render part of the library, so use this helper type to declare custom
// constructor/destructor for it
struct DestrContextRenderMaterialsHolder
{
  dag::Vector<RenderMaterial> renderMats;

  DestrContextRenderMaterialsHolder();
  virtual ~DestrContextRenderMaterialsHolder();
};

struct DestrMaterial
{
  bool isSolid = false;
};

struct DebugDrawContext
{
  // configuration
  bool drawCutFaceBasis = false;
  bool drawCutSegments = false;
  bool drawCutEdgeGraph = false;
  bool drawBoundaryFill = false;
  bool drawBoundaryTriangles = false;

  // state
  TMatrix tm = TMatrix::IDENT;
  int timeout = 10000000;

  void drawLine(Point3 a, Point3 b, E3DCOLOR col = E3DCOLOR(~0u)) const;
  void drawArrow(Point3 a, Point3 b, E3DCOLOR col = E3DCOLOR(~0u)) const;
  void drawPoint(Point3 a, E3DCOLOR col = E3DCOLOR(~0u)) const;
};

struct DestrContext : DestrContextRenderMaterialsHolder
{
  dag::Vector<DestrMaterial> materials;
  DebugDrawContext dbgDraw;
};

} // namespace frx
