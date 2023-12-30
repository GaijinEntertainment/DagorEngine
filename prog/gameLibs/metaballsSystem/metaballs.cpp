#include <3d/dag_drv3d.h>
#include <math/dag_Point4.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <util/dag_threadPool.h>

#include "metaballsSystem/metaballs.h"
#include "marchingCubeTables.h"


CONSOLE_FLOAT_VAL_MINMAX("metaballs", threshold, 0.5, 0, 10);

#define GLOBAL_VARS_OPTIONAL_LIST \
  VAR(metaballs_albedo)           \
  VAR(metaballs_smoothness)       \
  VAR(metaballs_reflectance)      \
  VAR(metaball_min_thickness)     \
  VAR(metaball_max_thickness)     \
  VAR(metaball_IoR)               \
  VAR(metaball_mediumTintColor)   \
  VAR(metaball_isShell)           \
  VAR(metaball_emission)          \
  VAR(metaball_max_alpha)         \
  VAR(metaball_min_alpha)

#define VAR(a) static int a##VarId;
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR

Metaballs::~Metaballs()
{
  for (int i = 0; i < updateJobs.size(); ++i)
    threadpool::wait(&updateJobs[i]);
}

const uint32_t MAX_GRID_SIZE = 64;

void Metaballs::initShader()
{
  shMat.reset();
  shMat.reset(new_shader_material_by_name(transparent ? "metaballs_refraction" : "metaballs_simple"));
  shMat->addRef();
  shElem = shMat->make_elem();
}

Metaballs::Metaballs()
{
  initShader();

  const int MAX_VERTICES_COUNT = MAX_GRID_SIZE * MAX_GRID_SIZE * MAX_GRID_SIZE * 6 * 2;
  vb.reset(d3d::create_vb(MAX_VERTICES_COUNT * sizeof(Point3), SBCF_DYNAMIC, "metaball"));
  ib.reset(d3d::create_ib(MAX_VERTICES_COUNT, SBCF_DYNAMIC));
  updateJobs.resize(2);

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_OPTIONAL_LIST
#undef VAR
}

void Metaballs::enableTransparency(bool enable)
{
  if (enable == transparent)
    return;
  transparent = enable;
  initShader();
}

void Metaballs::MetaballsUpdateJob::setGridSize(uint32_t grid_size)
{
  G_ASSERT(MAX_GRID_SIZE >= grid_size);
  gridSize = min(grid_size, MAX_GRID_SIZE);
  grid.resize(gridSize * gridSize * gridSize);
  gradGrid.assign(gridSize * gridSize * gridSize, Point4());
}

void Metaballs::MetaballsUpdateJob::updateGrid()
{
  // TIME_PROFILE(metaball_update_grid);
  grid.assign(gridSize * gridSize * gridSize, 0);

  int loweringThreshold = floorf(loweringLevel / gridScale);
  Point3 nodePos = Point3(-1, -1, -1);
  for (int i = 0, idx = 0; i < gridSize; ++i, nodePos.x += 2.f / gridSize)
  {
    nodePos.y = -1;
    for (int j = 0; j < gridSize; ++j, nodePos.y += 2.f / gridSize)
    {
      nodePos.z = -1;
      for (int k = 0; k < gridSize; ++k, nodePos.z += 2.f / gridSize, idx++)
      {
        if (loweringThreshold > j)
          continue;
        float value = 0;
        for (int ball_idx = 0; ball_idx < balls.size(); ++ball_idx)
          value += balls[ball_idx].w / ((Point3::xyz(balls[ball_idx]) - nodePos).lengthSq() + 1e-5);
        grid[idx] = value;
      }
    }
  }
  for (int i = 1; i < gridSize - 1; ++i)
  {
    for (int j = 1; j < gridSize - 1; ++j)
    {
      for (int k = 1; k < gridSize - 1; ++k)
      {
        gradGrid[(i * gridSize + j) * gridSize + k] =
          Point4(grid[((i - 1) * gridSize + j) * gridSize + k] - grid[((i + 1) * gridSize + j) * gridSize + k],
            grid[(i * gridSize + j - 1) * gridSize + k] - grid[(i * gridSize + j + 1) * gridSize + k],
            grid[(i * gridSize + j) * gridSize + k - 1] - grid[(i * gridSize + j) * gridSize + k + 1],
            grid[(i * gridSize + j) * gridSize + k]);
      }
    }
  }
}

void Metaballs::MetaballsUpdateJob::getVec4(int index, int i, int j, int k, Point4 &p)
{
  j += (index & 4) >> 2;
  k += (index & 2) >> 1;
  i += ((index & 2) >> 1) != (index & 1);
  p.w = grid[(i * gridSize + j) * gridSize + k];
}

void Metaballs::MetaballsUpdateJob::updateVerts(int i, int j, int k, int idx1, int idx2)
{
  int vvIdx = idx1 % 8;
  vertList[idx1] = vertexInterpolation(vv[vvIdx], vv[idx2]);

  gradients[vvIdx] = getGradient(vvIdx, i, j, k);
  gradients[idx2] = getGradient(idx2, i, j, k);

  gradList[idx1] = vertexInterpolation(gradients[vvIdx], gradients[idx2]);
}

void Metaballs::MetaballsUpdateJob::marchingCubes(int i, int j, int k)
{
  int cubeindex = 0;

  for (int ind = 0; ind < 8; ind++)
    getVec4(ind, i, j, k, vv[ind]);

  // check which vertices are inside the surface
  for (int idx = 0; idx < 8; ++idx)
    if (vv[idx].w < threshold.get())
      cubeindex |= 1 << idx;

  // cube is entirely out of the surface
  if (edgeTable[cubeindex] == 0)
    return;

  /* Find the vertices where the surface intersects the cube */

  const int EDGES_COUNT = 12;
  const int edgeStart[EDGES_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  const int edgeEnd[EDGES_COUNT] = {1, 2, 3, 0, 5, 6, 7, 4, 4, 5, 6, 7};

  for (int idx = 0; idx < EDGES_COUNT; ++idx)
    if (edgeTable[cubeindex] & (1 << idx))
      updateVerts(i, j, k, edgeStart[idx], edgeEnd[idx]);

  /* create triangles */

  const Point3 vertexShift(0.5 * gridScale, 0.5 * gridScale - loweringLevel, 0.5 * gridScale);
  for (int ind = 0; triTable[cubeindex][ind] != -1; ind += 3)
  {
    addVertex(vertList[triTable[cubeindex][ind]] * gridScale + vertexShift, gradList[triTable[cubeindex][ind]], i, j, k);
    addVertex(vertList[triTable[cubeindex][ind + 1]] * gridScale + vertexShift, gradList[triTable[cubeindex][ind + 1]], i, j, k);
    addVertex(vertList[triTable[cubeindex][ind + 2]] * gridScale + vertexShift, gradList[triTable[cubeindex][ind + 2]], i, j, k);

    numTriangles++;
  }
}

void Metaballs::MetaballsUpdateJob::addVertex(const Point3 &pos, const Point3 &normal, int x, int y, int z)
{
  int idx = vertices.size();
  for (int i = max(x - 1, 0); i < min(x + 1, (int)gridSize) && idx == vertices.size(); ++i)
    for (int j = max(y - 1, 0); j < min(y + 1, (int)gridSize) && idx == vertices.size(); ++j)
      for (int k = max(z - 1, 0); k < min(z + 1, (int)gridSize) && idx == vertices.size(); ++k)
      {
        int cubeIdx = (i * gridSize + j) * gridSize + k;
        for (const int v : verticesPerCubes[cubeIdx])
          if ((pos - vertices[v]).lengthSq() < 1e-10)
          {
            idx = v;
            break;
          }
      }
  if (idx == vertices.size())
  {
    vertices.push_back(pos);
    normals.push_back(normal);
    normalsCount.push_back(1);
    verticesPerCubes[(x * gridSize + y) * gridSize + z].push_back(idx);
  }
  else
  {
    normals[idx] += normal;
    ++normalsCount[idx];
  }
  indices.push_back(idx);
}

void Metaballs::MetaballsUpdateJob::update()
{
  if (balls.empty())
    return;
  // TIME_PROFILE(metaball_update);

  updateGrid();

  numTriangles = 0;
  vertData.clear();
  indices.clear();
  normalsCount.clear();
  vertices.clear();
  normals.clear();
  verticesPerCubes.assign(gridSize * gridSize * gridSize, eastl::vector<uint16_t>());
  for (int v = 0; v < 8; ++v)
    vv[v].x = ((v & 2) >> 1) != (v & 1);
  for (int x = 0; x < gridSize - 1; x++)
  {
    for (int v = 0; v < 8; ++v)
      vv[v].y = (v & 4) >> 2;
    for (int y = 0; y < gridSize - 1; y++)
    {
      for (int v = 0; v < 8; ++v)
        vv[v].z = (v & 2) >> 1;
      for (int z = 0; z < gridSize - 1; z++)
      {
        marchingCubes(x, y, z);
        for (int v = 0; v < 8; ++v)
          vv[v].z++;
      }
      for (int v = 0; v < 8; ++v)
        vv[v].y++;
    }
    for (int v = 0; v < 8; ++v)
      vv[v].x++;
  }

  for (int i = 0; i < vertices.size(); ++i)
  {
    vertData.push_back(vertices[i] + position);
    vertData.push_back(normalize(normals[i] / normalsCount[i]));
  }
}

Point3 Metaballs::MetaballsUpdateJob::vertexInterpolation(const Point4 &p1, const Point4 &p2)
{
  //  this is case where the resolution is too low anyway
  if (fabs(threshold.get() - p1.w) < 0.00001)
    return Point3::xyz(p1);
  if (fabs(threshold.get() - p2.w) < 0.00001)
    return Point3::xyz(p2);
  if (fabs(p1.w - p2.w) < 0.00001)
    return Point3::xyz(p1);

  float mu = (threshold.get() - p1.w) / (p2.w - p1.w);

  return Point3::xyz(p1 + mu * (p2 - p1));
}

Point4 Metaballs::MetaballsUpdateJob::getGradient(int index, int i, int j, int k)
{
  j += (index & 4) >> 2;
  k += (index & 2) >> 1;
  i += ((index & 2) >> 1) != (index & 1);
  return gradGrid[(i * gridSize + j) * gridSize + k];
}

void Metaballs::render()
{
  if (balls.empty())
    return;

  MetaballsUpdateJob &currentJob = updateJobs[currentUpdateJob];

  TIME_D3D_PROFILE(metaball);
  ShaderGlobal::set_color4(metaballs_albedoVarId, albedo.x, albedo.y, albedo.z, 0);
  ShaderGlobal::set_real(metaballs_smoothnessVarId, smoothness);
  ShaderGlobal::set_real(metaballs_reflectanceVarId, reflectance);
  ShaderGlobal::set_real(metaball_min_thicknessVarId, min_thickness);
  ShaderGlobal::set_real(metaball_max_thicknessVarId, max_thickness);
  ShaderGlobal::set_real(metaball_IoRVarId, IoR);
  ShaderGlobal::set_color4(metaball_mediumTintColorVarId, mediumTintColor.x, mediumTintColor.y, mediumTintColor.z, mediumTintColor.w);
  ShaderGlobal::set_real(metaball_isShellVarId, isShell);
  ShaderGlobal::set_real(metaball_emissionVarId, emission);
  ShaderGlobal::set_real(metaball_max_alphaVarId, max_alpha);
  ShaderGlobal::set_real(metaball_min_alphaVarId, min_alpha);

  shElem->setStates(0, true);
  d3d::setvsrc(0, vb.get(), 2 * sizeof(Point3));
  d3d::setind(ib.get());
  d3d::drawind(PRIM_TRILIST, 0, currentJob.numTriangles, 0);
  d3d::setvsrc(0, 0, 0);
}

void Metaballs::startUpdate()
{
  if (balls.empty())
    return;
  int nextJob = (currentUpdateJob + 1) % updateJobs.size();
  if (updateJobs[nextJob].done)
  {
    vb->updateData(0, updateJobs[nextJob].vertData.size() * sizeof(updateJobs[nextJob].vertData[0]),
      updateJobs[nextJob].vertData.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    ib->updateData(0, updateJobs[nextJob].indices.size() * sizeof(updateJobs[nextJob].indices[0]), updateJobs[nextJob].indices.data(),
      VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    int jobToUpdate = currentUpdateJob;
    currentUpdateJob = nextJob;
    updateJobs[jobToUpdate].setBalls(balls);
    updateJobs[jobToUpdate].setPos(position);
    updateJobs[jobToUpdate].setGridSize(gridSize);
    updateJobs[jobToUpdate].setScale(gridScale);
    updateJobs[jobToUpdate].setLoweringLevel(loweringLevel);
    threadpool::add(&updateJobs[jobToUpdate], threadpool::PRIO_NORMAL, false);
  }
}