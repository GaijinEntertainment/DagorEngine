#ifndef DELAUNAY_INCLUDED // -*- C++ -*-
#define DELAUNAY_INCLUDED

class Mesh;
class Point3;
namespace delaunay
{

class Map
{
public:
  virtual int width() = 0;
  virtual int height() = 0;

  float operator()(int i, int j) { return eval(i, j); }
  virtual float eval(int i, int j) = 0;
};

class ImportMask
{
public:
  virtual float apply(int /*x*/, int /*y*/, float val) { return val; }
};


void load(float error_threshold1, int point_limit1, Map *DEM, Mesh &out, float cell, const Point3 &ofs, ImportMask *MASK1);

}; // namespace delaunay
#endif
