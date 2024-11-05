//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_carray.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <generic/dag_tab.h>

class WaterRipples
{
public:
  WaterRipples(float world_size, int simulation_tex_size, float water_displacement_max = 0.125f, bool use_flowmap = false);
  ~WaterRipples();

  void placeDrop(const Point2 &pos, float strength, float radius);
  void placeSolidBox(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y, float strength,
    float radius);
  void placeDropCorrected(const Point2 &pos, float strength, float radius);
  void placeSolidBoxCorrected(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y, float strength,
    float radius);
  void reset();

  float getWorldSize() const { return worldSize; }
  int getNextNumSteps() const { return nextNumSteps; }

  void updateSteps(float dt);
  void advance(float dt, const Point2 &origin);

private:
  void addDropInst(int reg_num);
  void clearRts();

  int waterRipplesOnVarId;
  int waterRipplesDropCountVarId;
  int waterRipplesOriginVarId;
  int waterRipplesOriginDeltaVarId;
  int waterRipplesDisplaceMaxVarId;
  int waterRipplesT1VarId;
  int waterRipplesT1_samplerstateVarId;
  int waterRipplesT2VarId;
  int waterRipplesT2_samplerstateVarId;
  int waterRipplesDropsVarId;
  int waterRipplesFrameNoVarId;
  int waterRipplesFlowmapVarId;
  int waterRipplesFlowmapFrameCountVarId;

  int frameNo = 0;
  int flowmapFrameCount = 120;

  bool firstStep = true;
  int curBuffer = 0;
  Tab<Point4> drops;
  Tab<int> dropsInst;
  float dropsAliveTime = 0.0f;
  bool inSleep = true;
  bool useFlowmap = false;
  carray<Point2, 2> lastStepOrigin;
  float worldSize;
  int texSize;
  float texelSize;

  int curNumSteps = 1;
  int nextNumSteps = 0;
  float accumStep = 0.0f;
  int curStepNo = 0;
  carray<float, 30> steps;

  carray<UniqueTex, 3> texBuffers;
  UniqueTexHolder heightNormalTexture;
  PostFxRenderer updateRenderer;
  PostFxRenderer resolveRenderer;
  UniqueBuf dropsBuf;
  d3d::SamplerHandle waterRipplesLinearSampler;
  d3d::SamplerHandle waterRipplesPointSampler;
};
