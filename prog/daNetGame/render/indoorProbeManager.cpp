// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <memory/dag_framemem.h>
#include <render/lightProbe.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_vecMathCompatibility.h>
#include <render/toroidal_update.h>
#include "scene/dag_tiledScene.h"
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/daBfg/bfg.h>
#include <generic/dag_enumerate.h>
#include <daECS/core/ecsHash.h>

#include <shaders/indoor_probes_const.hlsli>
#include <render/lightProbeSpecularCubesContainer.h>
#include <render/indoorProbeManager.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_variableRateShading.h>
#include <render/world/depthBounds.h>
#include <util/dag_finally.h>
#include <render/indoorProbeScenes.h>

G_STATIC_ASSERT(MAX_ACTIVE_PROBES == LightProbeSpecularCubesContainer::INDOOR_PROBES);

static const uint32_t UPDATE_PROBE_THRESHOLD = 100;
static int indoor_probes_to_useVarId = -1;
static int indoor_probes_grid_y_centerVarId = -1;
static int indoor_probes_grid_xz_centerVarId = -1;
static int indoor_probes_max_distance_or_disabledVarId = -1; // if negative, indoor probes are disabled
static int indoor_probe_visible_pixels_count_const_no = -1;

CONSOLE_BOOL_VAL("render", indoor_probes, true);
CONSOLE_FLOAT_VAL("render", indoor_probes_max_distance, 70.0f);
CONSOLE_BOOL_VAL("indoor_probe", use_occlsuion, false);
CONSOLE_BOOL_VAL("indoor_probe", force_clusterization, false);

IndoorProbeManager::~IndoorProbeManager() { ShaderGlobal::set_real(indoor_probes_max_distance_or_disabledVarId, -1); }

IndoorProbeManager::IndoorProbeManager(LightProbeSpecularCubesContainer *container)
{
  cubesContainer = container;
  toroidalInfo.texSize = GRID_SIDE;
  lastY = -1e10;
  initBuffers();
  indoor_probes_to_useVarId = get_shader_variable_id("indoor_probes_to_use", true);
  indoor_probes_grid_y_centerVarId = get_shader_variable_id("indoor_probes_grid_y_center", true);
  indoor_probes_grid_xz_centerVarId = get_shader_variable_id("indoor_probes_grid_xz_center", true);
  indoor_probes_max_distance_or_disabledVarId = get_shader_variable_id("indoor_probes_max_distance_or_disabled", true);
  indoor_probe_visible_pixels_count_const_no =
    ShaderGlobal::get_int(get_shader_variable_id("indoor_probe_visible_pixels_count_const_no"));

  if (d3d::get_driver_desc().caps.hasVariableRateShadingBy4)
    debug("Indoor probes: hasVariableRateShadingBy4");
  else if (d3d::get_driver_desc().caps.hasVariableRateShading)
    debug("Indoor probes: hasVariableRateShading");
  else
    debug("Indoor probes: No VRS available");
  if (depth_bounds_enabled())
    debug("Indoor probes: Depth bounds are enabled");
  else
    debug("Indoor probes: Depth bounds are disabled");

  cullingDoneEvent.reset(d3d::create_event_query());
  cullingMat = new_shader_material_by_name("indoor_probes_gpu_culling");
  if (cullingMat)
    cullingElem = cullingMat->make_elem();
}

void IndoorProbeManager::initBuffers()
{
  indoorActiveProbesData =
    dag::buffers::create_persistent_cb(MAX_ACTIVE_PROBES * 4 + QUARTER_OF_ACTIVE_PROBES, "indoor_active_probes_data");
  indoorVisibleProbesData =
    dag::buffers::create_persistent_cb(NON_CELL_PROBES_COUNT * 4 + NON_CELL_PROBES_COUNT / 4, "indoor_visible_probes_data");
  cellClusters =
    dag::buffers::create_ua_sr_structured(sizeof(uint32_t) * 4, GRID_SIDE * GRID_SIDE * GRID_HEIGHT, "indoor_probe_cells_clusters");
  cellClusters.setVar();
  clusterizator.reset(new_compute_shader("light_probes_clusterization_cs", true));
  indoorActiveProbesStaging = dag::create_sbuffer(sizeof(uint32_t) * 4, MAX_ACTIVE_PROBES * 4 + QUARTER_OF_ACTIVE_PROBES,
    SBCF_STAGING_BUFFER, 0, "indoor_active_probes_staging"); // our dx11 drv create staging only if it RW
}

void IndoorProbeManager::fillCellsBuffer()
{
  MatrixLocalVector matrices(activeProbeIds.size());
  UintLocalVector shapeTypes(activeProbeIds.size());

  for (int i = 0; i < activeProbeIds.size(); ++i)
  {
    shapeTypes[i] = ENVI_PROBE_CUBE_TYPE;

    if (activeProbeIds[i] == -1)
    {
      matrices[i] = mat44f();
      continue;
    }

    auto sceneData = shapesContainer->getSceneDataForProbeIdx(activeProbeIds[i]);
    if (sceneData == nullptr)
    {
      matrices[i] = mat44f();
      continue;
    }

    matrices[i] = sceneData->scenePtr->getNode(probeIdxToNodeIdx[activeProbeIds[i]]);
    shapeTypes[i] = sceneData->shapeType;
  }

  UintLocalVector activeProbesIndices(activeProbeIds.size());
  for (int i = 0; i < activeProbeIds.size(); ++i)
    activeProbesIndices[i] = activeProbeIds[i] != -1 ? activeProbeIds[i] : activeProbes.size();

  Point4LocalVector dataToShader = packMatricesAndProbesData(activeProbesIndices, matrices, shapeTypes, MAX_ACTIVE_PROBES);

  indoorActiveProbesStaging->updateData(0, dataToShader.size() * sizeof(dataToShader[0]), dataToShader.data(), 0);
  indoorActiveProbesStaging->copyTo(indoorActiveProbesData.getBuf(), 0, 0, dataToShader.size() * sizeof(dataToShader[0]));

  d3d::resource_barrier({indoorActiveProbesData.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

  {
    TIME_D3D_PROFILE(indoor_probs_cells_clustering)
    d3d::set_rwbuffer(STAGE_CS, 0, cellClusters.getBuf());
    clusterizator->dispatch(1, 1, GRID_SIDE);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  }

  d3d::resource_barrier({cellClusters.getBuf(), RB_RO_COPY_SOURCE});
}

void IndoorProbeManager::initLightProbes()
{
  if (shapesContainer == nullptr || shapesContainer->getAllNodesCount() == 0)
    return;

  const uint32_t probesOnLevel = min(shapesContainer->getAllNodesCount(), static_cast<uint32_t>(MAX_ACTIVE_PROBES));

  activeProbes.resize(probesOnLevel);
  activeProbesUsageCounter.assign(probesOnLevel, 0);
  activeProbeIds.assign(probesOnLevel, -1);
  activeProbesState.assign(probesOnLevel, PROBE_IN_PROGRESS);
  for (LightProbe &probe : activeProbes)
    probe.init(cubesContainer, false);

  for (int i = 0; i < shapesContainer->sceneDatas.size(); i++)
  {
    for (const scene::node_index &nodeIdx : *shapesContainer->sceneDatas[i].scenePtr)
    {
      TMatrix4_vec4 matrix = reinterpret_cast<const TMatrix4_vec4 &>(shapesContainer->sceneDatas[i].scenePtr->getNode(nodeIdx));
      probeIdxToNodeIdx.push_back(nodeIdx);
      probesPositions.push_back(Point3::xyz(matrix.getrow(3)));
    }
  }

  uint32_t allProbesOnLevel = shapesContainer->getAllNodesCount();
  allIndoorProbeBoxes = dag::buffers::create_persistent_sr_structured(sizeof(Point4), allProbesOnLevel * 3, "all_indoor_probe_boxes");
  dag::Vector<Point4> allIndoorProbeBoxesData;
  allIndoorProbeBoxesData.reserve(allProbesOnLevel * 3);
  allIndoorProbePosAndCubes =
    dag::buffers::create_persistent_sr_structured(sizeof(Point4), allProbesOnLevel, "all_indoor_probe_pos_and_cubes");
  dag::Vector<Point4> allIndoorProbePosAndCubesData;
  allIndoorProbePosAndCubesData.reserve(allProbesOnLevel);

  uint32_t offset = 0;
  for (int sceneIdx = 0; sceneIdx < shapesContainer->sceneDatas.size(); sceneIdx++)
  {
    auto scene = shapesContainer->sceneDatas[sceneIdx].scenePtr.get();
    for (const auto [i, nodeIdx] : enumerate(scene->getBaseSimpleScene())) // using SimpleScene to help compiler with begin/end
    {
      const mat44f_const &matrix = reinterpret_cast<const mat44f_const &>(scene->getNode(nodeIdx));
      for (int j = 0; j < 3; ++j)
      {
        Point3_vec4 axis;
        v_st(&axis.x, matrix.col[j]);
        real axisLength = axis.length();
        if (axisLength > 0)
          axis /= axisLength;
        else
          logerr("Node %d has a side with 0 length!", nodeIdx);
        allIndoorProbeBoxesData.push_back(Point4::xyzV(axis, axisLength));
      }
      allIndoorProbePosAndCubesData.push_back(Point4::xyzV(probesPositions[offset + i], 0)); // Make cube ID valid
    }
    offset += scene->getNodesCount();
  }

  allIndoorProbeBoxes->updateData(0, data_size(allIndoorProbeBoxesData), allIndoorProbeBoxesData.data(), VBLOCK_WRITEONLY);
  allIndoorProbePosAndCubes->updateData(0, data_size(allIndoorProbePosAndCubesData), allIndoorProbePosAndCubesData.data(),
    VBLOCK_WRITEONLY);
  readbackBuffer =
    dag::buffers::create_ua_structured_readback(sizeof(uint32_t), allProbesOnLevel, "indoor_probe_readback", d3d::buffers::Init::Zero);
  needsToReadBackCulling = false;
  framesSinceLastCulling = GPU_CULLING_FREQUENCY;

  registerNodes(allProbesOnLevel);
}

void IndoorProbeManager::resetScene(eastl::unique_ptr<IndoorProbeScenes> &&scene_ptr)
{
  G_ASSERTF_AND_DO(!shapesContainer, unloadScene(), "Previous scene wasn't unloaded!");
  shapesContainer.swap(scene_ptr);
  initLightProbes();
  toroidalInfo = ToroidalHelper();
  toroidalInfo.texSize = GRID_SIDE;
}

eastl::unique_ptr<IndoorProbeScenes> IndoorProbeManager::unloadScene()
{
  tryStartUpdateNode = {};
  cpuCheckNode = {};
  completePreviousFrameReadbacksNode = {};
  createVisiblePixelsCountNode = {};
  calculateVisiblePixelsCountNode = {};
  initiateVisiblePixelsCountReadbackNode = {};
  setVisibleBoxesDataNode = {};

  probeIdxToNodeIdx.clear();
  probesPositions.clear();

  allIndoorProbeBoxes.close();
  allIndoorProbePosAndCubes.close();
  readbackBuffer.close();
  indoorProbeVisibilityMask.close();

  return eastl::move(shapesContainer);
}

static bool is_point_in_capsule(const vec3f &local_pos, float bbox_height, float width)
{
  float half_height = (bbox_height - width) * 0.5;
  vec3f a = v_make_vec3f(0.0f, 0.0f, half_height);
  vec3f b = v_make_vec3f(0.0f, 0.0f, -half_height);

  vec3f pa = v_sub(local_pos, a);
  vec3f ba = v_sub(b, a);

  float h = clamp(v_extract_x(v_dot3(pa, ba)) / v_extract_x(v_dot3(ba, ba)), 0.0f, 1.0f);
  return v_extract_x(v_length3(v_sub(pa, v_mul(ba, v_splats(h))))) < width * 0.5;
}

static bool is_point_in_box(const vec3f &local_pos, const vec3f &box_size)
{
  vec3f distanceVector = v_sub(v_abs(local_pos), v_mul(box_size, v_splats(0.5)));
  vec3f signedDistance = v_max(v_perm_xxxx(distanceVector), v_max(v_perm_yyyy(distanceVector), v_perm_zzzz(distanceVector)));
  return v_extract_x(signedDistance) < 0;
}

static bool is_point_in_cylinder(const vec3f &local_pos, float bbox_height, float width)
{
  float half_height = bbox_height * 0.5;
  vec3f a = v_make_vec3f(0.0f, 0.0f, half_height);
  vec3f b = v_make_vec3f(0.0f, 0.0f, -half_height);

  vec3f pa = v_sub(local_pos, a);
  vec3f ba = v_sub(b, a);

  float h = v_extract_x(v_dot3(pa, ba)) / v_extract_x(v_dot3(ba, ba));
  return h < 0 ? false : (h > 1 ? false : (v_extract_x(v_length3(v_sub(pa, v_mul(ba, v_splats(h))))) < width * 0.5));
}

static bool is_point_in_shape(const vec3f &world_pos, const mat44f &matrix4x4, uint8_t shape_type)
{
  vec3f boxX = matrix4x4.col0;
  vec3f boxY = matrix4x4.col1;
  vec3f boxZ = matrix4x4.col2;
  vec3f probePos = matrix4x4.col3;
  vec3f boxSize = v_sqrt(v_perm_xycw(v_perm_xbzw(v_length3_sq(boxX), v_length3_sq(boxY)), v_length3_sq(boxZ)));
  boxX = v_div(boxX, v_splat_x(boxSize));
  boxY = v_div(boxY, v_splat_y(boxSize));
  boxZ = v_div(boxZ, v_splat_z(boxSize));

  vec3f localPos = v_sub(world_pos, probePos);
  mat33f matrixTrans;
  v_mat33_transpose(matrixTrans, boxX, boxY, boxZ);
  localPos = v_mat33_mul_vec3(matrixTrans, localPos);

  float width = min(v_extract_x(boxSize), v_extract_y(boxSize));
  float height = v_extract_z(boxSize);

  switch (shape_type)
  {
    case ENVI_PROBE_CUBE_TYPE: return is_point_in_box(localPos, boxSize);

    case ENVI_PROBE_CAPSULE_TYPE: return is_point_in_capsule(localPos, height, width);

    case ENVI_PROBE_CYLINDER_TYPE: return is_point_in_cylinder(localPos, height, width);

    default: return false;
  }
}

bool IndoorProbeManager::isWorldPosInIndoorProbe(const Point3 &world_pos) const
{
  if (shapesContainer == nullptr || shapesContainer->getAllNodesCount() == 0)
    return false;

  for (int sceneIdx = 0; sceneIdx < shapesContainer->sceneDatas.size(); sceneIdx++)
  {
    auto scene = shapesContainer->sceneDatas[sceneIdx].scenePtr.get();
    auto shapeType = shapesContainer->sceneDatas[sceneIdx].shapeType;

    for (const scene::node_index &nodeIdx : *scene)
    {
      const mat44f &matrix4x4 = scene->getNode(nodeIdx);
      vec3f worldPos = v_make_vec4f(world_pos.x, world_pos.y, world_pos.z, 0);
      if (is_point_in_shape(worldPos, matrix4x4, shapeType))
        return true;
    }
  }
  return false;
}

void IndoorProbeManager::ensureDebugBuffersExist()
{
  if (indoorProbeVisibilityMask)
    return;
  uint32_t allProbesOnLevel = shapesContainer->getAllNodesCount();
  indoorProbeVisibilityMask =
    dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), (allProbesOnLevel + 31) / 32, "indoor_probe_visibility_mask");
}

IndoorProbeManager::Point4LocalVector IndoorProbeManager::packMatricesAndProbesData(
  dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices, dag::ConstSpan<uint32_t> shapeTypes, const int max_count)
{
  Point4LocalVector dataToShader(4 * max_count + max_count / 4, Point4(0, 0, 0, INVALID_CUBE_ID));
  for (int i = 0; i < matrices.size(); ++i)
  {
    // We need this cast only for convenience, to be able to index the columns of the matrix dynamically.
    // It looks like we can't do that for mat44f only for mat44f_const.
    const mat44f_const &matrix = reinterpret_cast<const mat44f_const &>(matrices[i]);
    for (int j = 0; j < 3; ++j)
    {
      Point3_vec4 axis;
      v_st(&axis.x, matrix.col[j]);
      real axisLength = axis.length();
      if (axisLength > 0)
      {
        axis /= axisLength;
      }
      dataToShader[i * 3 + j].set_xyzV(axis, axisLength);
    }
  }
  Point4LocalVector invalid_value(max_count / 4,
    Point4(INVALID_ENVI_PROBE_TYPE, INVALID_ENVI_PROBE_TYPE, INVALID_ENVI_PROBE_TYPE, INVALID_ENVI_PROBE_TYPE));
  eastl::copy(invalid_value.begin(), invalid_value.end(), dataToShader.data() + 4 * max_count);
  for (int i = 0; i < probe_indices.size(); ++i)
  {
    const uint32_t activeSlot =
      eastl::distance(activeProbeIds.begin(), eastl::find(activeProbeIds.begin(), activeProbeIds.end(), probe_indices[i]));
    if (activeSlot == activeProbeIds.size() || activeProbeIds[activeSlot] == -1 || !activeProbes[activeSlot].isValid())
      continue;
    const Point3 probePos = probesPositions[activeProbeIds[activeSlot]];
    dataToShader[i + 3 * max_count] = Point4(probePos.x, probePos.y, probePos.z, activeProbes[activeSlot].getCubeIndex());
    dataToShader[i / 4 + 4 * max_count][i % 4] = shapeTypes[i];
  }

  return dataToShader;
}

void IndoorProbeManager::setVisibleBoxesData(
  dag::ConstSpan<uint32_t> probe_indices, dag::ConstSpan<mat44f> matrices, dag::ConstSpan<uint32_t> shape_types)
{
  const Point4LocalVector dataToShader = packMatricesAndProbesData(probe_indices, matrices, shape_types, NON_CELL_PROBES_COUNT);
  indoorVisibleProbesData.getBuf()->updateData(0, dataToShader.size() * sizeof(dataToShader[0]), dataToShader.data(), VBLOCK_DISCARD);
}

void IndoorProbeManager::setActiveProbe(int probe_idx, uint32_t active_slot)
{
  activeProbeIds[active_slot] = probe_idx;
  activeProbesState[active_slot] = PROBE_IN_PROGRESS;
  if (probe_idx == -1)
    return;
  activeProbesUsageCounter[active_slot] = 0;
  activeProbes[active_slot].setPosition(probesPositions[probe_idx]);
  activeProbes[active_slot].update();
}

bool IndoorProbeManager::tryToActivateProbe(const uint32_t probe_index)
{
  const auto freeProbeId = eastl::find(activeProbeIds.begin(), activeProbeIds.end(), -1);
  if (freeProbeId != activeProbeIds.end())
  {
    setActiveProbe(probe_index, eastl::distance(activeProbeIds.begin(), freeProbeId));
    return true;
  }
  const auto lruProbeId = eastl::max_element(activeProbesUsageCounter.begin(), activeProbesUsageCounter.end());
  if (*lruProbeId > UPDATE_PROBE_THRESHOLD)
  {
    setActiveProbe(probe_index, eastl::distance(activeProbesUsageCounter.begin(), lruProbeId));
    return true;
  }
  return false;
}

void IndoorProbeManager::updateActiveProbes(const UintLocalVector &probe_indices)
{
  for (const uint32_t probeIndex : probe_indices)
  {
    const auto activeProbeId = eastl::find(activeProbeIds.begin(), activeProbeIds.end(), probeIndex);
    if (activeProbeId != activeProbeIds.end())
      activeProbesUsageCounter[eastl::distance(activeProbeIds.begin(), activeProbeId)] = 0;
  }
  bool someProbesChanged = false;
  for (const uint32_t probeIndex : probe_indices)
    if (eastl::find(activeProbeIds.begin(), activeProbeIds.end(), probeIndex) == activeProbeIds.end())
      someProbesChanged = tryToActivateProbe(probeIndex) || someProbesChanged;
  if (someProbesChanged)
    fillCellsBuffer();
}

void IndoorProbeManager::update(const Point3 &new_origin)
{
  ToroidalGatherCallback::RegionTab regions;
  ToroidalGatherCallback toroidalCb(regions);
  toroidal_update(IPoint2(new_origin.x / GRID_CELL_SIZE, new_origin.z / GRID_CELL_SIZE), toroidalInfo, 3, toroidalCb);
  const float currentY = static_cast<int>(new_origin.y / GRID_CELL_HEIGHT) * GRID_CELL_HEIGHT;
  ShaderGlobal::set_color4_fast(indoor_probes_grid_xz_centerVarId, (float)toroidalInfo.curOrigin.x * GRID_CELL_SIZE,
    (float)toroidalInfo.curOrigin.y * GRID_CELL_SIZE, 0, 0);
  ShaderGlobal::set_real_fast(indoor_probes_grid_y_centerVarId, currentY);

  bool probesActiveStatusChanged = false;
  for (int i = 0; i < activeProbesState.size(); ++i)
  {
    if (activeProbesState[i] == PROBE_IN_PROGRESS && activeProbes[i].isValid())
    {
      for (uint32_t &state : activeProbesState)
        if (state == LAST_UPDATED_PROBE)
          state = ACTIVE_PROBE;
      activeProbesState[i] = LAST_UPDATED_PROBE;
      probesActiveStatusChanged = true;
    }
  }

  if (regions.size() || probesActiveStatusChanged || currentY != lastY)
    fillCellsBuffer();
  lastY = currentY;
}

using CpuIndices = dag::RelocatableFixedVector<uint32_t, NON_CELL_PROBES_COUNT>;
using CpuMatrices = dag::RelocatableFixedVector<mat44f, NON_CELL_PROBES_COUNT>;

auto IndoorProbeManager::cpuCheck(
  Occlusion *occlusion, const Point3 &view_pos, const TMatrix4 &view_mat, const Driver3dPerspective &persp)
{
  CpuMatrices cpuMatrices;
  CpuIndices cpuIndices;
  CpuIndices cpuShapeTypes;
  bool useClusterization = force_clusterization;
  uint32_t sceneIdx = 0;
  {
    // Optimize GPU by checking if there are more than 4 active nodes visible.
    TIME_PROFILE(cpu_indoor_probe_check)
    auto storeVisibleBox = [this, &cpuMatrices, &cpuIndices, &useClusterization, &cpuShapeTypes, &sceneIdx](scene::node_index idx,
                             mat44f_cref m, vec4f dist) {
      G_UNUSED(dist);
      if (cpuMatrices.size() < NON_CELL_PROBES_COUNT)
      {
        auto sceneData = &shapesContainer->sceneDatas[sceneIdx];
        uint32_t cubeIdx = sceneData->nodeIdxToProbeIdx[idx];
        const uint32_t activeSlot =
          eastl::distance(activeProbeIds.begin(), eastl::find(activeProbeIds.begin(), activeProbeIds.end(), cubeIdx));
        if (activeSlot == activeProbeIds.size() || activeProbeIds[activeSlot] == -1 || !activeProbes[activeSlot].isValid())
          return;
        cpuMatrices.push_back(m);
        cpuIndices.push_back(cubeIdx);
        cpuShapeTypes.push_back(sceneData->shapeType);
      }
      else
        useClusterization = true;
    };

    TMatrix4 projMat = matrix_perspective(persp.wk, persp.hk, persp.zn, indoor_probes_max_distance);
    TMatrix4_vec4 globtm = view_mat * projMat;
    const vec4f vpos_distscale = v_make_vec4f(view_pos.x, view_pos.y, view_pos.z, 1);
    if (occlusion && use_occlsuion)
    {
      for (auto &sceneData : shapesContainer->sceneDatas)
      {
        sceneData.scenePtr->frustumCull<false /* use_flags */, true /* use_pools */, true /* use_occlusion */>((mat44f &)globtm,
          vpos_distscale, 0, 0, occlusion, storeVisibleBox);

        ++sceneIdx;
      }
    }
    else
    {
      for (auto &sceneData : shapesContainer->sceneDatas)
      {
        sceneData.scenePtr->frustumCull<false /* use_flags */, true /* use_pools */, false /* use_occlusion */>((mat44f &)globtm,
          vpos_distscale, 0, 0, occlusion, storeVisibleBox);

        ++sceneIdx;
      }
    }

    ShaderGlobal::set_int_fast(indoor_probes_to_useVarId, useClusterization);
  }
  ShaderGlobal::set_real(indoor_probes_max_distance_or_disabledVarId,
    cpuMatrices.size() > 0 && indoor_probes.get() ? indoor_probes_max_distance : -1.0f);

  return eastl::tuple<CpuMatrices, CpuIndices, CpuIndices, bool>{
    eastl::move(cpuMatrices), eastl::move(cpuIndices), eastl::move(cpuShapeTypes), useClusterization};
}

void IndoorProbeManager::completeCullingReadback()
{
  if (!needsToReadBackCulling)
    return;

  TIME_D3D_PROFILE(process_readback)

  struct IdCountPair
  {
    int id;
    uint32_t count;
  };
  dag::Vector<IdCountPair, framemem_allocator> readBackDataToSort;
  readBackDataToSort.reserve(shapesContainer->getAllNodesCount());
  if (auto data = lock_sbuffer<const uint32_t>(readbackBuffer.getBuf(), 0, shapesContainer->getAllNodesCount(), VBLOCK_READONLY))
  {
    for (int i = 0; i < shapesContainer->getAllNodesCount(); i++)
      readBackDataToSort.push_back({i, data[i]});
  }
  else
  {
    logerr("Failed to lock readback indoor probe visible pixel count buffer!");
    return;
  }
  stlsort::sort(readBackDataToSort.begin(), readBackDataToSort.end(), [](auto a, auto b) { return a.count > b.count; });

  {
    UintLocalVector cubeIndices;
    cubeIndices.reserve(MAX_ACTIVE_PROBES);
    for (int i = 0, end = min<int>(MAX_ACTIVE_PROBES, readBackDataToSort.size()); i < end; ++i)
    {
      if (readBackDataToSort[i].count == 0)
        break;
      cubeIndices.push_back(readBackDataToSort[i].id);
    }

    for (uint32_t &cnt : activeProbesUsageCounter)
      ++cnt;

    updateActiveProbes(cubeIndices);
  }

  // Debug feature: send indices of boxes that passed the visibility test
  // back to the GPU for highlighting them in the debug visualization
  if (indoorProbeVisibilityMask)
  {
    UintLocalVector indoorProbeVisibilityMaskData((readBackDataToSort.size() + 31) / 32, 0);
    for (int i = 0; i < readBackDataToSort.size(); i++)
    {
      if (readBackDataToSort[i].count > 0)
      {
        uint32_t arrayIndex = readBackDataToSort[i].id / 32;
        uint32_t maskIndex = readBackDataToSort[i].id % 32;
        indoorProbeVisibilityMaskData[arrayIndex] |= 1u << maskIndex;
      }
    }

    if (!indoorProbeVisibilityMaskData.empty())
      indoorProbeVisibilityMask->updateData(0, data_size(indoorProbeVisibilityMaskData), indoorProbeVisibilityMaskData.data(),
        VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }

  needsToReadBackCulling = false;
}

void IndoorProbeManager::registerNodes(uint32_t allProbesOnLevel)
{
  if (allProbesOnLevel == 0)
    return;

  // NOTE: we use a funny trick for conditional running of nodes:
  // If a node that creates a bool blob does an early return, that
  // blob will be false by default, so we'll skip consequent nodes
  // automatically.
  // This is why we do dontUseClusterization instead of useClusterization,
  // otherwise the default would be wrong.

  auto ns = dabfg::root() / "indoor_probes";

  tryStartUpdateNode = ns.registerNode("try_start_update_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    auto shouldUpdateHndl = registry.create("should_update", dabfg::History::No).blob<bool>().handle();

    return [this, shouldUpdateHndl](dabfg::multiplexing::Index multiplexing_index) {
      const bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      if (!isFirstIteration)
        return;

      // TODO: can this be hardcoded as allProbesOnLevel? Not sure,
      // maybe it changes during runtime.
      const bool anyBoxes = shapesContainer && (shapesContainer->getAllNodesCount() != 0);
      if (!anyBoxes)
        return;

      shouldUpdateHndl.ref() = true;

      framesSinceLastCulling++;
    };
  });

  using CpuIndices = dag::RelocatableFixedVector<uint32_t, NON_CELL_PROBES_COUNT>;
  using CpuMatrices = dag::RelocatableFixedVector<mat44f, NON_CELL_PROBES_COUNT>;

  cpuCheckNode = ns.registerNode("cpu_check_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    auto shouldUpdateHndl = registry.read("should_update").blob<bool>().handle();

    auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();

    auto cpuMatricesHndl = registry.create("cpu_matrices", dabfg::History::No).blob<CpuMatrices>().handle();
    auto cpuIndicesHndl = registry.create("cpu_indices", dabfg::History::No).blob<CpuIndices>().handle();
    auto cpuShapeTypesHndl = registry.create("cpu_shape_types", dabfg::History::No).blob<CpuIndices>().handle();
    auto dontUseClusterization = registry.create("dont_use_clusterization", dabfg::History::No).blob<bool>().handle();
    auto readbackReadyHndl = registry.create("readback_ready", dabfg::History::No).blob<bool>().handle();

    return [this, shouldUpdateHndl, cameraHndl, cpuMatricesHndl, cpuIndicesHndl, cpuShapeTypesHndl, dontUseClusterization,
             readbackReadyHndl]() {
      if (!shouldUpdateHndl.ref())
        return;

      auto [cpuMats, cpuInds, cpuShapes, useCluster] =
        cpuCheck(current_occlusion, cameraHndl.ref().viewItm.getcol(3), cameraHndl.ref().viewTm, cameraHndl.ref().jitterPersp);

      cpuMatricesHndl.ref() = eastl::move(cpuMats);
      cpuIndicesHndl.ref() = eastl::move(cpuInds);
      cpuShapeTypesHndl.ref() = eastl::move(cpuShapes);
      dontUseClusterization.ref() = !useCluster;

      readbackReadyHndl.ref() = d3d::get_event_query_status(cullingDoneEvent.get(), false);
    };
  });

  completePreviousFrameReadbacksNode =
    ns.registerNode("complete_previous_frame_readbacks_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto readbackReadyHndl = registry.read("readback_ready").blob<bool>().handle();
      auto shouldInitiateReadbackHndl = registry.create("should_initiate_readback", dabfg::History::No).blob<bool>().handle();

      // We don't modify these directly, but we can invalidate the stuff
      // that they represent in updateActiveProbes, so we need to
      // reflect that as a modification for correct ordering
      registry.modify("cpu_matrices").blob<CpuMatrices>();
      registry.modify("cpu_indices").blob<CpuIndices>();
      registry.modify("cpu_shape_types").blob<CpuIndices>();

      return [this, readbackReadyHndl, shouldInitiateReadbackHndl]() {
        if (!readbackReadyHndl.ref())
          return;

        completeCullingReadback();

        shouldInitiateReadbackHndl.ref() = framesSinceLastCulling > GPU_CULLING_FREQUENCY;
      };
    });

  createVisiblePixelsCountNode =
    ns.registerNode("create_visible_pixels_count_node", DABFG_PP_NODE_SRC, [allProbesOnLevel](dabfg::Registry registry) {
      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl = registry.create("visible_pixels_count", dabfg::History::No)
                                      .structuredBufferUa<uint32_t>(allProbesOnLevel)
                                      .atStage(dabfg::Stage::ALL_GRAPHICS)
                                      .useAs(dabfg::Usage::SHADER_RESOURCE)
                                      .handle();

      return [shouldInitiateReadbackHndl, visiblePixelsCountHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        uint32_t nullVec[] = {0, 0, 0, 0};
        d3d::clear_rwbufi(visiblePixelsCountHndl.get(), nullVec);
      };
    });

  calculateVisiblePixelsCountNode =
    ns.registerNode("calculate_visible_pixels_count", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl = registry.modify("visible_pixels_count")
                                      .buffer()
                                      .atStage(dabfg::Stage::ALL_GRAPHICS)
                                      .useAs(dabfg::Usage::SHADER_RESOURCE)
                                      .handle();

      // Water SSR modifies downsampled depth, but we kind of read it,
      // so we ought to do it before it gets changed. Otherwise, when
      // looking at a reflective surface inside a house through water,
      // probes would be culled away and not used :/
      registry.create("probes_ready_token", dabfg::History::No).blob<OrderingToken>();

      registry.requestRenderPass().depthRw("downsampled_depth");
      shaders::OverrideState state;
      state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
      registry.requestState().setFrameBlock("global_frame").enableOverride(depth_bounds_enabled() ? state : shaders::OverrideState{});

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                          .bindAsView<&CameraParams::viewTm>()
                          .bindAsProj<&CameraParams::jitterProjTm>()
                          .handle();

      return [this, shouldInitiateReadbackHndl, visiblePixelsCountHndl, cameraHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        if (depth_bounds_enabled())
        {
          Point2 zn_zfar = {cameraHndl.ref().noJitterPersp.zn, cameraHndl.ref().noJitterPersp.zf};
          // inv_linearizeZ
          float zMin = (zn_zfar.y / (indoor_probes_max_distance)-1) * zn_zfar.x / (zn_zfar.y - zn_zfar.x);
          api_set_depth_bounds(zMin, 1);
        }

        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, indoor_probe_visible_pixels_count_const_no, VALUE),
          visiblePixelsCountHndl.get());
        const bool hasUavInVSCapability = d3d::get_driver_desc().caps.hasUAVOnEveryStage;
        STATE_GUARD_NULLPTR(
          hasUavInVSCapability ? d3d::set_rwbuffer(STAGE_VS, indoor_probe_visible_pixels_count_const_no, VALUE) : false,
          visiblePixelsCountHndl.get());

        if (d3d::get_driver_desc().caps.hasVariableRateShadingBy4)
          d3d::set_variable_rate_shading(4, 4);
        else if (d3d::get_driver_desc().caps.hasVariableRateShading)
          d3d::set_variable_rate_shading(2, 2);

        cullingElem->setStates(0, true);
        d3d::draw(PRIM_TRILIST, 0, 12 * shapesContainer->getAllNodesCount());

        if (d3d::get_driver_desc().caps.hasVariableRateShading)
          d3d::set_variable_rate_shading(1, 1);
      };
    });

  initiateVisiblePixelsCountReadbackNode =
    ns.registerNode("initiate_visible_pixels_readback", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.executionHas(dabfg::SideEffects::External);

      auto shouldInitiateReadbackHndl = registry.read("should_initiate_readback").blob<bool>().handle();

      auto visiblePixelsCountHndl =
        registry.read("visible_pixels_count").buffer().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();

      return [this, shouldInitiateReadbackHndl, visiblePixelsCountHndl]() {
        if (!shouldInitiateReadbackHndl.ref())
          return;

        visiblePixelsCountHndl.view()->copyTo(readbackBuffer.getBuf());
        issue_readback_query(cullingDoneEvent.get(), readbackBuffer.getBuf());
        framesSinceLastCulling = 0;
        needsToReadBackCulling = true;
      };
    });

  setVisibleBoxesDataNode = ns.registerNode("set_visible_boxes_data", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);

    auto dontUseClusterizationHndl = registry.read("dont_use_clusterization").blob<bool>().handle();

    auto cpuMatricesHndl = registry.read("cpu_matrices").blob<CpuMatrices>().handle();
    auto cpuIndicesHndl = registry.read("cpu_indices").blob<CpuIndices>().handle();
    auto cpuShapeTypesHndl = registry.read("cpu_shape_types").blob<CpuIndices>().handle();

    // We consolidate probe visibility data here and upload it to the
    // GPU, so this node MUST be run before anything that uses probes
    // is rendered. In the future, this blob should probably become
    // creation of the indoorVisibleProbesData buffer.
    registry.modify("probes_ready_token").blob<OrderingToken>();

    return [this, dontUseClusterizationHndl, cpuMatricesHndl, cpuIndicesHndl, cpuShapeTypesHndl]() {
      if (!dontUseClusterizationHndl.ref())
        return;

      setVisibleBoxesData(cpuIndicesHndl.ref(), cpuMatricesHndl.ref(), cpuShapeTypesHndl.ref());
    };
  });
}

void IndoorProbeManager::invalidate()
{
  for (LightProbe &probe : activeProbes)
    probe.invalidate();
  activeProbesUsageCounter.assign(activeProbesUsageCounter.size(), 0);
  activeProbeIds.assign(activeProbeIds.size(), -1);
  toroidalInfo = ToroidalHelper();
  toroidalInfo.texSize = GRID_SIDE;
}

const scene::TiledScene *IndoorProbeManager::getEnviProbeBoxes() const
{
  return shapesContainer->getScene(ECS_HASH("envi_probe_box").hash);
}

const IndoorProbeScenes *IndoorProbeManager::getIndoorProbeScenes() const { return shapesContainer.get(); }
