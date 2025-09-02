//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_bounds2.h>

namespace cfd
{

class DirichletSolver
{
public:
  DirichletSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step = 1.f);

  void fillInitialConditions();
  bool solveEquations(float dt, int num_dispatches);
  void setTotalDispatches(int num_dispatches);

  int getNumDispatches() const;
  void reset();

  bool isResultReady() const;

  TEXTUREID getPotentialTexId() const;
  float getSimulationTime() const;

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;

  eastl::array<UniqueTex, 2> potentialTex;
  IPoint3 texSize;
  float spatialStep;

  int numDispatchesUntilConvergence = 10000;

  float simulationTime = 0.0f;
  int totalNumDispatches = 0;
  bool resultReady = false;
};

// Cascade solver

class DirichletCascadeSolver
{
  struct Cascade
  {
    eastl::array<UniqueTex, 2> potentialTex;
    IPoint2 texSize;
    float spatialStep;
  };
  static constexpr int NUM_CASCADES = 3;

public:
  enum class ToroidalUpdateRegion
  {
    TOP,
    RIGHT,
    BOTTOM,
    LEFT,
    NONE
  };

  DirichletCascadeSolver(IPoint3 tex_size, float spatial_step = 1.f,
    const eastl::array<uint32_t, NUM_CASCADES> &num_dispatches_per_cascade = {150, 650, 1600});

  void fillInitialConditions();
  void fillInitialConditionsToroidal(ToroidalUpdateRegion update_region);
  void solveEquations(float dt, int num_dispatches, bool implicit = false);
  void solveEquationsPartial(float dt, int num_dispatches, const BBox2 &area, bool implicit = false);
  void reset();
  bool isResultReady() const;
  bool isPartialResultReady() const;
  void resetPartialUpdate();

  TEXTUREID getPotentialTexId() const;
  d3d::SamplerHandle getPotentialTexSampler() const { return pointSampler; }
  float getSimulationTime() const;
  int getNumDispatches() const;
  void setNumDispatchesForCascade(int cascade_no, int num_dispatches);
  void setNumDispatchesPartial(int num_dispatches);
  void setBoundariesCb(eastl::function<void(int)> cb);

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> initialConditionsFromTexCs;
  eastl::unique_ptr<ComputeShaderElement> initialConditionsToroidalCs;
  eastl::unique_ptr<ComputeShaderElement> explicitSolverCs;
  eastl::unique_ptr<ComputeShaderElement> implicitSolverCs;

  int currentCascade = 0;

  // From the biggest to the smallest
  eastl::array<uint32_t, NUM_CASCADES> numDispatchesPerCascade;
  uint32_t numDispatchesPartial = 64;

  int curNumDispatches = 0;
  int curNumDispatchesPartial = 0;
  int totalNumDispatches = 0;
  bool resultReady = false;
  bool partialResultReady = false;

  eastl::array<Cascade, NUM_CASCADES> cascades;
  eastl::function<void(int)> boundariesCb;
  int textureDepth = 0;

  float simulationTime = 0.0f;

  void switchToCascade(int cascade);
  d3d::SamplerHandle pointSampler = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle linearSampler = d3d::INVALID_SAMPLER_HANDLE;
};

} // namespace cfd
