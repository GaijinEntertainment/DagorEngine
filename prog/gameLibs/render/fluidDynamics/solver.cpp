#include <render/fluidDynamics/solver.h>
#include <perfMon/dag_statDrv.h>

namespace cfd
{

#define VARS_LIST                 \
  VAR(velocity_pressure_tex)      \
  VAR(next_velocity_pressure_tex) \
  VAR(density_tex)                \
  VAR(next_density_tex)           \
  VAR(tex_size)                   \
  VAR(plot_type)                  \
  VAR(plot_tex)                   \
  VAR(simulation_dt)              \
  VAR(simulation_dx)              \
  VAR(simulation_time)            \
  VAR(standard_density)           \
  VAR(standard_velocity)          \
  VAR(initial_density_tex)        \
  VAR(euler_implicit_mode)        \
  VAR(initial_velocity_pressure_tex)

#define VAR(a) static int a##VarId = -1;
VARS_LIST
#undef VAR

// Solver

Solver::Solver(const char *solver_shader_name, uint32_t tex_width, uint32_t tex_height, float spatal_step) :
  textureWidth(tex_width), textureHeight(tex_height)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  VARS_LIST
#undef VAR

  initialConditionsCs.reset(new_compute_shader("fill_initial_conditions"));
  solverCs.reset(new_compute_shader(solver_shader_name));
  blurCs.reset(new_compute_shader("blur_result_cs"));
  showSolution.init("show_cfd_solution");

  velPressureTex[0] =
    dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "velocity_pressure_tex");
  velPressureTex[1] =
    dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "next_velocity_pressure_tex");
  densityTex[0] = dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "density_tex");
  densityTex[1] = dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "next_density_tex");

  // Mirror for ghost cells on the edges
  velPressureTex[0].getTex2D()->texaddr(TEXADDR_MIRROR);
  velPressureTex[1].getTex2D()->texaddr(TEXADDR_MIRROR);
  densityTex[0].getTex2D()->texaddr(TEXADDR_MIRROR);
  densityTex[1].getTex2D()->texaddr(TEXADDR_MIRROR);

  ShaderGlobal::set_int4(tex_sizeVarId, IPoint4(textureWidth, textureHeight, 0, 0));
  ShaderGlobal::set_real(simulation_dxVarId, spatal_step);
}

void Solver::fillInitialConditions(float standard_density, const Point2 &standard_velocity)
{
  ShaderGlobal::set_texture(velocity_pressure_texVarId, velPressureTex[0]);
  ShaderGlobal::set_texture(density_texVarId, densityTex[0]);
  ShaderGlobal::set_real(standard_densityVarId, standard_density);
  ShaderGlobal::set_color4(standard_velocityVarId, Color4(standard_velocity.x, standard_velocity.y, 0.0f, 0.0f));

  initialConditionsCs->dispatchThreads(textureWidth, textureHeight, 1);
}

void Solver::solveEquations(float dt, int num_dispatches)
{
  TIME_D3D_PROFILE("cfd::solveEquations");

  int currentIdx = 0;
  int currentImplicit = 0;

  for (int i = 0; i < num_dispatches; ++i)
  {
    ShaderGlobal::set_texture(velocity_pressure_texVarId, velPressureTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_pressure_texVarId, velPressureTex[1 - currentIdx]);
    ShaderGlobal::set_texture(density_texVarId, densityTex[currentIdx]);
    ShaderGlobal::set_texture(next_density_texVarId, densityTex[1 - currentIdx]);
    ShaderGlobal::set_real(simulation_dtVarId, dt);
    ShaderGlobal::set_real(simulation_timeVarId, simulationTime);
    ShaderGlobal::set_int(euler_implicit_modeVarId, currentImplicit);

    solverCs->dispatchThreads(textureWidth, textureHeight, 1);

    simulationTime += dt;
    currentIdx = (currentIdx + 1) % 2;
    currentImplicit = (currentImplicit + 1) % 2;

    ShaderGlobal::set_texture(velocity_pressure_texVarId, velPressureTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_pressure_texVarId, velPressureTex[1 - currentIdx]);
    ShaderGlobal::set_texture(density_texVarId, densityTex[currentIdx]);
    ShaderGlobal::set_texture(next_density_texVarId, densityTex[1 - currentIdx]);

    blurCs->dispatchThreads(textureWidth, textureHeight, 1);

    currentIdx = (currentIdx + 1) % 2;
  }

  totalNumDispatches += num_dispatches;
}

void Solver::showResult(PlotType plot_type)
{
  ShaderGlobal::set_int(plot_typeVarId, (int)plot_type);
  switch (plot_type)
  {
    case PlotType::DENSITY: ShaderGlobal::set_texture(plot_texVarId, densityTex[0].getTexId()); break;
    case PlotType::SPEED: ShaderGlobal::set_texture(plot_texVarId, velPressureTex[0].getTexId()); break;
    case PlotType::PRESSURE: ShaderGlobal::set_texture(plot_texVarId, velPressureTex[0].getTexId()); break;
  }
  showSolution.render();
}

int Solver::getNumDispatches() const { return totalNumDispatches; }

TEXTUREID Solver::getVelocityPressureTexId() const { return velPressureTex[0].getTexId(); }

float Solver::getSimulationTime() const { return simulationTime; }


// CascadeSolver

// TODO: account for different number of cascades
static constexpr float cascadeDtMultipliers[4] = {4.f, 3.5f, 2.0f, 1.0f};

CascadeSolver::CascadeSolver(const char *solver_shader_name, uint32_t tex_width, uint32_t tex_height,
  const eastl::array<uint32_t, 4> &num_dispatches_per_cascade, float spatial_step) :
  numDispatchesPerCascade(eastl::move(num_dispatches_per_cascade))
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  VARS_LIST
#undef VAR

  initialConditionsCs.reset(new_compute_shader("fill_initial_conditions"));
  initialConditionsFromTexCs.reset(new_compute_shader("fill_initial_conditions_from_tex"));
  solverCs.reset(new_compute_shader(solver_shader_name));
  blurCs.reset(new_compute_shader("blur_result_cs"));
  showSolution.init("show_cfd_solution");

  for (int i = 0; i < NUM_CASCADES; ++i)
  {
    auto &newCascade = cascades.push_back();
    newCascade.texSize = IPoint2(tex_width / (1 << (NUM_CASCADES - 1 - i)), tex_height / (1 << (NUM_CASCADES - 1 - i)));
    newCascade.spatialStep = spatial_step * (1 << (NUM_CASCADES - 1 - i));
    newCascade.dtMultiplier = cascadeDtMultipliers[i];
    newCascade.velPressureTex[0] = dag::create_tex(NULL, newCascade.texSize.x, newCascade.texSize.y,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "velocity_pressure_cascade_%d", i));
    newCascade.velPressureTex[1] = dag::create_tex(NULL, newCascade.texSize.x, newCascade.texSize.y,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "next_velocity_pressure_cascade_%d", i));
    newCascade.densityTex[0] = dag::create_tex(NULL, newCascade.texSize.x, newCascade.texSize.y,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "density_cascade_%d", i));
    newCascade.densityTex[1] = dag::create_tex(NULL, newCascade.texSize.x, newCascade.texSize.y,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "next_density_cascade_%d", i));

    // Mirror for ghost cells on the edges
    newCascade.velPressureTex[0].getTex2D()->texaddr(TEXADDR_MIRROR);
    newCascade.velPressureTex[1].getTex2D()->texaddr(TEXADDR_MIRROR);
    newCascade.densityTex[0].getTex2D()->texaddr(TEXADDR_MIRROR);
    newCascade.densityTex[1].getTex2D()->texaddr(TEXADDR_MIRROR);
  }
}

void CascadeSolver::fillInitialConditions(float standard_density, const Point2 &standard_velocity)
{
  switchToCascade(0);

  ShaderGlobal::set_real(standard_densityVarId, standard_density);
  ShaderGlobal::set_color4(standard_velocityVarId, Color4(standard_velocity.x, standard_velocity.y, 0.0f, 0.0f));

  initialConditionsCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, 1);
}

void CascadeSolver::solveEquations(float dt, int num_dispatches)
{
  TIME_D3D_PROFILE("cfd::solveEquationsCascade");

  if (curNumDispatches > numDispatchesPerCascade[currentCascade])
    return;

  const float actualDt = dt * (cascades[currentCascade].dtMultiplier == 1 ? 1 : cascades[currentCascade].dtMultiplier / 1.5f);
  int currentIdx = 0;
  int currentImplicit = 0;
  ShaderGlobal::set_real(simulation_dtVarId, actualDt);
  for (int i = 0; i < num_dispatches; ++i)
  {
    ShaderGlobal::set_texture(velocity_pressure_texVarId, cascades[currentCascade].velPressureTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_pressure_texVarId, cascades[currentCascade].velPressureTex[1 - currentIdx]);
    ShaderGlobal::set_texture(density_texVarId, cascades[currentCascade].densityTex[currentIdx]);
    ShaderGlobal::set_texture(next_density_texVarId, cascades[currentCascade].densityTex[1 - currentIdx]);
    ShaderGlobal::set_int(euler_implicit_modeVarId, currentImplicit);

    solverCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, 1);

    simulationTime += actualDt;
    currentIdx = (currentIdx + 1) % 2;
    currentImplicit = (currentImplicit + 1) % 2;

    ShaderGlobal::set_texture(velocity_pressure_texVarId, cascades[currentCascade].velPressureTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_pressure_texVarId, cascades[currentCascade].velPressureTex[1 - currentIdx]);
    ShaderGlobal::set_texture(density_texVarId, cascades[currentCascade].densityTex[currentIdx]);
    ShaderGlobal::set_texture(next_density_texVarId, cascades[currentCascade].densityTex[1 - currentIdx]);

    blurCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, 1);

    currentIdx = (currentIdx + 1) % 2;
  }

  curNumDispatches += num_dispatches;
  totalNumDispatches += num_dispatches;
  if (curNumDispatches >= numDispatchesPerCascade[currentCascade])
  {
    if (currentCascade != NUM_CASCADES - 1)
      switchToCascade(currentCascade + 1);
    else
      return;
  }
}

void CascadeSolver::showResult(PlotType plot_type)
{
  ShaderGlobal::set_int(plot_typeVarId, (int)plot_type);
  switch (plot_type)
  {
    case PlotType::DENSITY: ShaderGlobal::set_texture(plot_texVarId, cascades[currentCascade].densityTex[0].getTexId()); break;
    case PlotType::SPEED: ShaderGlobal::set_texture(plot_texVarId, cascades[currentCascade].velPressureTex[0].getTexId()); break;
    case PlotType::PRESSURE: ShaderGlobal::set_texture(plot_texVarId, cascades[currentCascade].velPressureTex[0].getTexId()); break;
  }
  showSolution.render();
}

TEXTUREID CascadeSolver::getVelocityPressureTexId() const { return cascades[currentCascade].velPressureTex[0].getTexId(); }

float CascadeSolver::getSimulationTime() const { return simulationTime; }

int CascadeSolver::getNumDispatches() const { return totalNumDispatches; }

void CascadeSolver::switchToCascade(int cascade)
{
  ShaderGlobal::set_texture(velocity_pressure_texVarId, cascades[cascade].velPressureTex[0]);
  ShaderGlobal::set_texture(next_velocity_pressure_texVarId, cascades[cascade].velPressureTex[1]);
  ShaderGlobal::set_texture(density_texVarId, cascades[cascade].densityTex[0]);
  ShaderGlobal::set_texture(next_density_texVarId, cascades[cascade].densityTex[1]);
  ShaderGlobal::set_int4(tex_sizeVarId, IPoint4(cascades[cascade].texSize.x, cascades[cascade].texSize.y, 0, 0));
  ShaderGlobal::set_real(simulation_dxVarId, cascades[cascade].spatialStep);

  fillNextCascadeInitialConditions();

  curNumDispatches = 0;
  currentCascade = cascade;
}

void CascadeSolver::fillNextCascadeInitialConditions()
{
  ShaderGlobal::set_texture(initial_velocity_pressure_texVarId, cascades[currentCascade].velPressureTex[0]);
  ShaderGlobal::set_texture(initial_density_texVarId, cascades[currentCascade].densityTex[0]);

  initialConditionsFromTexCs->dispatchThreads(cascades[currentCascade + 1].texSize.x, cascades[currentCascade + 1].texSize.y, 1);
}

} // namespace cfd
