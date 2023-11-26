#include <render/fluidDynamics/solver.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint3.h>

namespace cfd
{

#define VARS_LIST                   \
  VAR(velocity_density_tex)         \
  VAR(next_velocity_density_tex)    \
  VAR(tex_size)                     \
  VAR(plot_type)                    \
  VAR(plot_tex)                     \
  VAR(simulation_dt)                \
  VAR(simulation_dx)                \
  VAR(simulation_time)              \
  VAR(standard_density)             \
  VAR(standard_velocity)            \
  VAR(initial_velocity_density_tex) \
  VAR(euler_implicit_mode)

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

  velDensityTex[0] =
    dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "velocity_density_tex");
  velDensityTex[1] =
    dag::create_tex(NULL, textureWidth, textureHeight, TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, "next_velocity_density_tex");

  // Mirror for ghost cells on the edges
  velDensityTex[0].getTex2D()->texaddr(TEXADDR_MIRROR);
  velDensityTex[1].getTex2D()->texaddr(TEXADDR_MIRROR);

  ShaderGlobal::set_int4(tex_sizeVarId, IPoint4(textureWidth, textureHeight, 0, 0));
  ShaderGlobal::set_real(simulation_dxVarId, spatal_step);
}

void Solver::fillInitialConditions(float standard_density, const Point2 &standard_velocity)
{
  ShaderGlobal::set_texture(velocity_density_texVarId, velDensityTex[0]);
  ShaderGlobal::set_real(standard_densityVarId, standard_density);
  ShaderGlobal::set_color4(standard_velocityVarId, Color4(standard_velocity.x, standard_velocity.y, 0.0f, 0.0f));

  initialConditionsCs->dispatchThreads(textureWidth, textureHeight, 1);
}

void Solver::solveEquations(float dt, int num_dispatches)
{
  TIME_D3D_PROFILE("cfd_solveEquations");

  int currentIdx = 0;
  int currentImplicit = 0;

  for (int i = 0; i < num_dispatches; ++i)
  {
    ShaderGlobal::set_texture(velocity_density_texVarId, velDensityTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_density_texVarId, velDensityTex[1 - currentIdx]);
    ShaderGlobal::set_real(simulation_dtVarId, dt);
    ShaderGlobal::set_real(simulation_timeVarId, simulationTime);
    ShaderGlobal::set_int(euler_implicit_modeVarId, currentImplicit);

    solverCs->dispatchThreads(textureWidth, textureHeight, 1);

    simulationTime += dt;
    currentIdx = (currentIdx + 1) % 2;
    currentImplicit = (currentImplicit + 1) % 2;

    ShaderGlobal::set_texture(velocity_density_texVarId, velDensityTex[currentIdx]);
    ShaderGlobal::set_texture(next_velocity_density_texVarId, velDensityTex[1 - currentIdx]);

    blurCs->dispatchThreads(textureWidth, textureHeight, 1);

    currentIdx = (currentIdx + 1) % 2;
  }

  totalNumDispatches += num_dispatches;
}

void Solver::showResult(PlotType plot_type)
{
  ShaderGlobal::set_int(plot_typeVarId, (int)plot_type);
  ShaderGlobal::set_texture(plot_texVarId, velDensityTex[0].getTexId());
  showSolution.render();
}

int Solver::getNumDispatches() const { return totalNumDispatches; }

TEXTUREID Solver::getVelocityDensityTexId() const { return velDensityTex[0].getTexId(); }

float Solver::getSimulationTime() const { return simulationTime; }


// CascadeSolver

static constexpr float cascadeDtMultipliers[4] = {2.0f, 2.0f, 1.5f, 1.0f};
static constexpr float cascadeDispatchNumMultipliers[4] = {1.0f, 0.5f, 0.125f, 0.03125f};

CascadeSolver::CascadeSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step, const char *solver_name,
  const eastl::array<uint32_t, 4> &num_dispatches_per_cascade) :
  numDispatchesPerCascade(eastl::move(num_dispatches_per_cascade)), textureDepth(tex_size.z)
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
    Cascade &newCascade = cascades[i];
    newCascade.texSize = IPoint2(tex_size.x / (1 << (NUM_CASCADES - 1 - i)), tex_size.y / (1 << (NUM_CASCADES - 1 - i)));
    newCascade.spatialStep = spatial_step * (1 << (NUM_CASCADES - 1 - i));
    newCascade.dtMultiplier = cascadeDtMultipliers[i];
    newCascade.velDensityTex[0] = dag::create_voltex(newCascade.texSize.x, newCascade.texSize.y, tex_size.z,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "velocity_density_cascade_%d_%s", i, solver_name));
    newCascade.velDensityTex[1] = dag::create_voltex(newCascade.texSize.x, newCascade.texSize.y, tex_size.z,
      TEXFMT_A32B32G32R32F | TEXCF_UNORDERED, 1, String(0, "next_velocity_density_cascade_%d_%s", i, solver_name));

    d3d::resource_barrier({cascades[i].velDensityTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
    d3d::resource_barrier({cascades[i].velDensityTex[1].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});

    newCascade.velDensityTex[0].getVolTex()->texfilter(TEXFILTER_POINT);
    newCascade.velDensityTex[1].getVolTex()->texfilter(TEXFILTER_POINT);

    // Mirror for ghost cells on the edges
    newCascade.velDensityTex[0].getVolTex()->texaddr(TEXADDR_MIRROR);
    newCascade.velDensityTex[1].getVolTex()->texaddr(TEXADDR_MIRROR);
  }
}

void CascadeSolver::fillInitialConditions(float standard_density, const Point2 &standard_velocity)
{
  switchToCascade(0);

  standardDensity = standard_density;
  standardVelocity = standard_velocity;

  ShaderGlobal::set_real(standard_densityVarId, standardDensity);
  ShaderGlobal::set_color4(standard_velocityVarId, Color4(standardVelocity.x, standardVelocity.y, 0.0f, 0.0f));

  initialConditionsCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth);
  d3d::resource_barrier({cascades[currentCascade].velDensityTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
}

bool CascadeSolver::solveEquations(float dt, int base_num_dispatches)
{
  TIME_D3D_PROFILE("cfd_solveEquationsCascade");

  if (curNumDispatches >= numDispatchesPerCascade[currentCascade])
    return false;

  int i = 0;
  const int actualNumDispatches = static_cast<int>(base_num_dispatches * cascadeDispatchNumMultipliers[currentCascade]);

  ShaderGlobal::set_real(standard_densityVarId, standardDensity);
  ShaderGlobal::set_color4(standard_velocityVarId, Color4(standardVelocity.x, standardVelocity.y, 0.0f, 0.0f));

  while (i < actualNumDispatches)
  {
    const float actualDt = dt * cascades[currentCascade].dtMultiplier;

    ShaderGlobal::set_real(simulation_dtVarId, actualDt);
    ShaderGlobal::set_int4(tex_sizeVarId,
      IPoint4(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth, 0));
    ShaderGlobal::set_real(simulation_dxVarId, cascades[currentCascade].spatialStep);

    int currentIdx = 0;
    int currentImplicit = 0;

    solverCs->setStates();
    blurCs->setStates();
    for (; i < actualNumDispatches && curNumDispatches < numDispatchesPerCascade[currentCascade]; ++i)
    {
      d3d::set_tex(STAGE_CS, 1, cascades[currentCascade].velDensityTex[currentIdx].getBaseTex());
      d3d::set_rwtex(STAGE_CS, 0, cascades[currentCascade].velDensityTex[1 - currentIdx].getBaseTex(), 0, 0);

      solverCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth,
        GpuPipeline::GRAPHICS, false);
      d3d::resource_barrier({cascades[currentCascade].velDensityTex[currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
      d3d::resource_barrier({cascades[currentCascade].velDensityTex[1 - currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});

      simulationTime += actualDt;
      currentIdx = (currentIdx + 1) % 2;
      currentImplicit = (currentImplicit + 1) % 2;

      d3d::set_tex(STAGE_CS, 1, cascades[currentCascade].velDensityTex[currentIdx].getBaseTex());
      d3d::set_rwtex(STAGE_CS, 0, cascades[currentCascade].velDensityTex[1 - currentIdx].getBaseTex(), 0, 0);

      blurCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth,
        GpuPipeline::GRAPHICS, false);
      d3d::resource_barrier({cascades[currentCascade].velDensityTex[currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
      d3d::resource_barrier({cascades[currentCascade].velDensityTex[1 - currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});

      currentIdx = (currentIdx + 1) % 2;

      ++curNumDispatches;
      ++totalNumDispatches;
    }

    d3d::set_tex(STAGE_CS, 1, nullptr);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);

    if (curNumDispatches >= numDispatchesPerCascade[currentCascade])
    {
      d3d::resource_barrier(
        {cascades[currentCascade].velDensityTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_RO_SRV, 0, 0});
      if (currentCascade != NUM_CASCADES - 1)
        switchToCascade(currentCascade + 1);
      else
      {
        resultReady = true;
        return true;
      }
    }
  }
  return true;
}

void CascadeSolver::showResult(PlotType plot_type)
{
  ShaderGlobal::set_int(plot_typeVarId, (int)plot_type);
  ShaderGlobal::set_texture(plot_texVarId, cascades[currentCascade].velDensityTex[0].getTexId());
  showSolution.render();
}

void CascadeSolver::reset()
{
  curNumDispatches = 0;
  totalNumDispatches = 0;
  simulationTime = 0.0f;
  currentCascade = 0;
  resultReady = false;
}

bool CascadeSolver::isResultReady() const { return resultReady; }

TEXTUREID CascadeSolver::getVelocityDensityTexId() const { return cascades[currentCascade].velDensityTex[0].getTexId(); }

float CascadeSolver::getSimulationTime() const { return simulationTime; }

int CascadeSolver::getNumDispatches() const { return totalNumDispatches; }

void CascadeSolver::switchToCascade(int cascade)
{
  ShaderGlobal::set_texture(velocity_density_texVarId, cascades[cascade].velDensityTex[0]);
  ShaderGlobal::set_texture(next_velocity_density_texVarId, cascades[cascade].velDensityTex[1]);
  ShaderGlobal::set_int4(tex_sizeVarId, IPoint4(cascades[cascade].texSize.x, cascades[cascade].texSize.y, textureDepth, 0));
  ShaderGlobal::set_real(simulation_dxVarId, cascades[cascade].spatialStep);

  if (cascade != 0)
    fillNextCascadeInitialConditions();

  curNumDispatches = 0;
  currentCascade = cascade;
}

void CascadeSolver::fillNextCascadeInitialConditions()
{
  ShaderGlobal::set_texture(initial_velocity_density_texVarId, cascades[currentCascade].velDensityTex[0]);

  initialConditionsFromTexCs->dispatchThreads(cascades[currentCascade + 1].texSize.x, cascades[currentCascade + 1].texSize.y,
    textureDepth);
  d3d::resource_barrier({cascades[currentCascade + 1].velDensityTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
}

} // namespace cfd
