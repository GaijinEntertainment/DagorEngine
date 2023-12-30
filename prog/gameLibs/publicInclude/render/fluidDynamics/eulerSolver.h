#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>

namespace cfd
{

enum class PlotType : uint8_t
{
  DENSITY,
  SPEED,
  PRESSURE,
  PLACEHOLDER,
  NUM_TYPES = PLACEHOLDER
};

// Single solver

class EulerSolver
{
public:
  EulerSolver(const char *solver_shader_name, uint32_t tex_width = 640, uint32_t tex_height = 384, float spatial_step = 1.f);

  void fillInitialConditions(float standard_density, const Point2 &standard_velocity);
  void solveEquations(float dt, int num_dispatches);
  void showResult(PlotType plot_type);

  int getNumDispatches() const;

  TEXTUREID getVelocityDensityTexId() const;
  float getSimulationTime() const;

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;
  eastl::unique_ptr<ComputeShaderElement> blurCs;
  PostFxRenderer showSolution;

  eastl::array<UniqueTex, 2> velDensityTex;

  uint32_t textureWidth = 640;
  uint32_t textureHeight = 384;
  float simulationTime = 0.0f;
  int totalNumDispatches = 0;
};

// Cascade solver

struct Cascade
{
  eastl::array<UniqueTex, 2> velDensityTex;
  IPoint2 texSize;
  float spatialStep;
  float dtMultiplier;
};

class EulerCascadeSolver
{
public:
  EulerCascadeSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step = 1.f, const char *solver_name = "",
    const eastl::array<uint32_t, 4> &num_dispatches_per_cascade = {4700, 1600, 650, 150});

  void fillInitialConditions(float standard_density, const Point2 &standard_velocity);
  bool solveEquations(float dt, int num_dispatches);
  void showResult(PlotType plot_type);
  void reset();

  bool isResultReady() const;

  TEXTUREID getVelocityDensityTexId() const;
  float getSimulationTime() const;
  int getNumDispatches() const;
  void setNumDispatchesForCascade(int cascade_no, int num_dispatches);

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> initialConditionsFromTexCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;
  eastl::unique_ptr<ComputeShaderElement> blurCs;
  PostFxRenderer showSolution;

  static constexpr int NUM_CASCADES = 4;
  int currentCascade = 0;

  eastl::array<uint32_t, NUM_CASCADES> numDispatchesPerCascade;

  int curNumDispatches = 0;
  int totalNumDispatches = 0;
  bool resultReady = false;

  eastl::array<Cascade, NUM_CASCADES> cascades;
  int textureDepth = 0;

  float simulationTime = 0.0f;
  float standardDensity = 0.0f;
  Point2 standardVelocity = Point2::ZERO;

  void switchToCascade(int cascade);
  void fillNextCascadeInitialConditions();
};

} // namespace cfd