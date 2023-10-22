#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>

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

class Solver
{
public:
  Solver(const char *solver_shader_name, uint32_t tex_width = 640, uint32_t tex_height = 384, float spatial_step = 1.f);

  void fillInitialConditions(float standard_density, const Point2 &standard_velocity);
  void solveEquations(float dt, int num_dispatches);
  void showResult(PlotType plot_type);

  int getNumDispatches() const;

  TEXTUREID getVelocityPressureTexId() const;
  float getSimulationTime() const;

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;
  eastl::unique_ptr<ComputeShaderElement> blurCs;
  PostFxRenderer showSolution;

  eastl::array<UniqueTex, 2> velPressureTex;
  eastl::array<UniqueTex, 2> densityTex;

  uint32_t textureWidth = 640;
  uint32_t textureHeight = 384;
  float simulationTime = 0.0f;
  int totalNumDispatches = 0;
};

struct Cascade
{
  eastl::array<UniqueTex, 2> velPressureTex;
  eastl::array<UniqueTex, 2> densityTex;
  IPoint2 texSize;
  float spatialStep;
  float dtMultiplier;
};

class CascadeSolver
{
public:
  CascadeSolver(const char *solver_shader_name, uint32_t tex_width, uint32_t tex_height,
    const eastl::array<uint32_t, 4> &num_dispatches_per_cascade = {5000, 2000, 1000, 200}, float spatial_step = 1.f);

  void fillInitialConditions(float standard_density, const Point2 &standard_velocity);
  void solveEquations(float dt, int num_dispatches);
  void showResult(PlotType plot_type);

  TEXTUREID getVelocityPressureTexId() const;
  float getSimulationTime() const;
  int getNumDispatches() const;

  static constexpr int NUM_CASCADES = 4;

private:
  eastl::unique_ptr<ComputeShaderElement> initialConditionsCs;
  eastl::unique_ptr<ComputeShaderElement> initialConditionsFromTexCs;
  eastl::unique_ptr<ComputeShaderElement> solverCs;
  eastl::unique_ptr<ComputeShaderElement> blurCs;
  PostFxRenderer showSolution;

  int currentCascade = 0;

  eastl::array<uint32_t, NUM_CASCADES> numDispatchesPerCascade;

  int curNumDispatches = 0;
  int totalNumDispatches = 0;

  Tab<Cascade> cascades;

  float simulationTime = 0.0f;

  void switchToCascade(int cascade);
  void fillNextCascadeInitialConditions();
};
} // namespace cfd