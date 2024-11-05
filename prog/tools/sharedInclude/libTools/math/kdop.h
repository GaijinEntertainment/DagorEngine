// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <dag/dag_vector.h>
#include <math/integer/dag_IPoint3.h>

enum KdopPreset
{
  SET_EMPTY = -1,
  SET_6DOP,
  SET_14DOP,
  SET_18DOP,
  SET_26DOP,
  SET_KDOP_FIXED_X,
  SET_KDOP_FIXED_Y,
  SET_KDOP_FIXED_Z,
  SET_CUSTOM_KDOP,
};

struct PlaneDefinition
{
  dag::Vector<Point3> vertices;
  dag::Vector<int> indices;
  Point3 planeNormal;
  float limit;
  float area;

  PlaneDefinition(const Point3 &plane_normal);
};

class Kdop
{
public:
  dag::Vector<Point3> vertices;
  dag::Vector<int> indices;
  dag::Vector<PlaneDefinition> planeDefinitions;
  Point3 center;
  TMatrix rm = TMatrix::IDENT;

  void setPreset(KdopPreset set_preset, float cut_off_threshold = 0.0f, int seg1 = -1, int seg2 = -1);
  void setRotation(const Point3 &rot_deg);
  void reset();
  template <typename T>
  void calcKdop(const T &verts, const TMatrix &tm = TMatrix::IDENT);
  bool isPointInside(const Point3 &p);

private:
  float eps = 0.0f;
  float cutOffThreshold = 0.0f;
  float areaLimit = -FLT_MAX;

  void calcKdop();
  void calcPlanes();
  void calcIndices();
  void calcArea();
  void cutOffPlanes();
  void clear();

  void setup6Dop();
  void setup14Dop();
  void setup18Dop();
  void setup26Dop();
  void setupKdopFixedX(int seg);
  void setupKdopFixedY(int seg);
  void setupKdopFixedZ(int seg);
  void setupCustomKdop(int seg1, int seg2);
};

template <typename T>
void Kdop::calcKdop(const T &verts, const TMatrix &tm)
{
  G_ASSERT_RETURN(!verts.empty(), );
  TMatrix itm = inverse(tm);
  for (int i = 0; i < planeDefinitions.size(); ++i)
  {
    planeDefinitions[i].planeNormal = itm % rm % planeDefinitions[i].planeNormal;
  }

  int cnt = verts.size();
  for (const auto &vert : verts)
  {
    center += vert;
  }
  center /= cnt;

  for (const auto &vert : verts)
  {
    for (auto &planeDefinition : planeDefinitions)
    {
      const Point3 &p = vert;
      const Point3 &n = planeDefinition.planeNormal;
      const Point3 &o = center;
      float t = (p - o) * n;
      planeDefinition.limit = max(t, planeDefinition.limit);
    }
  }

  calcKdop();
}
