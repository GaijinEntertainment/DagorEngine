#include <math/dag_frustum.h>
#include <perfMon/dag_statDrv.h>
#include <render/toroidal_update.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderMesh.h>
#include <util/dag_stlqsort.h>
#include <scene/dag_scene.h>
#include <gameRes/dag_gameResources.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <render/nodeBasedShader.h>
#include <osApiWrappers/dag_direct.h>
#include <daECS/core/componentTypes.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_lockSbuffer.h>

#include <gpuObjects/gpuObjects.h>
#include <gpuObjects/volumePlacer.h>
#include <math/dag_hlsl_floatx.h>
#include <gpuObjects/gpu_objects_const.hlsli>
#include <render/toroidal_update_regions.h>
#include <frustumCulling/frustumPlanes.h>

namespace gpu_objects
{

#define GLOBAL_VARS_LIST                    \
  VAR(gpu_objects_world_coord)              \
  VAR(gpu_objects_bounding_radius)          \
  VAR(gpu_objects_groups_count)             \
  VAR(gpu_objects_groups_bbox_offset)       \
  VAR(gpu_objects_cell_buffer_offset)       \
  VAR(gpu_objects_group_idx)                \
  VAR(gpu_objects_biomes_count)             \
  VAR(gpu_objects_seed)                     \
  VAR(gpu_objects_up_vector)                \
  VAR(gpu_objects_scale_rotate)             \
  VAR(gpu_objects_map_size_offset)          \
  VAR(gpu_objects_color_from)               \
  VAR(gpu_objects_color_to)                 \
  VAR(gpu_objects_rendinst_offset)          \
  VAR(gpu_objects_on_ri_placing_params)     \
  VAR(gpu_objects_num_for_placing)          \
  VAR(gpu_objects_num_per_rendinst)         \
  VAR(gpu_objects_rendinst_mesh_params)     \
  VAR(gpu_objects_slope_factor)             \
  VAR(gpu_objects_weights)                  \
  VAR(gpu_objects_gather_target_offset)     \
  VAR(gpu_objects_max_count_in_cell)        \
  VAR(gpu_objects_visible_cells)            \
  VAR(gpu_objects_ri_pool_id_offset)        \
  VAR(gpu_objects_place_on_water)           \
  VAR(gpu_objects_enable_displacement)      \
  VAR(gpu_objects_instance_buf)             \
  VAR(gpu_object_ints_to_clear)             \
  VAR(gpu_objects_count)                    \
  VAR(gpu_objects_no)                       \
  VAR(gpu_objects_gpu_instancing)           \
  VAR(gpu_objects_generation_state)         \
  VAR(gpu_objects_sum_area_step)            \
  VAR(gpu_object_rendinst_instances_count)  \
  VAR(gpu_objects_coast_range)              \
  VAR(gpu_objects_face_coast)               \
  VAR(gpu_objects_bomb_hole_point_radius_0) \
  VAR(gpu_objects_bomb_hole_point_radius_1) \
  VAR(gpu_objects_bomb_hole_point_radius_2) \
  VAR(gpu_objects_bomb_hole_point_radius_3)

#define VAR(a) int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

int biom_indexes_const_no = -1;
int gpu_objects_indirect_params_const_no = -1;
int gpu_objects_lod_offsets_buffer_const_no = -1;
int gpu_objects_generation_params_const_no = -1;

const uint32_t DISPATCH_THREAD_AXIS = 4;

constexpr int BBOX3F_SIZE_IN_INT = sizeof(bbox3f) / sizeof(int32_t);
constexpr int VEC4I_SIZE_IN_INT = sizeof(vec4i) / sizeof(int32_t);

struct ValidationParams
{
  float boundingBoxMinWidth{1.0f};
  int maxCellsCount{64};
  float maxObjectDensity{1.0f};
};
static ValidationParams ValidationParameterCheckLimits;

static bool do_parameter_check{false};

template <typename T>
constexpr T pow2(const T &t)
{
  return t * t;
}

ObjectManager::ObjectManager(const char *name, uint32_t ri_pool_id, int cell_tile, int cells_size_count, float cell_size,
  float bounding_sphere_radius, dag::ConstSpan<float> dist_sq_lod, const PlacingParameters &parameters) :
  assetName(name),
  riPoolOffset(ri_pool_id * rendinstgenrender::PER_DRAW_VECS_COUNT + 1),
  boundingSphereRadius(bounding_sphere_radius),
  parameters(parameters),
  cellTileOrig(-1)
{
  inCellPlacer.reset(new_compute_shader("gpu_objects_cs", true));
  counterAndBboxCleaner.reset(new_compute_shader("gpu_obj_clear_counter_and_bbox_cs", true));
  gatherMatrices.reset(new_compute_shader("gpu_objects_gather_matrices_cs", true));

  recreateGrid(cell_tile, cells_size_count, cell_size);

  updateQueue.get_container().reserve(cellsCount);

  if (!parameters.map.empty())
  {
    mapTexId = dag::get_tex_gameres(parameters.map, "gpu_objects_map");
    if (!mapTexId)
      logerr("Texture %s for gpu objects mask not found in resources.", parameters.map);
  }


  if (parameters.decal)
    layer = rendinst::LAYER_DECALS;
  else if (parameters.transparent)
    layer = rendinst::LAYER_TRANSPARENT;
  else if (parameters.distorsion)
    layer = rendinst::LAYER_DISTORTION;
  else
    layer = rendinst::LAYER_OPAQUE;

  numLods = min<int>(dist_sq_lod.size(), MAX_LODS);
  memcpy(distSqLod.data(), dist_sq_lod.data(), numLods * sizeof(dist_sq_lod[0]));

#if DAGOR_DBGLEVEL > 0
  validateParams();
#endif
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  biom_indexes_const_no = ShaderGlobal::get_int_fast(get_shader_variable_id("biom_indexes_const_no"));
  gpu_objects_indirect_params_const_no = ShaderGlobal::get_int_fast(get_shader_variable_id("gpu_objects_indirect_params_const_no"));
  gpu_objects_lod_offsets_buffer_const_no =
    ShaderGlobal::get_int_fast(get_shader_variable_id("gpu_objects_lod_offsets_buffer_const_no"));
  gpu_objects_generation_params_const_no =
    ShaderGlobal::get_int_fast(get_shader_variable_id("gpu_objects_generation_params_const_no"));
}

ObjectManager::ObjectManager(ObjectManager &&) = default;

void ObjectManager::setParameters(const PlacingParameters &params)
{
  Point2 oldWeights = parameters.weightRange;
  parameters = params;
  if (oldWeights != params.weightRange)
    recreateGrid(cellTile, cellsSideCount, cellSize);
  invalidate();
#if DAGOR_DBGLEVEL > 0
  validateParams();
#endif
}

ObjectManager::~ObjectManager() {}

void ObjectManager::recreateGrid(int cell_tile, int cells_size_count, float cell_size)
{
  cellTileOrig = cell_tile;
  cellDispatchTile = (cell_tile + DISPATCH_THREAD_AXIS - 1) / DISPATCH_THREAD_AXIS;
  cellTile = cellDispatchTile * DISPATCH_THREAD_AXIS;
  cellsSideCount = cells_size_count;
  cellSize = cell_size;
  cellsCount = pow2(cells_size_count);
  waitingForGpuData = false;
  maxObjectsCountInCell = pow2(cellTile);
  toroidalGrid.texSize = cellsSideCount;

  float weightMultiplier = parameters.weightRange.y - parameters.weightRange.x;
  if (weightMultiplier < 0.9999)
  {
    // buffer size will be based on weight, and a little bigger because random not very precise
    weightMultiplier = min(1.0, weightMultiplier * 1.5);
    maxObjectsCountInCell *= weightMultiplier;
    if (maxObjectsCountInCell == 0 && cell_tile != 0)
    {
      logerr("%s has too small weight, gpu objects with this type won't be generated on land.", assetName);
    }
  }

  gatheredBuffer.close();
  countersBuffer.close();
  bboxesBuffer.close();
  if (maxObjectsCountInCell > 0)
  {
    gatheredBuffer = dag::create_sbuffer(sizeof(Point4), maxObjectsCountInCell * cellsCount * ROWS_IN_MATRIX,
      SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_MAYBELOST, TEXFMT_A32B32G32R32F,
      String(0, "gathered_gpu_objects_%s", assetName));
    countersBuffer = dag::buffers::create_ua_byte_address_readback(cellsCount, String(0, "ObjectManagerCounts_%s", assetName),
      dag::buffers::Init::Zero);
    bboxesBuffer = dag::buffers::create_ua_structured_readback(sizeof(int32_t), BBOX3F_SIZE_IN_INT * cellsCount,
      String(0, "ObjectManagerCellsBboxes_%s", assetName));
    d3d::resource_barrier({countersBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  }

  makeMatricesOffsetsBuffer();

  size_t vectorSize = maxObjectsCountInCell > 0 ? cellsCount : 0;
  counters.assign(vectorSize, 0);
  cellsBboxes.resize(vectorSize);
  renderOrder.resize(vectorSize);
  appendOrder.resize(vectorSize);
  updateQueue.get_container().clear();

  updateBboxesFence.reset(d3d::create_event_query());
}

bool ObjectManager::dispatchNextCell()
{
  if (updateQueue.empty())
    return false;
  TIME_D3D_PROFILE(ObjectManagerPlacingCell)
  onLandPlacing();
  updateQueue.pop();
  return true;
}

void ObjectManager::makeMatricesOffsetsBuffer()
{
  matricesOffsetsBuffer.close();
  uint32_t size = maxObjectsCountInCell > 0 ? cellsCount : 0;
  matricesOffsetsBuffer = dag::create_sbuffer(sizeof(uint32_t) * 2, size + 1, SBCF_BIND_SHADER_RES | SBCF_MAYBELOST, TEXFMT_R32G32UI,
    String(0, "ObjectManager_MatricesOffsets_%s", assetName));
}

void ObjectManager::validateDisplacedGPUObjs(float displacement_tex_range)
{
  if (parameters.enableDisplacement && !texSizeValidated)
  {
    float gridSize = (cellsSideCount + 1) * cellSize; //+1 for the random noise in shader:
                                                      // coords += randValues.xy * 2*gpu_objects_world_coord.z;
    if (gridSize >= displacement_tex_range)
    {
      logerr("GPU object %s has displacement enabled, and its grid size is larger than the hmap_ofs_tex texture's range "
             "Grid size: %f, hmap_ofs_tex range: %f",
        this->assetName.c_str(), gridSize, displacement_tex_range);
    }
  }
  texSizeValidated = true;
}

void ObjectManager::setShaderVarsAndConsts()
{

  ShaderGlobal::set_real(gpu_objects_bounding_radiusVarId, boundingSphereRadius);
  ShaderGlobal::set_real(gpu_objects_groups_countVarId, cellDispatchTile);
  ShaderGlobal::set_real(gpu_objects_seedVarId, parameters.seed);
  ShaderGlobal::set_color4(gpu_objects_up_vectorVarId, parameters.upVector.x, parameters.upVector.y, parameters.upVector.z,
    parameters.inclineDelta);
  ShaderGlobal::set_color4(gpu_objects_scale_rotateVarId, parameters.scale.x, parameters.scale.y, parameters.rotate.x,
    parameters.rotate.y);
  ShaderGlobal::set_int(gpu_objects_ri_pool_id_offsetVarId, riPoolOffset);
  if (mapTexId)
  {
    ShaderGlobal::set_color4(gpu_objects_map_size_offsetVarId, parameters.mapSizeOffset.x, parameters.mapSizeOffset.y,
      parameters.mapSizeOffset.z, parameters.mapSizeOffset.w);
  }
  else
  {
    ShaderGlobal::set_int(gpu_objects_biomes_countVarId, parameters.biomes.size());
    if (parameters.biomes.size() > 0)
    {
      d3d::set_cs_const(biom_indexes_const_no, &parameters.biomes[0].x, parameters.biomes.size());
    }
  }
  ShaderGlobal::set_color4(gpu_objects_color_fromVarId, parameters.colorFrom);
  ShaderGlobal::set_color4(gpu_objects_color_toVarId, parameters.colorTo);
  ShaderGlobal::set_color4(gpu_objects_weightsVarId, parameters.weightRange.x, parameters.weightRange.y, maxObjectsCountInCell, 0);
  ShaderGlobal::set_color4(gpu_objects_slope_factorVarId, parameters.slopeFactor);
  ShaderGlobal::set_int(gpu_objects_place_on_waterVarId, static_cast<int>(parameters.placeOnWater));
  ShaderGlobal::set_int(gpu_objects_enable_displacementVarId, static_cast<int>(parameters.enableDisplacement));
  ShaderGlobal::set_color4(gpu_objects_coast_rangeVarId, parameters.coastRange.x, parameters.coastRange.y, 0.0f, 0.0f);
  ShaderGlobal::set_int(gpu_objects_face_coastVarId, static_cast<int>(parameters.faceCoast));

  ShaderGlobal::set_color4(gpu_objects_world_coordVarId, 0, 0, cellSize / cellTile, 0);
}

void ObjectManager::addBombHole(const Point3 &point, const float radius)
{
  bomb_holes.push_back(Point4(point.x, point.y, point.z, radius));
}

void ObjectManager::setBombHoleShaderGlobals(const CellToUpdateData cellData)
{
  Point2 cellCenter{cellData.x + cellSize / 2, cellData.z + cellSize / 2};
  eastl::sort(bomb_holes.begin(), bomb_holes.end(), [&cellCenter](Point4 p1, Point4 p2) {
    return (Point2(p1.x, p1.z) - cellCenter).lengthSq() < (Point2(p2.x, p2.z) - cellCenter).lengthSq();
  });
  Point4 bomb_hole_point_radius_0 = bomb_holes.size() < 1 ? Point4::ZERO : bomb_holes[0];
  ShaderGlobal::set_color4(gpu_objects_bomb_hole_point_radius_0VarId, bomb_hole_point_radius_0);
  Point4 bomb_hole_point_radius_1 = bomb_holes.size() < 2 ? Point4::ZERO : bomb_holes[1];
  ShaderGlobal::set_color4(gpu_objects_bomb_hole_point_radius_1VarId, bomb_hole_point_radius_1);
  Point4 bomb_hole_point_radius_2 = bomb_holes.size() < 3 ? Point4::ZERO : bomb_holes[2];
  ShaderGlobal::set_color4(gpu_objects_bomb_hole_point_radius_2VarId, bomb_hole_point_radius_2);
  Point4 bomb_hole_point_radius_3 = bomb_holes.size() < 4 ? Point4::ZERO : bomb_holes[3];
  ShaderGlobal::set_color4(gpu_objects_bomb_hole_point_radius_3VarId, bomb_hole_point_radius_3);
}

void ObjectManager::onLandPlacing()
{
  if (!cellDispatchTile)
    return;
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), gatheredBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), bboxesBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), countersBuffer.get());

  setShaderVarsAndConsts();

  const CellToUpdateData cellData = updateQueue.top();

  ShaderGlobal::set_int4(gpu_object_ints_to_clearVarId, IPoint4(cellData.idx * BBOX3F_SIZE_IN_INT, BBOX3F_SIZE_IN_INT, 0, 0));
  ShaderGlobal::set_color4(gpu_objects_world_coordVarId, cellData.x, cellData.z, cellSize / cellTile, 0);
  ShaderGlobal::set_real(gpu_objects_groups_bbox_offsetVarId, cellData.idx * BBOX3F_SIZE_IN_INT);
  ShaderGlobal::set_real(gpu_objects_cell_buffer_offsetVarId, cellData.idx * maxObjectsCountInCell);
  ShaderGlobal::set_real(gpu_objects_group_idxVarId, cellData.idx);
  setBombHoleShaderGlobals(cellData);

  constexpr int cleanerBatch = (BBOX3F_SIZE_IN_INT + GPU_OBJ_BBOX_CLEANER_SIZE - 1) / GPU_OBJ_BBOX_CLEANER_SIZE;
  counterAndBboxCleaner->dispatch(cleanerBatch, 1, 1);
  d3d::resource_barrier({countersBuffer.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  int prev = ShaderGlobal::get_int_fast(gpu_objects_gpu_instancingVarId);
  if (prev != 0)
    ShaderGlobal::set_int_fast(gpu_objects_gpu_instancingVarId, 0);
  inCellPlacer->dispatch(cellDispatchTile, cellDispatchTile, 1);
  if (prev != 0)
    ShaderGlobal::set_int_fast(gpu_objects_gpu_instancingVarId, 1);

  d3d::resource_barrier({gatheredBuffer.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
}

void ObjectManager::dispatchCells(const ToroidalQuadRegion &region)
{
  TIME_D3D_PROFILE(ObjectManager_dispatchCells)
  for (int i = 0; i < region.wd.x; ++i)
  {
    for (int j = 0; j < region.wd.y; ++j)
    {
      const uint32_t cellIndex = (i + region.lt.x) * cellsSideCount + j + region.lt.y;
      const float xCoord = (static_cast<float>(region.texelsFrom.x) + i) * cellSize;
      const float zCoord = (static_cast<float>(region.texelsFrom.y) + j) * cellSize;
      eastl::vector<CellToUpdateData>::iterator it = eastl::find_if(updateQueue.get_container().begin(),
        updateQueue.get_container().end(), [cellIndex](const CellToUpdateData &data) { return data.idx == cellIndex; });
      if (it != updateQueue.get_container().end())
      {
        it->x = xCoord;
        it->z = zCoord;
      }
      else
      {
        CellToUpdateData updateData = {cellIndex, xCoord, zCoord};
        updateQueue.push(updateData);
      }
    }
  }
}

void ObjectManager::gatherBuffers()
{
  if (maxObjectsCountInCell == 0)
    return;
  TIME_D3D_PROFILE(ObjectManager_gatherBuffers)
  for (int i = 0, flatIdx = 0; i < cellsSideCount; ++i)
    for (int j = 0; j < cellsSideCount; ++j, ++flatIdx)
    {
      Point3 cellPos(toroidalGrid.mainOrigin.x + i + 0.5f - cellsSideCount * 0.5f, 0,
        toroidalGrid.mainOrigin.y + j + 0.5f - cellsSideCount * 0.5f);
      cellPos = cellPos * cellSize;
      appendOrder[flatIdx].first = (lastOrigin - cellPos).lengthSq();
      appendOrder[flatIdx].second = flatIdx;
    }

  stlsort::sort(appendOrder.begin(), appendOrder.end());

  for (int i = 0; i < appendOrder.size(); ++i)
    renderOrder[i] = appendOrder[i].second;
}

void ObjectManager::updateVisibilityAndLods(const Frustum &frustum, const Occlusion *occlusion, ShadowPass for_shadow)
{
  vec4f origin = v_ldu(&lastOrigin.x);
  for (int l = 0; l < MAX_LODS; ++l)
  {
    cellIndexesByLods[l].clear();
  }

  if (for_shadow == ShadowPass::YES && !isRenderedIntoShadows())
    return;

  for (uint32_t idx : renderOrder)
    if (counters[idx] && frustum.testBoxB(cellsBboxes[idx].bmin, cellsBboxes[idx].bmax) &&
        (!occlusion || occlusion->isVisibleBox(cellsBboxes[idx])))
    {
      float distSq = v_extract_x(v_distance_sq_to_bbox_x(cellsBboxes[idx].bmin, cellsBboxes[idx].bmax, origin));
      int lod = 0;
      while (lod < numLods && distSq >= distSqLod[lod])
        ++lod;
      if (lod < numLods)
        cellIndexesByLods[lod].push_back(idx);
    }
}

void ObjectManager::copyMatrices(const eastl::vector<uint32_t> &cells_to_copy, const eastl::vector<uint32_t> &cell_counters,
  Sbuffer *dst_buffer, Sbuffer *src_buffer, uint32_t max_in_cell, uint32_t dst_offset_rows)
{
  matricesOffsets.resize((cells_to_copy.size() + 1) * 2);
  matricesOffsets[0] = 0;
  uint32_t matricessInCell = 0;
  for (uint32_t i = 1; i < matricesOffsets.size() / 2; ++i)
  {
    matricesOffsets[2 * i] = matricesOffsets[2 * (i - 1)] + cell_counters[cells_to_copy[i - 1]];
    matricesOffsets[2 * i - 1] = cells_to_copy[i - 1];
    matricessInCell = max(matricessInCell, cell_counters[cells_to_copy[i - 1]]);
  }

  if (cells_to_copy.size() > 0 && matricessInCell > 0)
  {
    matricesOffsetsBuffer->updateData(0, sizeof(uint32_t) * matricesOffsets.size(), matricesOffsets.data(), 0);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), dst_buffer);
    STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, 0, VALUE), matricesOffsetsBuffer.get());
    STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, 1, VALUE), src_buffer);

    ShaderGlobal::set_int(gpu_objects_max_count_in_cellVarId, max_in_cell);
    ShaderGlobal::set_int(gpu_objects_gather_target_offsetVarId, dst_offset_rows);
    ShaderGlobal::set_int(gpu_objects_visible_cellsVarId, cells_to_copy.size());
    gatherMatrices->dispatchThreads(cells_to_copy.size(), matricessInCell, 1);
    d3d::resource_barrier({dst_buffer, RB_NONE});
  }
}

void ObjectManager::addMatricesToBuffer(Sbuffer *buffer, uint32_t offset_in_bytes, int lod)
{
  copyMatrices(cellIndexesByLods[lod], counters, buffer, gatheredBuffer.get(), maxObjectsCountInCell, offset_in_bytes);
}

uint32_t ObjectManager::getInstancesInGridToDraw(uint32_t lod) const
{
  uint32_t count = 0;
  G_ASSERT(lod < MAX_LODS);
  for (uint32_t idx : cellIndexesByLods[lod])
    count += counters[idx];
  return count;
}

uint32_t ObjectManager::getInstancesToDraw(uint32_t lod) const
{
  G_ASSERT(lod < MAX_LODS);
  uint32_t count = getInstancesInGridToDraw(lod);
  return count;
}

bool ObjectManager::readyToCpuAccess() const { return d3d::get_event_query_status(updateBboxesFence.get(), false); }

// Like split_toroidal
// split box so there is no wrapping, for up to 4 boxes.
// it is assumed, that box is clipped with texture box
// but add +1 splitLine. So assume, that ibox rule is [x, y)
static inline int gpu_objects_split_toroidal(const IBBox2 &cbox, const ToroidalHelper &helper, carray<IBBox2, 4> &boxes)
{
  const IPoint2 halfTexSize(helper.texSize >> 1, helper.texSize >> 1), texSize(helper.texSize, helper.texSize);
  const IPoint2 wrapPtViewport = wrap_texture(helper.curOrigin - helper.mainOrigin, helper.texSize);
  const IPoint2 splitPt = helper.curOrigin + halfTexSize - IPoint2(1, 1) - wrapPtViewport;
  const IPoint2 iboxLT = transform_point_to_viewport(cbox[0], helper);
  const IPoint2 iboxRB = transform_point_to_viewport(cbox[1], helper);

  boxes[0] = cbox;
  int cnt = 1;

  if (iboxLT.x > iboxRB.x)
  {
    const int splitLine = splitPt.x;
    boxes[1] = boxes[0];
    boxes[1][1].x = boxes[0][0].x = splitLine + 1;
    ++cnt;
  }

  if (iboxLT.y > iboxRB.y)
  {
    const int splitLine = splitPt.y;
    for (int i = 0, e = cnt; i < e; ++i)
    {
      boxes[cnt] = boxes[i];
      boxes[cnt][1].y = boxes[i][0].y = splitLine + 1;
      ++cnt;
    }
  }

  return cnt;
}

void ObjectManager::invalidateBBox(const BBox2 &bbox)
{
  IBBox2 ibox(ipoint2(floor(bbox[0] / cellSize)) - IPoint2(1, 1), ipoint2(ceil(bbox[1] / cellSize)) + IPoint2(1, 1));

  toroidal_clip_region(toroidalGrid, ibox); // Now ibox is only visible part of bbox
  if (ibox.isEmpty())
    return;

  carray<IBBox2, 4> splitBoxes;
  int cnt = gpu_objects_split_toroidal(ibox, toroidalGrid, splitBoxes);

  for (int i = 0; i < cnt; ++i)
    dispatchCells(
      ToroidalQuadRegion(transform_point_to_viewport(splitBoxes[i][0], toroidalGrid), splitBoxes[i].size(), splitBoxes[i][0]));
}

void ObjectManager::updateGrid(const Point3 &origin)
{
  if (waitingForGpuData)
    return;
  lastOrigin = origin;
  ToroidalGatherCallback::RegionTab regions;
  ToroidalGatherCallback toroidalCb(regions);
  toroidal_update(IPoint2(origin.x, origin.z) / cellSize, toroidalGrid, 5, toroidalCb);
  for (const ToroidalQuadRegion &region : regions)
    dispatchCells(region);
  if (regions.size())
  {
    stlsort::sort(updateQueue.get_container().begin(), updateQueue.get_container().end(),
      [&origin](const CellToUpdateData &a, const CellToUpdateData &b) -> bool {
        const float distA = pow2(a.x - origin.x) + pow2(a.z - origin.z);
        const float distB = pow2(b.x - origin.x) + pow2(b.z - origin.z);
        return distA > distB || (distA == distB && a.idx > b.idx);
      });
  }
  if (dispatchNextCell())
  {
    if (countersBuffer && countersBuffer->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
      countersBuffer->unlock();

    if (bboxesBuffer && bboxesBuffer->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
      bboxesBuffer->unlock();
    d3d::issue_event_query(updateBboxesFence.get());
    waitingForGpuData = true;
  }
}

void ObjectManager::updateBuffer()
{
  TIME_D3D_PROFILE(ObjectManager_updateBuffer)
  if (waitingForGpuData && readyToCpuAccess())
  {
    void *ptr;
    if (countersBuffer && counters.size() > 0 &&
        countersBuffer->lock(0, sizeof(counters[0]) * counters.size(), &ptr, VBLOCK_READONLY) && ptr)
    {
      counters.assign(reinterpret_cast<uint32_t *>(ptr), reinterpret_cast<uint32_t *>(ptr) + counters.size());
      countersBuffer->unlock();
    }
    if (bboxesBuffer && counters.size() > 0 &&
        bboxesBuffer->lock(0, sizeof(cellsBboxes[0]) * counters.size(), &ptr, VBLOCK_READONLY) && ptr)
    {
      processBboxes(eastl::span<int32_t>(reinterpret_cast<int32_t *>(ptr), BBOX3F_SIZE_IN_INT * counters.size()));
      bboxesBuffer->unlock();
    }
    waitingForGpuData = false;
  }
}

void ObjectManager::processBboxes(const eastl::span<int32_t> &raw_bboxes)
{
  // IntelockedMin/Max doesn't work on floats in hlsl, so needed to encode them to ints
  TIME_PROFILE_DEV(processBboxes);
  const vec4f decodeMul = v_splats(1.0f / GPU_OBJ_HLSL_ENCODE_VAL);
  const int32_t *__restrict rbi = raw_bboxes.cbegin();
  for (bbox3f *__restrict cbd = cellsBboxes.begin(), *cbde = cellsBboxes.end(); cbd != cbde; ++cbd)
  {
    // decodes 8 int to one bbox3 (6 + 2 unused float)
    cbd->bmin = v_mul(v_cvt_vec4f(v_ldui(rbi)), decodeMul);
    rbi += VEC4I_SIZE_IN_INT;
    cbd->bmax = v_mul(v_cvt_vec4f(v_ldui(rbi)), decodeMul);
    rbi += VEC4I_SIZE_IN_INT;
  }
}
void ObjectManager::onLandGpuInstancing(Sbuffer *indirection_buffer, int offset)
{
  setShaderVarsAndConsts();
  inCellPlacer->setStates();
  inCellPlacer->dispatch_indirect(indirection_buffer, offset);
}

void ObjectManager::update(const Point3 &origin)
{
  updateGrid(origin);
  if (!waitingForGpuData)
    return;
  updateBuffer();
  gatherBuffers();
}

void ObjectManager::validateParams() const
{
  if (!do_parameter_check)
    return;
  RenderableInstanceLodsResource *asset = reinterpret_cast<RenderableInstanceLodsResource *>(
    get_one_game_resource_ex((GameResHandle)assetName.c_str(), RendInstGameResClassId));

  BBox3 bbox = asset->bbox;
  bbox.scale(parameters.scale.y); // scale with maximum
  Point3 w = bbox.width();
  if (w.x < ValidationParameterCheckLimits.boundingBoxMinWidth || w.y < ValidationParameterCheckLimits.boundingBoxMinWidth ||
      w.z < ValidationParameterCheckLimits.boundingBoxMinWidth)
  {
    logerr("GPU object validation failed. %s : Maximum bounding box size (%f, %f, %f) is smaller than %f !", this->assetName.c_str(),
      w.x, w.y, w.z, ValidationParameterCheckLimits.boundingBoxMinWidth);
  }
  if (cellsCount > ValidationParameterCheckLimits.maxCellsCount)
  {
    logerr("GPU object validation failed. %s : CellsCount (%d) is larger than %f !", this->assetName.c_str(), cellsCount,
      ValidationParameterCheckLimits.maxCellsCount);
  }
}

void GpuObjects::update(const Point3 &origin)
{
  TIME_PROFILE(GpuObjects_update)
  for (ObjectManager &object : objects)
    object.update(origin);
  if (VolumePlacer *volumePlacer = get_volume_placer_mgr())
    volumePlacer->performPlacing(origin);
}

void GpuObjects::setGpuInstancingRelemParams(int cascade_no)
{
  int objCount = objectIds.size();

  const int GPUOBJDATA_SIZE = 2;

  dag::Vector<Point4, framemem_allocator> gpuObjectData;

  clearCascade(cascade_no);

  gpuObjectData.resize(objCount * MAX_LODS * GPUOBJDATA_SIZE);

  cascades[cascade_no].generateIndirectParamsBuffer.close();

  String bufferName;
  bufferName.printf(0, "generateIndirectParamsBuffer%d", cascade_no);
  cascades[cascade_no].generateIndirectParamsBuffer = dag::create_sbuffer(sizeof(vec4f), GPUOBJDATA_SIZE * MAX_LODS * objCount,
    SBCF_BIND_SHADER_RES | SBCF_MAYBELOST, TEXFMT_A32B32G32R32F, bufferName);

  for (int lod = 0; lod < MAX_LODS; lod++)
  {
    for (int i = 0; i < GPUOBJ_LAYER_COUNT; i++)
    {
      cascades[cascade_no].layers[i].objectLodOffsets[lod] = cascades[cascade_no].layers[i].objectIds.size();
    }
    int objOffset = 0;
    for (unsigned int objIdx = 0; objIdx < objCount; ++objIdx)
    {
      gpuObjectData[GPUOBJDATA_SIZE * (objIdx * MAX_LODS + lod) + 0] = Point4(0, 0, 0, 0);
      gpuObjectData[GPUOBJDATA_SIZE * (objIdx * MAX_LODS + lod) + 1] = Point4(0, 0, 0, 0);

      ObjectManager &object = objects[objIdx];
      int layerIdx = layerRIOnLayerGpuobjRemap(object.layer);
      RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(objectIds[objIdx]);

      bool lodExist = lod < res->lods.size();
      if (lodExist)
      {
        const ShaderMesh *m = res->lods[lod].scene->getMesh()->getMesh()->getMesh();
        dag::ConstSpan<ShaderMesh::RElem> elems = m->getAllElems();
        const ShaderMesh::RElem &elem = elems[0];
        // index_count_per_instance, start_index_location, base vertex, object offset
        gpuObjectData[GPUOBJDATA_SIZE * (objIdx * MAX_LODS + lod) + 0] = Point4(elem.numf * 3, elem.si, elem.baseVertex, objOffset);

        int t1, t2;
        float cellSize;
        object.getCellData(t1, t2, cellSize);
        cellSize /= (float)object.getCellDispatchTile();
        // tile size, lod range, pad, pad
        float lodRange = min(0.5f * cellSize * object.getCellsSizeCount(), object.getLodRange(lod));
        gpuObjectData[GPUOBJDATA_SIZE * (objIdx * MAX_LODS + lod) + 1] = Point4(cellSize, lodRange, 1, 1);

        // objects laying by objects: obj0_lod0, obj1_lod0 .. obj0_lod1, obj1_lod1 ...
        // offsets are static because we use indirect drawing, and real offsets and counts are stored in buffer
        cascades[cascade_no].layers[layerIdx].offsetsAndCounts.emplace_back(objIdx * MAX_LODS + lod, 1);
        cascades[cascade_no].layers[layerIdx].objectIds.emplace_back(objectIds[objIdx]);
      }
      objOffset += object.getMaxPossibleInstances();
    }
  }

  cascades[cascade_no].generateIndirectParamsBuffer->updateData(0, sizeof(vec4f) * objCount * MAX_LODS * GPUOBJDATA_SIZE,
    gpuObjectData.data(), 0);
}

void GpuObjects::rebuildGpuInstancingRelemParams()
{
  TIME_D3D_PROFILE(GpuObjects_RebuildRelems)
  int objCount = objectIds.size();
  if (objCount == 0 || !gpuInstancingRebuildRelems)
    return;

  for (int cascade_no = 0; cascade_no < cascades.size(); cascade_no++)
  {
    if (cascades[cascade_no].isDeleted || !cascades[cascade_no].buffersCreated)
      continue;

    setGpuInstancingRelemParams(cascade_no);
    {
      // create indirect inputs
      STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, gpu_objects_generation_params_const_no, VALUE),
        cascades[cascade_no].generateIndirectParamsBuffer.get());
      // create indirect outputs
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 4, VALUE), cascades[cascade_no].drawIndirectBuffer.get());
      // create and fill indirection buffers
      gpuInstancingRebuildRelems->setStates();
      gpuInstancingRebuildRelems->dispatch(1, 1, 1);
    }
    d3d::resource_barrier({cascades[cascade_no].drawIndirectBuffer.get(), RB_RO_INDIRECT_BUFFER});
  }
}

void GpuObjects::prepareGpuInstancing(int cascade_no)
{
  // fill data buffers (if needed) and reset counters

  ShaderGlobal::set_int_fast(gpu_objects_gpu_instancingVarId, 1);

  if (!cascades[cascade_no].buffersCreated)
  {
    ShaderGlobal::set_int(gpu_objects_generation_stateVarId, 0);

    if (!gpuInstancingGenerateIndirect)
      gpuInstancingGenerateIndirect.reset(new_compute_shader("gpu_objects_create_indirect", true));

    if (!gpuInstancingRebuildRelems)
      gpuInstancingRebuildRelems.reset(new_compute_shader("gpu_objects_rebuild_relems", true));

    int objCount = objectIds.size();
    if (objCount == 0)
      return;

    ShaderGlobal::set_real_fast(gpu_objects_countVarId, (float)objCount + 0.1f);

    {
      String bufferName;
      bufferName.printf(0, "drawIndirectBuffer%d", cascade_no);
      cascades[cascade_no].drawIndirectBuffer =
        dag::buffers::create_ua_indirect(dag::buffers::Indirect::DrawIndexed, MAX_LODS * objCount, bufferName);
    }
    {
      String bufferName;
      bufferName.printf(0, "dispatchCountBuffer%d", cascade_no);
      cascades[cascade_no].dispatchCountBuffer =
        dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, objCount, bufferName);
    }
    {
      String bufferName;
      bufferName.printf(0, "lodOffsetsBuffer%d", cascade_no);
      cascades[cascade_no].lodOffsetsBuffer = dag::buffers::create_ua_sr_byte_address(MAX_LODS * objCount, bufferName);
    }
    {
      struct GpuObjectsIndirectParams
      {
        int2 cornerPos;
        uint xSize;
        float tileSize;
      };
      String bufferName;
      bufferName.printf(0, "GpuObjectsIndirectParams%d", cascade_no);

      cascades[cascade_no].indirectParamsBuffer =
        dag::buffers::create_ua_sr_structured(sizeof(GpuObjectsIndirectParams), objCount, bufferName);
    }

    setGpuInstancingRelemParams(cascade_no);

    logdbg("gpu objects gpu instancing buffers created");
    cascades[cascade_no].buffersCreated = true;
  }
  else
    ShaderGlobal::set_int(gpu_objects_generation_stateVarId, 1);

  {
    TIME_D3D_PROFILE(GpuObjects_CreateIndirect)
    // create indirect inputs
    STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, gpu_objects_generation_params_const_no, VALUE),
      cascades[cascade_no].generateIndirectParamsBuffer.get());
    // create indirect outputs
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 4, VALUE), cascades[cascade_no].drawIndirectBuffer.get());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 5, VALUE), cascades[cascade_no].dispatchCountBuffer.get());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 6, VALUE), cascades[cascade_no].lodOffsetsBuffer.get());
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 7, VALUE), cascades[cascade_no].indirectParamsBuffer.get());
    // create and fill indirection buffers
    gpuInstancingGenerateIndirect->setStates();
    gpuInstancingGenerateIndirect->dispatch(1, 1, 1);
  }

  // generate indirect inputs
  STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, gpu_objects_generation_params_const_no, VALUE),
    cascades[cascade_no].generateIndirectParamsBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, gpu_objects_indirect_params_const_no, VALUE),
    cascades[cascade_no].indirectParamsBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_buffer(STAGE_CS, gpu_objects_lod_offsets_buffer_const_no, VALUE),
    cascades[cascade_no].lodOffsetsBuffer.get());
  // generate indirect outputs
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), cascades[cascade_no].matricesBuffer.get());
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), cascades[cascade_no].drawIndirectBuffer.get());

  for (uint32_t i = 0, e = objects.size(); i < e; ++i)
  {
    ObjectManager &object = objects[i];
    ShaderGlobal::set_real_fast(gpu_objects_noVarId, (float)i + 0.1f);
    // todo: single dispatch for all objects
    object.onLandGpuInstancing(cascades[cascade_no].dispatchCountBuffer.get(), i * 12);
  }
}

void GpuObjects::clearCascade(int cascade)
{
  for (int i = 0; i < GPUOBJ_LAYER_COUNT; i++)
  {
    cascades[cascade].layers[i].offsetsAndCounts.clear();
    cascades[cascade].layers[i].objectIds.clear();
  }
}

void GpuObjects::beforeDraw(rendinst::RenderPass render_pass, int cascade, const Frustum &frustum, const Occlusion *occlusion,
  const char *mission_name, const char *map_name, bool gpu_instancing)
{
  TIME_D3D_PROFILE(GpuObjects_BeforeDraw)
  ShadowPass forShadow = (render_pass == rendinst::RenderPass::Depth         // dynamic
                           || render_pass == rendinst::RenderPass::ToShadow) // csm
                           ? ShadowPass::YES
                           : ShadowPass::NO;


  if (gpu_instancing != cascades[cascade].gpuInstancing)
  {
    cascades[cascade].gpuInstancing = gpu_instancing;
    cascades[cascade].buffersCreated = false;
  }

  if (!cascades[cascade].gpuInstancing)
    clearCascade(cascade);

  // We read max objects count here, because game params is not initialized when we initialize GPU objects.
  // TODO: move max objects count setting to ECS.
  if (maxRowsCountInBuffer <= 0) // first init
  {
    const DataBlock *gpuObjectsBlk = dgs_get_game_params()->getBlockByNameEx("GpuObjects");
    maxRowsCountInBuffer = gpuObjectsBlk->getInt("maxGpuObjectsCount", 1024 * 1024) * ROWS_IN_MATRIX;
  }

  VolumePlacer *volumePlacer = get_volume_placer_mgr();
  bool hasInVolumeInstances = false;
  if (volumePlacer)
  {
    volumePlacer->updateVisibility(frustum, forShadow);
    hasInVolumeInstances = volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_OPAQUE) > 0 ||
                           volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_DECAL) > 0 ||
                           volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_DISTORSION) > 0;
  }

  if (!objects.empty() || hasInVolumeInstances)
  {
    uint32_t maxInstancesCount = 0;
    uint32_t maxInstancesCountOnRi = 0;
    if (volumePlacer)
    {
      maxInstancesCount += volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_OPAQUE);
      maxInstancesCount += volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_DECAL);
      maxInstancesCount += volumePlacer->getNumInstancesToDraw(VolumePlacer::LAYER_DISTORSION);
    }
    for (ObjectManager &object : objects)
      maxInstancesCount += object.getMaxPossibleInstances();

    uint32_t rowsInBuffer = (maxInstancesCountOnRi + maxInstancesCount) * ROWS_IN_MATRIX;
    if ((!cascades[cascade].matricesBuffer || cascades[cascade].matricesBuffer->getNumElements() < rowsInBuffer) &&
        maxInstancesCount > 0)
    {
      G_UNUSED(mission_name);
      G_UNUSED(map_name);
      if (rowsInBuffer > maxRowsCountInBuffer) // more than 64MB
        logerr("buffer for gpu objects is too large, %dMB for %d objects, %d instances placed on RI, mis %s, map %s",
          (rowsInBuffer * sizeof(Point4)) >> 20, maxInstancesCount, maxInstancesCountOnRi, mission_name, map_name);
      else
        logdbg("buffer for gpu objects created, %dMB for %d objects, %d instances placed on RI", (rowsInBuffer * sizeof(Point4)) >> 20,
          maxInstancesCount, maxInstancesCountOnRi);
      cascades[cascade].matricesBuffer = dag::create_sbuffer(sizeof(Point4), rowsInBuffer,
        SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED, TEXFMT_A32B32G32R32F, "GPUobjects");
    }
  }

  uint32_t bufferOffset = 0;

  if (!cascades[cascade].gpuInstancing)
  {
    ShaderGlobal::set_int_fast(gpu_objects_gpu_instancingVarId, 0);
    for (ObjectManager &object : objects)
    {
      object.updateVisibilityAndLods(frustum, occlusion, forShadow);
    }
  }
  else
  {
    set_frustum_planes(frustum);
    prepareGpuInstancing(cascade);
    return;
  }

  for (int lod = 0; lod < MAX_LODS; ++lod)
  {
    for (int i = 0; i < GPUOBJ_LAYER_COUNT; i++)
    {
      cascades[cascade].layers[i].objectLodOffsets[lod] = cascades[cascade].layers[i].objectIds.size();
    }

    for (uint32_t i = 0, e = objects.size(); i < e; ++i)
    {
      ObjectManager &object = objects[i];
      uint32_t countByLod = object.getInstancesToDraw(lod);
      if (!countByLod)
        continue;

      int layerIdx = layerRIOnLayerGpuobjRemap(object.layer);

      object.addMatricesToBuffer(cascades[cascade].matricesBuffer.get(), bufferOffset * ROWS_IN_MATRIX, lod);
      cascades[cascade].layers[layerIdx].offsetsAndCounts.emplace_back(bufferOffset * ROWS_IN_MATRIX, countByLod);
      cascades[cascade].layers[layerIdx].objectIds.emplace_back(objectIds[i]);
      bufferOffset += countByLod;
    }
    if (volumePlacer)
    {
      volumePlacer->copyMatrices(cascades[cascade].matricesBuffer.get(), bufferOffset, lod, VolumePlacer::LAYER_OPAQUE,
        cascades[cascade].layers[GPUOBJ_LAYER_OPAQUE].offsetsAndCounts, cascades[cascade].layers[GPUOBJ_LAYER_OPAQUE].objectIds);
      volumePlacer->copyMatrices(cascades[cascade].matricesBuffer.get(), bufferOffset, lod, VolumePlacer::LAYER_DECAL,
        cascades[cascade].layers[GPUOBJ_LAYER_DECAL].offsetsAndCounts, cascades[cascade].layers[GPUOBJ_LAYER_DECAL].objectIds);
      volumePlacer->copyMatrices(cascades[cascade].matricesBuffer.get(), bufferOffset, lod, VolumePlacer::LAYER_DISTORSION,
        cascades[cascade].layers[GPUOBJ_LAYER_DISTORSION].offsetsAndCounts,
        cascades[cascade].layers[GPUOBJ_LAYER_DISTORSION].objectIds);
    }
  }
}

void GpuObjects::validateDisplacedGPUObjs(float displacement_tex_range)
{
  for (ObjectManager &object : objects)
  {
    object.validateDisplacedGPUObjs(displacement_tex_range);
  }
}

int GpuObjects::addCascade()
{
  for (int i = 0; i < cascades.size(); i++)
  {
    if (cascades[i].isDeleted)
    {
      cascades[i].isDeleted = false;
      return i;
    }
  }
  cascades.push_back();
  cascades.back().isDeleted = false;
  return cascades.size() - 1;
}

void GpuObjects::invalidateBBox(const BBox2 &bbox)
{
  for (ObjectManager &object : objects)
    object.invalidateBBox(bbox);
}

Sbuffer *GpuObjects::getBuffer(int cascade, unsigned layer)
{
  G_ASSERT_RETURN(cascade >= 0 && cascade < cascades.size() && !cascades[cascade].isDeleted, nullptr);
  G_UNUSED(layer);
  return cascades[cascade].matricesBuffer.get();
}

Sbuffer *GpuObjects::getIndirectionBuffer(int cascade)
{
  G_ASSERT_RETURN(cascade >= 0 && cascade < cascades.size() && !cascades[cascade].isDeleted, nullptr);
  return cascades[cascade].drawIndirectBuffer.get();
}

Sbuffer *GpuObjects::getOffsetsBuffer(int cascade)
{
  G_ASSERT_RETURN(cascade >= 0 && cascade < cascades.size() && !cascades[cascade].isDeleted, nullptr);
  return cascades[cascade].lodOffsetsBuffer.get();
}

bool GpuObjects::getGpuInstancing(int cascade)
{
  G_ASSERT_RETURN(cascade >= 0 && cascade < cascades.size() && !cascades[cascade].isDeleted, false);
  return cascades[cascade].gpuInstancing;
}

void GpuObjects::releaseCascade(int cascade)
{
  G_ASSERT_RETURN(cascade >= 0 && cascade < cascades.size() && !cascades[cascade].isDeleted, );
  cascades[cascade] = CascadeData(); // Free everything
  cascades[cascade].isDeleted = true;
  int firstValidCascade = cascades.size() - 1;
  for (; firstValidCascade >= 0; --firstValidCascade)
    if (!cascades[firstValidCascade].isDeleted)
      break;
  cascades.resize(firstValidCascade + 1);
}

void setup_parameter_validation(const DataBlock *validationBlock)
{
  if (validationBlock->isEmpty())
    return;
  ValidationParameterCheckLimits = {validationBlock->getReal("min_bounding_box_size", 1.0f),
    validationBlock->getInt("max_cell_scount", 64), validationBlock->getReal("max_object_density", 1.0f)};
  do_parameter_check = true;
}

} // namespace gpu_objects
