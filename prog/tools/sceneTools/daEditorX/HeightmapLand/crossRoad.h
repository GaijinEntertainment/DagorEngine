#pragma once


class SplinePointObject;

class CrossRoadData
{
public:
  PtrTab<SplinePointObject> points;
  GeomObject *geom;
  bool changed;

  CrossRoadData();
  ~CrossRoadData();

  bool checkCrossRoad();

  GeomObject *getRoadGeom() { return geom; }

  GeomObject &getClearedRoadGeom();
  void removeRoadGeom();

  int find(SplinePointObject *p) const
  {
    for (int i = 0; i < points.size(); i++)
      if (points[i] == p)
        return i;
    return -1;
  }
};
