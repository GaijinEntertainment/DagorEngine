// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>


class DataBlock;
class Mesh;


class GeomMeshHelper
{
public:
  struct Face
  {
    int v[3];

    Face() {} //-V730
    Face(int v0, int v1, int v2)
    {
      v[0] = v0;
      v[1] = v1;
      v[2] = v2;
    }
  };

  struct Edge
  {
    int v[2];

    Edge() {} //-V730
    Edge(int v0, int v1)
    {
      v[0] = v0;
      v[1] = v1;
    }
    bool operator==(Edge other) const { return v[0] == other.v[0] && v[1] == other.v[1]; }
  };

  Tab<Point3> verts;
  Tab<Face> faces;
  Tab<Edge> edges;

  BBox3 localBox;


  GeomMeshHelper(IMemAlloc *mem = tmpmem);

  void init(Mesh &from_mesh);

  void buildEdges();
  void calcBox();

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  void calcMomj(Matrix3 &momj, Point3 &cmpos, real &volume, const TMatrix &tm);
};


struct GeomMeshHelperDagObject
{
  String name;
  TMatrix wtm;
  GeomMeshHelper mesh;

  void save(DataBlock &blk);
};


bool import_geom_mesh_helpers_dag(const char *filename, Tab<GeomMeshHelperDagObject> &objects);
