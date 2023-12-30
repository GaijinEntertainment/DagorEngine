#include <render/fluidDynamics/dirichletSolver.h>
#include <perfMon/dag_statDrv.h>

namespace cfd
{

// Single solver

#define DIRICHLET_VARS_LIST      \
  VAR(cfd_tex_size)              \
  VAR(cfd_simulation_dt)         \
  VAR(cfd_simulation_dx)         \
  VAR(cfd_simulation_time)       \
  VAR(cfd_potential_tex)         \
  VAR(cfd_initial_potential_tex) \
  VAR(dirichlet_cascade_no)      \
  VAR(dirichlet_implicit_mode)   \
  VAR(cfd_cascade_no)

#define VAR(a) static int a##VarId = -1;
DIRICHLET_VARS_LIST
#undef VAR

DirichletSolver::DirichletSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step) :
  texSize(tex_size), spatialStep(spatial_step)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  DIRICHLET_VARS_LIST
#undef VAR

  for (int i = 0; i < 2; ++i)
  {
    potentialTex[i] = dag::create_voltex(tex_size.x, tex_size.y, tex_size.z, TEXFMT_G32R32F | TEXCF_UNORDERED, 1,
      String(0, "cfd_potential_tex_%d", i));
    d3d::resource_barrier({potentialTex[i].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
    potentialTex[i].getVolTex()->texfilter(TEXFILTER_POINT);
    potentialTex[i].getVolTex()->texaddr(TEXADDR_MIRROR);
  }

  ShaderGlobal::set_int4(cfd_tex_sizeVarId, IPoint4(tex_size.x, tex_size.y, tex_size.z, 0));
  initialConditionsCs.reset(new_compute_shader("dirichlet_initial_conditions"));
  solverCs.reset(new_compute_shader(solver_shader_name));
}

void DirichletSolver::fillInitialConditions()
{
  ShaderGlobal::set_texture(cfd_potential_texVarId, potentialTex[0]);
  initialConditionsCs->dispatchThreads(texSize.x, texSize.y, texSize.z);
  d3d::resource_barrier({potentialTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
}

bool DirichletSolver::solveEquations(float dt, int num_dispatches)
{
  TIME_D3D_PROFILE("cfd_solveEquationsDirichlet");

  int currentIdx = 0;

  ShaderGlobal::set_real(cfd_simulation_dtVarId, dt * spatialStep);
  ShaderGlobal::set_real(cfd_simulation_dxVarId, spatialStep);

  solverCs->setStates();
  for (int i = 0; i < num_dispatches; ++i)
  {
    d3d::set_tex(STAGE_CS, 1, potentialTex[currentIdx].getBaseTex());
    d3d::set_rwtex(STAGE_CS, 0, potentialTex[1 - currentIdx].getBaseTex(), 0, 0);

    solverCs->dispatchThreads(texSize.x, texSize.y, texSize.z, GpuPipeline::GRAPHICS, false);

    d3d::resource_barrier({potentialTex[currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
    d3d::resource_barrier({potentialTex[1 - currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
    currentIdx = (currentIdx + 1) % 2;
    simulationTime += dt;

    ++totalNumDispatches;

    if (totalNumDispatches >= numDispatchesUntilConvergence)
    {
      resultReady = true;
      d3d::resource_barrier({potentialTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_RO_SRV, 0, 0});
      return false;
    }
  }

  return true;
}

void DirichletSolver::setTotalDispatches(int num_dispatches) { numDispatchesUntilConvergence = num_dispatches; }

int DirichletSolver::getNumDispatches() const { return totalNumDispatches; }

void DirichletSolver::reset()
{
  totalNumDispatches = 0;
  simulationTime = 0.0f;
  resultReady = false;
}

bool DirichletSolver::isResultReady() const { return resultReady; }
TEXTUREID DirichletSolver::getPotentialTexId() const { return potentialTex[0].getTexId(); }

float DirichletSolver::getSimulationTime() const { return simulationTime; }

// Cascade solver

DirichletCascadeSolver::DirichletCascadeSolver(const char *solver_shader_name, IPoint3 tex_size, float spatial_step,
  const eastl::array<uint32_t, NUM_CASCADES> &num_dispatches_per_cascade) :
  numDispatchesPerCascade(std::move(num_dispatches_per_cascade)), textureDepth(tex_size.z)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  DIRICHLET_VARS_LIST
#undef VAR

  initialConditionsCs.reset(new_compute_shader("dirichlet_initial_conditions"));
  initialConditionsFromTexCs.reset(new_compute_shader("dirichlet_initial_conditions_from_tex"));
  solverCs.reset(new_compute_shader(solver_shader_name));

  for (int cascade_no = 0; cascade_no < NUM_CASCADES; ++cascade_no)
  {
    cascades[cascade_no].texSize = IPoint2(tex_size.x, tex_size.y) / (1 << cascade_no);
    cascades[cascade_no].spatialStep = spatial_step * (1 << cascade_no);
    for (int j = 0; j < 2; ++j)
    {
      cascades[cascade_no].potentialTex[j] = dag::create_voltex(cascades[cascade_no].texSize.x, cascades[cascade_no].texSize.y,
        textureDepth, TEXFMT_G32R32F | TEXCF_UNORDERED, 1, String(0, "cfd_potential_tex_cascade_%d_%d", cascade_no, j));
      d3d::resource_barrier({cascades[cascade_no].potentialTex[j].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
      cascades[cascade_no].potentialTex[j].getVolTex()->texfilter(TEXFILTER_POINT);
      // Mirror for ghost cells on boundaries
      cascades[cascade_no].potentialTex[j].getVolTex()->texaddr(TEXADDR_MIRROR);
    }
  }
}

void DirichletCascadeSolver::fillInitialConditions()
{
  switchToCascade(NUM_CASCADES - 1);
  initialConditionsCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth);
  d3d::resource_barrier({cascades[currentCascade].potentialTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
}

static const eastl::array<float, 3> cascade_num_factor = {0.125f, 0.5f, 1.f};
void DirichletCascadeSolver::solveEquations(float dt, int num_dispatches, bool implicit)
{
  TIME_D3D_PROFILE("cfd_solveDirichletCascade");

  if (curNumDispatches >= numDispatchesPerCascade[currentCascade])
    return;

  num_dispatches *= cascade_num_factor[currentCascade];

  const float actualDt = dt * cascades[currentCascade].spatialStep;
  ShaderGlobal::set_real(cfd_simulation_dtVarId, actualDt);
  ShaderGlobal::set_int4(cfd_tex_sizeVarId,
    IPoint4(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth, 0));
  ShaderGlobal::set_real(cfd_simulation_dxVarId, cascades[currentCascade].spatialStep);

  int currentIdx = 0;
  solverCs->setStates();
  for (int i = 0; i < num_dispatches; ++i)
  {
    d3d::set_tex(STAGE_CS, 1, cascades[currentCascade].potentialTex[currentIdx].getBaseTex());
    d3d::set_rwtex(STAGE_CS, 0, cascades[currentCascade].potentialTex[1 - currentIdx].getBaseTex(), 0, 0);

    if (implicit)
    {
      ShaderGlobal::set_int(dirichlet_cascade_noVarId, currentCascade);
      int implicitMode = currentIdx;
      ShaderGlobal::set_int(dirichlet_implicit_modeVarId, currentIdx);
      // we use groupsize of 64, so in larger cascades we process multiple pixels per thread
      solverCs->dispatchThreads(implicitMode == 0 ? 64 : cascades[currentCascade].texSize.x,
        implicitMode == 1 ? 64 : cascades[currentCascade].texSize.y, textureDepth, GpuPipeline::GRAPHICS, true);
    }
    else
      solverCs->dispatchThreads(cascades[currentCascade].texSize.x, cascades[currentCascade].texSize.y, textureDepth,
        GpuPipeline::GRAPHICS, false);

    d3d::resource_barrier({cascades[currentCascade].potentialTex[currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RW_UAV, 0, 0});
    d3d::resource_barrier({cascades[currentCascade].potentialTex[1 - currentIdx].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
    currentIdx = (currentIdx + 1) % 2;
    simulationTime += actualDt;

    ++totalNumDispatches;
    ++curNumDispatches;

    if (curNumDispatches >= numDispatchesPerCascade[currentCascade])
    {
      debug("Cascade %d finished in %d dispatches", currentCascade, curNumDispatches);
      d3d::resource_barrier(
        {cascades[currentCascade].potentialTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_RO_SRV, 0, 0});
      if (currentCascade == 0)
        resultReady = true;
      else
        switchToCascade(currentCascade - 1);
      break;
    }
  }
}

void DirichletCascadeSolver::switchToCascade(int cascade)
{
  boundariesCb(cascade);
  ShaderGlobal::set_int(cfd_cascade_noVarId, cascade);
  ShaderGlobal::set_texture(cfd_potential_texVarId, cascades[cascade].potentialTex[0]);
  ShaderGlobal::set_int4(cfd_tex_sizeVarId, IPoint4(cascades[cascade].texSize.x, cascades[cascade].texSize.y, textureDepth, 0));

  if (cascade != NUM_CASCADES - 1)
  {
    ShaderGlobal::set_texture(cfd_initial_potential_texVarId, cascades[currentCascade].potentialTex[0]);
    cascades[currentCascade].potentialTex[0].getVolTex()->texfilter(TEXFILTER_SMOOTH);
    initialConditionsFromTexCs->dispatchThreads(cascades[cascade].texSize.x, cascades[cascade].texSize.y, textureDepth);
    d3d::resource_barrier({cascades[cascade].potentialTex[0].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
    cascades[currentCascade].potentialTex[0].getVolTex()->texfilter(TEXFILTER_POINT);
  }

  curNumDispatches = 0;
  currentCascade = cascade;
}

void DirichletCascadeSolver::reset()
{
  totalNumDispatches = 0;
  simulationTime = 0.0f;
  curNumDispatches = 0;
  resultReady = false;
  currentCascade = NUM_CASCADES - 1;
}

bool DirichletCascadeSolver::isResultReady() const { return resultReady; }
TEXTUREID DirichletCascadeSolver::getPotentialTexId() const { return cascades[currentCascade].potentialTex[0].getTexId(); }
float DirichletCascadeSolver::getSimulationTime() const { return simulationTime; }
int DirichletCascadeSolver::getNumDispatches() const { return totalNumDispatches; }
void DirichletCascadeSolver::setNumDispatchesForCascade(int cascade_no, int num_dispatches)
{
  numDispatchesPerCascade[cascade_no] = num_dispatches;
}

void DirichletCascadeSolver::setBoundariesCb(eastl::function<void(int)> cb) { boundariesCb = std::move(cb); }
} // namespace cfd
