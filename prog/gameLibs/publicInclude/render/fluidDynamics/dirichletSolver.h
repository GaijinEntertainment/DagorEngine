#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>

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
  DirichletCascadeSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step = 1.f,
    const eastl::array<uint32_t, NUM_CASCADES> &num_dispatches_per_cascade = {150, 650, 1600});

  void fillInitialConditions();
  void solveEquations(float dt, int num_dispatches, bool implicit = false);
  void reset();
  bool isResultReady() const;

  TEXTUREID getPotentialTexId() const;
  float getSimulationTime() const;
  int getNumDispatches() const;
  void setNumDispatchesForCascade(int cascade_no, int num_dispatches);
  void setBoundariesCb(eastl::function<void(int)> cb);

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> initialConditionsFromTexCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;

  int currentCascade = 0;

  // From the biggest to the smallest
  eastl::array<uint32_t, NUM_CASCADES> numDispatchesPerCascade;

  int curNumDispatches = 0;
  int totalNumDispatches = 0;
  bool resultReady = false;

  eastl::array<Cascade, NUM_CASCADES> cascades;
  eastl::function<void(int)> boundariesCb;
  int textureDepth = 0;

  float simulationTime = 0.0f;

  void switchToCascade(int cascade);
};

} // namespace cfd
