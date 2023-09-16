//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <osApiWrappers/dag_cpuJobs.h>

class Point4;
class ShaderMaterial;
class ShaderElement;

class Metaballs
{
  uint32_t gridSize = 0;
  float gridScale = 1;
  Point3 position = Point3(0, 0, 0);
  eastl::vector<Point4> balls;

  eastl::unique_ptr<ShaderMaterial> shMat;
  ShaderElement *shElem;
  eastl::unique_ptr<Vbuffer, DestroyDeleter<Vbuffer>> vb;
  eastl::unique_ptr<Ibuffer, DestroyDeleter<Ibuffer>> ib;
  bool transparent = false;
  float loweringLevel = 0;

  class MetaballsUpdateJob final : public cpujobs::IJob
  {
    uint32_t gridSize = 0;
    float gridScale = 1;
    float loweringLevel = 0;
    Point3 position = Point3(0, 0, 0);
    eastl::vector<float> grid;
    eastl::vector<Point4> gradGrid;
    eastl::vector<Point4> balls;

    eastl::array<Point3, 12> vertList;
    eastl::array<Point4, 8> gradients;
    eastl::array<Point3, 12> gradList;
    eastl::array<Point4, 8> vv;
    eastl::vector<uint8_t> normalsCount;
    eastl::vector<eastl::vector<uint16_t>> verticesPerCubes;
    eastl::vector<Point3> normals;

    void updateVerts(int i, int j, int k, int idx1, int idx2);
    Point3 vertexInterpolation(const Point4 &p1, const Point4 &p2);
    void getVec4(int index, int i, int j, int k, Point4 &p);
    Point4 getGradient(int index, int i, int j, int k);
    void marchingCubes(int cubeX, int cubeY, int cubeZ);
    void updateGrid();
    void addVertex(const Point3 &pos, const Point3 &normal, int i, int j, int k);
    void update();

  public:
    eastl::vector<Point3> vertices;
    eastl::vector<uint16_t> indices;
    eastl::vector<Point3> vertData;
    int numTriangles;

    void setBalls(const eastl::vector<Point4> &b) { balls = b; };
    void setPos(const Point3 &pos) { position = pos; }
    void setGridSize(uint32_t grid_size);
    void setScale(float grid_scale) { gridScale = grid_scale; }
    void setLoweringLevel(float lowering_level) { loweringLevel = lowering_level; }
    virtual void doJob() override { update(); }
  };

  eastl::vector<MetaballsUpdateJob> updateJobs;
  int currentUpdateJob = 0;
  void initShader();

public:
  Point3 albedo;
  Point4 mediumTintColor;
  float smoothness, reflectance, min_thickness, max_thickness, IoR, isShell, emission, max_alpha, min_alpha;

  Metaballs();
  ~Metaballs();

  void setBalls(const eastl::vector<Point4> &b) { balls = b; };
  void setPos(const Point3 &pos) { position = pos; }
  void setGridSize(uint32_t grid_size) { gridSize = grid_size; }
  void setScale(float grid_scale) { gridScale = grid_scale; }
  void setLoweringLevel(float lowering_level) { loweringLevel = lowering_level; }
  void enableTransparency(bool enable);
  void render();
  void startUpdate();
};
