#include <3d/dag_drv3d.h>
#include <perfMon/dag_statDrv.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/riexHandle.h>
#include <render/lruCollision.h>
#include <render/primitiveObjects.h>
#include <math/dag_mathUtils.h>
#include <../shaders/gi_voxelize_inc.hlsli>
#include <3d/dag_lockSbuffer.h>
#include <util/dag_convar.h>
#include <generic/dag_smallTab.h>

// todo: use clusters may be?
// todo: classification
// todo: for big cascade we can use compute shader voxelization (i.e. if triangles are typically less then few voxels)
//  so, we can classify instances into two lists: HW rasterizer and SW rasterizer (and may be +very small objects separately, objects
//  that fit into one voxel)
// todo: KD-tree (top & bottom structures)
// todo: gpu submission on DX12/VK/Metal at least
static constexpr bool can_voxelize_in_compute = false;
static constexpr bool use_geom_shader = false;

CONSOLE_BOOL_VAL("render", gi_voxelize_cs, false);
CONSOLE_BOOL_VAL("render", gi_voxelize_md, true);

#define GLOBAL_VARS_LIST           \
  VAR(gi_voxelize_with_instancing) \
  VAR(gi_voxelize_use_multidraw)   \
  VAR(gi_voxelize_with_colors)     \
  VAR(gi_voxelization_vbuffer)     \
  VAR(gi_voxelization_ibuffer)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

extern const CollisionResource *lru_collision_get_collres(uint32_t i);
extern mat43f lru_collision_get_transform(rendinst::riex_handle_t h);
extern uint32_t lru_collision_get_type(rendinst::riex_handle_t h);

uint32_t LRURendinstCollision::getMaxBatchSize() const { return MAX_VOXELIZATION_INSTANCES; }

static constexpr uint32_t compute_vb_flags = (can_voxelize_in_compute ? SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW : 0);

LRURendinstCollision::LRURendinstCollision() :
  vbAllocator(SbufferHeapManager("vb_collision_", 4, compute_vb_flags | SBCF_MAYBELOST | SBCF_BIND_VERTEX)),
  ibAllocator(SbufferHeapManager("ib_collision_", 4, compute_vb_flags | SBCF_MAYBELOST | SBCF_BIND_INDEX))
{
  create_cubic_indices(make_span((uint8_t *)boxIndices.data(), COLLISION_BOX_INDICES_NUM * sizeof(uint16_t)), 1, false);

  supportNoOverwrite = d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers;
  instanceTms = dag::create_sbuffer(sizeof(Point4), getMaxBatchSize() * 3,
    (supportNoOverwrite ? (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE) : 0) | SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED,
    0, "collision_voxelization_tm");
  // if we don't support nooverwrite do not use SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE
  voxelizeCollisionMat.reset(new_shader_material_by_name_optional("voxelize_collision"));
  if (voxelizeCollisionMat)
  {
    voxelizeCollisionMat->addRef();
    voxelizeCollisionElem = voxelizeCollisionMat->make_elem();
  }
  voxelize_collision_cs.reset();
  auto csMat = new_shader_material_by_name_optional("voxelize_collision_cs");
  if (csMat)
    voxelize_collision_cs.reset(new ComputeShaderElement(&csMat->native()));
  const bool multiDrawIndirectSupported = d3d::get_driver_desc().caps.hasWellSupportedIndirect;
  if (multiDrawIndirectSupported)
  {
    serialBuf.init(MAX_VOXELIZATION_INSTANCES * 3); // 3 is for 3-axis projection
    VSDTYPE vsdInstancing[] = {VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_HALF4), VSD_STREAM_PER_INSTANCE_DATA(1),
      VSD_REG(VSDR_TEXC0, VSDT_INT1), VSD_END};
    if (voxelizeCollisionElem)
      voxelizeCollisionElem->replaceVdecl(d3d::create_vdecl(vsdInstancing));

    multiDrawBuf = dag::create_sbuffer(INDIRECT_BUFFER_ELEMENT_SIZE,
      getMaxBatchSize() * DRAW_INDEXED_INDIRECT_NUM_ARGS / INDIRECT_BUFFER_ELEMENT_SIZE, SBCF_INDIRECT, 0,
      "collision_voxelization_indirect");
    debug("%s support multi draw indirect", __FUNCTION__);
  }
}

LRURendinstCollision::~LRURendinstCollision()
{
  if (multiDrawBuf && voxelizeCollisionElem)
    d3d::delete_vdecl(voxelizeCollisionElem->getEffectiveVDecl());
}

LRURendinstCollision::RiDataInfo LRURendinstCollision::getRiData(uint32_t type)
{
  uint32_t oldSize = riInfo.size();
  if (type >= oldSize)
  {
    riInfo.resize(type + 1);
    for (uint32_t i = oldSize, ei = riInfo.size(); i < ei; ++i)
    {
      const CollisionResource *collRes = lru_collision_get_collres(i);
      if (!collRes)
        continue;
      for (int ni = 0, ne = collRes->getAllNodes().size(); ni < ne; ++ni)
      {
        const CollisionNode *node = collRes->getNode(ni);
        if (!node)
          continue;

        if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE))
          continue;
        if (node->type == COLLISION_NODE_TYPE_BOX)
        {
          riInfo[i].ibSize += COLLISION_BOX_INDICES_NUM * sizeof(uint16_t);
          riInfo[i].vbSize += COLLISION_BOX_VERTICES_NUM * sizeof(CollisionVertex);
        }
        else if (node->indices.size() > 0 && (node->type == COLLISION_NODE_TYPE_MESH || node->type == COLLISION_NODE_TYPE_CONVEX))
        {
          riInfo[i].ibSize += data_size(node->indices);
          riInfo[i].vbSize += node->vertices.size() * sizeof(CollisionVertex);
        }
      }
      riInfo[i].ibSize = (riInfo[i].ibSize + 3) & ~3; // align size to 4
    }
  }
  return riInfo[type];
}

uint32_t LRURendinstCollision::getVbCapacity() const
{
  if (auto buf = vbAllocator.getHeap().getBuf())
    return buf->ressize();
  return 0;
}

uint32_t LRURendinstCollision::getIbCapacity() const
{
  if (auto buf = ibAllocator.getHeap().getBuf())
    return buf->ressize();
  return 0;
}

bool LRURendinstCollision::allocateEntries(const uint32_t *begin, const uint32_t *end, uint32_t &vMaxSz, uint32_t &iMaxSz,
  uint32_t cFrame)
{
  uint32_t newestFrameToFree = 0;
  auto freeEntry = [&](size_t entry) {
    G_ASSERT(riToLRUmap[entry].vb && riToLRUmap[entry].ib);
    ibAllocator.free(riToLRUmap[entry].ib);
    vbAllocator.free(riToLRUmap[entry].vb);
    newestFrameToFree = eastl::max(newestFrameToFree, entryLastFrameUsed[entry]);
  };
  vMaxSz = iMaxSz = 0;
  uint32_t vAddSz = 0, iAddSz = 0;
  for (const uint32_t *i = begin; i != end; ++i)
  {
    RiDataInfo info = riInfo[*i];
    vMaxSz = max(vMaxSz, info.vbSize);
    iMaxSz = max(iMaxSz, info.ibSize);
    vAddSz += info.vbSize;
    iAddSz += info.ibSize;
  }

  struct UnusedEntry
  {
    uint32_t entry, frame;
  };
  bool unusedListPrepared = false;
  dag::Vector<UnusedEntry, framemem_allocator> unusedEntries;
  // let's reduce amount of copies, by freeing unused (currently)
  auto prepareLRU = [&]() {
    if (!unusedListPrepared)
    {
      unusedListPrepared = true;
      for (uint32_t i = 0, e = riToLRUmap.size(); i < e; ++i)
      {
        uint32_t frame = entryLastFrameUsed[i];
        if (cFrame != frame && riToLRUmap[i].vb)
          unusedEntries.push_back(UnusedEntry{i, frame});
      }
      stlsort::sort(unusedEntries.begin(), unusedEntries.end(), [](auto a, auto b) { return a.frame > b.frame; }); // smaller frame
                                                                                                                   // number first
    }
  };
  if (vAddSz > vbAllocator.freeMemLeft() || iAddSz > ibAllocator.freeMemLeft())
  {
    debug("LRU: %s adding %d new instances, vb/ib needed: %d/%d, insufficient space left: %d/%d", __FUNCTION__, end - begin, vAddSz,
      iAddSz, vbAllocator.freeMemLeft(), ibAllocator.freeMemLeft());
    // prepareLRU();//we will need it
  }

  const uint32_t *i = begin;
  for (; i != end; ++i)
  {
    auto type = *i;
    RiDataInfo info = riInfo[type];
    G_ASSERTF(type < riToLRUmap.size(), "riToLRUmap.size()=%d type = %d", riToLRUmap.size(), type);
    G_ASSERT(!info.isEmpty() && !riToLRUmap[type].vb && !riToLRUmap[type].ib);
    while (!vbAllocator.canAllocate(info.vbSize) || !ibAllocator.canAllocate(info.ibSize))
    {
      prepareLRU();
      if (unusedEntries.empty()) // no unused entries
        break;
      freeEntry(unusedEntries.back().entry);
      unusedEntries.pop_back();
    };
    if (!vbAllocator.canAllocate(info.vbSize) || !ibAllocator.canAllocate(info.ibSize))
      break;
    riToLRUmap[type] = LRUEntry{vbAllocator.allocateInHeap(info.vbSize), ibAllocator.allocateInHeap(info.ibSize)};
  }
  constexpr uint32_t thrashingFrameThreshold = 6; // if we are freeing something that has been used less than N frames ago
  if (i == end && newestFrameToFree < cFrame - thrashingFrameThreshold) // succeeded
    return true;
  if (i == end)
    debug("we are thrashing LRU cache too often! last frame age used was %d", cFrame - newestFrameToFree);

  --i;
  for (auto rend = begin - 1; i != rend; --i) // revert allocations (so we won't make useless copies into, when resize)
    freeEntry(*i);

  auto resize = [](LinearHeapAllocatorSbuffer &alloc, uint32_t added, uint32_t minBufSize) {
    const uint32_t pageMask = 65536 - 1; // and align to page

    const uint32_t needed = alloc.allocated() + added;
    const uint32_t next = max<uint32_t>(alloc.getHeapSize() * 3 / 2, max((needed + pageMask) & ~pageMask, minBufSize));
    debug("LRU %s: allocate +%d bytes in heap (%d allocated, %d free %d biggest), increase heap size to %d!",
      alloc.getManager().getName(), added, alloc.allocated(), alloc.freeMemLeft(), alloc.biggestContigousChunkLeft(), next);
    if (alloc.resize(next))
      return true;
    logerr("%s: can not resize allocator to %d+%d = %d size", alloc.getManager().getName(), alloc.allocated(), added, next);
    return false;
  };

  if (!resize(vbAllocator, vAddSz, 2 << 20) || !resize(ibAllocator, iAddSz, 11 << 18)) // to reduce re-allocation, allocate 2mb at
                                                                                       // least for VB and 2.75 mb for ib (ratio is
                                                                                       // manually tweaked)
    return false;

  // after resize we try again, and now it has to succeed
  for (const uint32_t *i = begin; i != end; ++i)
  {
    auto type = *i;
    RiDataInfo info = riInfo[type];
    G_ASSERTF(type < riToLRUmap.size(), "riToLRUmap.size()=%d type = %d", riToLRUmap.size(), type);
    G_ASSERT(!info.isEmpty() && !riToLRUmap[type].vb && !riToLRUmap[type].ib);
    riToLRUmap[type] = LRUEntry{vbAllocator.allocateInHeap(info.vbSize), ibAllocator.allocateInHeap(info.ibSize)};
    G_ASSERT(riToLRUmap[type].vb && riToLRUmap[type].ib);
  }
  return true;
}

void LRURendinstCollision::reset()
{
  for (auto &entry : riToLRUmap)
  {
    ibAllocator.free(entry.ib);
    vbAllocator.free(entry.vb);
  }
}

bool LRURendinstCollision::updateLRU(dag::ConstSpan<rendinst::riex_handle_t> ri)
{
  FRAMEMEM_REGION;
  uint32_t currentFrame = ::dagor_frame_no();
  eastl::bitvector<framemem_allocator> processed;
  processed.reserve(2048);
  typedef eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> entry_map_t;
  entry_map_t nonExistent;

  for (auto h : ri)
  {
    const uint32_t type = lru_collision_get_type(h);
    if (processed.test(type, false))
      continue;
    if (riToLRUmap.size() <= type)
    {
      riToLRUmap.resize(type + 1);
      entryLastFrameUsed.resize(type + 1);
    }
    processed.set(type, true);
    const auto info = getRiData(type);
    if (info.isEmpty())
      continue;
    if (!riToLRUmap[type].vb)
      nonExistent.insert(type);
    entryLastFrameUsed[type] = currentFrame;
  }
  if (nonExistent.size() == 0)
    return true;

  // sort by size, to decrease defragmentation, not very important
  stlsort::sort(nonExistent.begin(), nonExistent.end(), [&](auto a, auto b) { return riInfo[a].ibSize > riInfo[b].ibSize; });

  // add missing lru
  // try make allocations first, and if we can't do them due to defragmentation, then revert allocations made and only then update data
  // (to decrease amount of updateData calls)
  uint32_t vMaxSz, iMaxSz;
  if (!allocateEntries(nonExistent.begin(), nonExistent.end(), vMaxSz, iMaxSz, currentFrame))
    return false;

  Tab<uint8_t> verticesRes(framemem_ptr()), indicesRes(framemem_ptr());
  verticesRes.resize(vMaxSz);
  indicesRes.resize(iMaxSz);

  for (auto type : nonExistent)
  {
    G_ASSERTF(type < riToLRUmap.size(), "riToLRUmap.size()=%d type = %d", riToLRUmap.size(), type);
    G_ASSERT(riToLRUmap[type].vb && riToLRUmap[type].ib);

    auto vb = vbAllocator.get(riToLRUmap[type].vb), ib = ibAllocator.get(riToLRUmap[type].ib);

    G_ASSERTF(ib.size == riInfo[type].ibSize && vb.size == riInfo[type].vbSize, "requested %d,%d == %d,%d", riInfo[type].vbSize,
      riInfo[type].ibSize, vb.size, ib.size);

    G_ASSERT(vb.offset % sizeof(CollisionVertex) == 0 && ib.offset % sizeof(uint16_t) == 0);
    G_ASSERT((ib.size & 3) == 0 && (ib.offset & 3) == 0); // it is 16 bit index, but we copy aligned to 4

    const CollisionResource *collRes = lru_collision_get_collres(type);
    G_ASSERT_CONTINUE(collRes && collRes->getAllNodes().size());
    uint8_t *vertices = verticesRes.data(), *indices = indicesRes.data();

    int firstVertex = 0;
    int nodeIbSize = 0;
    int nodeVbSize = 0;
    for (int ni = 0, ne = collRes->getAllNodes().size(); ni < ne; ++ni)
    {
      const CollisionNode *node = collRes->getNode(ni);
      if (!node || !node->checkBehaviorFlags(CollisionNode::TRACEABLE))
        continue;
      if (node->type == COLLISION_NODE_TYPE_BOX)
      {
        nodeIbSize = COLLISION_BOX_INDICES_NUM * sizeof(uint16_t);
        nodeVbSize = COLLISION_BOX_VERTICES_NUM * sizeof(CollisionVertex);
        Point3_vec4 boxVertices[COLLISION_BOX_VERTICES_NUM];
        for (int vertNo = 0; vertNo < COLLISION_BOX_VERTICES_NUM; ++vertNo)
          boxVertices[vertNo] = node->modelBBox.point(vertNo);

        const vec4f *__restrict verts = (const vec4f *)boxVertices;
        CollisionVertex *__restrict vertsDest = (CollisionVertex *)vertices;
        for (const vec4f *__restrict ve = verts + COLLISION_BOX_VERTICES_NUM; verts != ve; ++verts, ++vertsDest)
          v_float_to_half(&vertsDest->x, *verts);
        G_FAST_ASSERT((uint8_t *)vertsDest <= verticesRes.end());

        if (!firstVertex)
          memcpy(indices, boxIndices.data(), nodeIbSize);
        else
        {
          uint16_t *ind = (uint16_t *)indices;
          for (int ii = 0; ii < COLLISION_BOX_INDICES_NUM; ++ii, ++ind)
            *ind = boxIndices[ii] + firstVertex;
        }
      }
      else if (node->indices.size() > 0 && (node->type == COLLISION_NODE_TYPE_MESH || node->type == COLLISION_NODE_TYPE_CONVEX))
      {
        nodeIbSize = data_size(node->indices);
        nodeVbSize = node->vertices.size() * sizeof(CollisionVertex);
        G_ASSERT_CONTINUE(elem_size(node->vertices) == sizeof(vec4f));
        CollisionVertex *__restrict vertsDest = (CollisionVertex *)vertices;
        const vec4f *__restrict verts = (const vec4f *)node->vertices.data();
        if ((node->flags & (node->IDENT | node->TRANSLATE)) == node->IDENT)
        {
          for (const vec4f *__restrict ve = verts + node->vertices.size(); verts != ve; ++verts, ++vertsDest)
            v_float_to_half(&vertsDest->x, *verts);
        }
        else
        {
          mat44f nodeTm;
          v_mat44_make_from_43cu_unsafe(nodeTm, node->tm[0]);
          for (const vec4f *__restrict ve = verts + node->vertices.size(); verts != ve; ++verts, ++vertsDest)
            v_float_to_half(&vertsDest->x, v_mat44_mul_vec3p(nodeTm, *verts));
        }
        G_FAST_ASSERT((uint8_t *)vertsDest <= verticesRes.end());

        if (!firstVertex)
          memcpy(indices, node->indices.data(), nodeIbSize);
        else
        {
          uint16_t *ind = (uint16_t *)indices;
          for (int ii = 0; ii < node->indices.size(); ++ii, ++ind)
            *ind = node->indices[ii] + firstVertex;
        }
      }
      else
        continue;

      G_ASSERT(nodeIbSize > 0 && nodeVbSize > 0);

      indices += nodeIbSize;
      firstVertex += nodeVbSize / sizeof(CollisionVertex);
      vertices += nodeVbSize;
    }
    const size_t indicesWrittenBytes = (indices - indicesRes.data());
    G_UNUSED(indicesWrittenBytes);
    G_ASSERTF(vertices - verticesRes.data() == vb.size && ((indicesWrittenBytes + 3) & ~3) == ib.size, "%d == %d && %d == %d",
      vertices - verticesRes.data(), vb.size, ((indicesWrittenBytes + 3) & ~3), ib.size);
    const uint32_t flag = VBLOCK_WRITEONLY;
    ibAllocator.getHeap().getBuf()->updateData(ib.offset, ib.size, indicesRes.data(), flag);
    vbAllocator.getHeap().getBuf()->updateData(vb.offset, vb.size, verticesRes.data(), flag);
  }
  debug("lru map, vb,ib: %d,%d allocated, %d,%d free, %d,%d biggest", vbAllocator.allocated(), ibAllocator.allocated(),
    vbAllocator.freeMemLeft(), ibAllocator.freeMemLeft(), vbAllocator.biggestContigousChunkLeft(),
    ibAllocator.biggestContigousChunkLeft());
  return true;
}

void LRURendinstCollision::batchInstances(const rendinst::riex_handle_t *begin, const rendinst::riex_handle_t *end,
  eastl::function<void(uint32_t start_instance, const uint32_t *types_counts, uint32_t count)> cb)
{
  FRAMEMEM_REGION;
  SmallTab<uint32_t, framemem_allocator> instanceTypesCounts;
  while (begin < end)
  {
    instanceTypesCounts.resize(0);
    const size_t handlesBatchSize = min(size_t(end - begin), (size_t)getMaxBatchSize());
    instanceTypesCounts.reserve(handlesBatchSize * 2); // worst case
    // bi-partioned buffer would be better, but this is a valid substitute
    // if buffer hasn't been used in last N frames, than we can safely start from begining using nooverwrite
    static constexpr int max_flying_frames = 6;
    const uint32_t currentFrame = dagor_frame_no() + max_flying_frames; // to avoid discard on first usage
    const bool bufferWasNotUsedRecently = (lastFrameTmsUsed + max_flying_frames < currentFrame);
    if (bufferWasNotUsedRecently)
      lastUpdatedInstance = 0; // start from begining
    lastFrameTmsUsed = currentFrame;
    const uint32_t startInstance =
      supportNoOverwrite && (lastUpdatedInstance + handlesBatchSize <= getMaxBatchSize()) ? lastUpdatedInstance : 0;
    // it is beter use bi-partioning buffer to avoid ever call with DISCARD. However, in a reality even with a free camera and 30fps we
    // can't go over that limit of 6 frames
    uint32_t actualInstances = 0;
    auto fillInstances = [&](Point4 *dataPtr) {
      auto batchEnd = begin + handlesBatchSize;
      uint32_t prevType = ~0u;
      uint32_t addedInstances = 0;
      for (; begin < batchEnd; ++begin)
      {
        const rendinst::riex_handle_t h = *begin;
        const uint32_t type = lru_collision_get_type(h);
        if (riToLRUmap.size() <= type)
          continue;
        const LRUEntry &lruEntry = riToLRUmap[type];
        if (!lruEntry.vb || !lruEntry.ib)
          continue;
        if (type != prevType)
        {
          instanceTypesCounts.push_back(prevType = type);
          instanceTypesCounts.push_back(0);
        }
        mat43f tm = lru_collision_get_transform(h);
        memcpy(dataPtr, &tm, sizeof(tm));
        dataPtr += sizeof(mat43f) / sizeof(vec4f);
        addedInstances++;
        instanceTypesCounts.back()++;
      }
      return addedInstances;
    };
    const uint32_t offsetBytes = startInstance * sizeof(mat43f);
    if (!supportNoOverwrite) // Win7 codepath
    {
      // if we don't supportNoOverwrite, then we use updateData instead, at least when size of data is small enough (<33% of buffer
      // size),
      //  which is often it is faster and less memory
      SmallTab<Point4, framemem_allocator> updateDataCopy;
      updateDataCopy.resize(handlesBatchSize * sizeof(mat43f) / sizeof(vec4f));
      actualInstances = fillInstances(updateDataCopy.data());
      if (actualInstances)
        instanceTms.getBuf()->updateData(offsetBytes, actualInstances * sizeof(mat43f), updateDataCopy.data(), VBLOCK_WRITEONLY);
    }
    else
    {
      const uint32_t flags = (startInstance != 0 || bufferWasNotUsedRecently) ? VBLOCK_NOOVERWRITE : VBLOCK_DISCARD;
      if (auto data = lock_sbuffer<Point4>(instanceTms.getBuf(), offsetBytes, handlesBatchSize * sizeof(mat43f) / sizeof(Point4),
            flags | VBLOCK_WRITEONLY))
      {
        actualInstances = fillInstances(data.get());
      }
      else
      {
        logerr("Could not lock buffer to update collision voxel scene");
        return;
      }
    }
    lastUpdatedInstance = startInstance + actualInstances;
    if (actualInstances)
      cb(startInstance, instanceTypesCounts.begin(), instanceTypesCounts.size() / 2);
  }
}

void LRURendinstCollision::drawInstances(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha,
  uint32_t instMul, ShaderElement &elem) // expect sorted
{
  TIME_D3D_PROFILE(LRU_draw_instances);
  if (alpha)
    d3d::set_rwtex(STAGE_PS, 6, alpha, 0, 0);
  if (color)
    d3d::set_rwtex(STAGE_PS, 7, color, 0, 0);

  d3d::setind(ibAllocator.getHeap().getBuf());
  d3d::setvsrc_ex(0, vbAllocator.getHeap().getBuf(), 0, sizeof(CollisionVertex)); // we can set with different offset, but we rely on
                                                                                  // same vertex size
  auto inds = serialBuf.get();
  d3d::setvsrc_ex(1, inds.get(), 0, sizeof(uint32_t));

  ShaderGlobal::set_int(gi_voxelize_with_instancingVarId, 1);
  instanceTms.setVar();
  elem.setStates(0, true);

  batchInstances(handles.begin(), handles.end(), [&](uint32_t start_instance, const uint32_t *types_counts, uint32_t count) {
    drawInstances(start_instance, types_counts, count, color, alpha, instMul);
  });
  d3d::setind(nullptr);
  d3d::setvsrc(0, 0, 0); // we can set with different offset, but we rely on same vertex size
  d3d::setvsrc(1, 0, 0); // we can set with different offset, but we rely on same vertex size
  if (color)
    d3d::set_rwtex(STAGE_PS, 7, 0, 0, 0);
  if (alpha)
    d3d::set_rwtex(STAGE_PS, 6, 0, 0, 0);
}

void LRURendinstCollision::drawInstances(uint32_t start_instance, const uint32_t *types_counts, uint32_t batches, VolTexture *color,
  VolTexture *alpha, uint32_t instMul)
{
  uint32_t offset = start_instance * instMul;
  if (multiDrawBuf.getBuf() && gi_voxelize_md.get())
  {
    TIME_D3D_PROFILE(LRU_gathered_instances_md);
    ShaderGlobal::set_int(gi_voxelize_use_multidrawVarId, 1);
    d3d::set_immediate_const(STAGE_VS, &offset, 1);                  // for conformance with no md codepath
    eastl::vector<DrawIndexedIndirectArgs, framemem_allocator> data; // todo: replace me with full gpu submission
    data.resize(batches);
    auto dataPtr = data.data(); // data.get();
    uint32_t instances = 0;
    for (uint32_t ti = 0; ti < batches; ++ti, offset += types_counts[1], types_counts += 2, dataPtr++)
    {
      const uint32_t type = types_counts[0];
      const uint32_t count = types_counts[1];
      const uint32_t instCount = count * instMul;

      const LRUEntry &lruEntry = riToLRUmap[type];
      auto vbInfo = vbAllocator.get(lruEntry.vb), ibInfo = ibAllocator.get(lruEntry.ib);
      G_ASSERT(vbInfo.size && ibInfo.size);

      dataPtr->indexCountPerInstance = ibInfo.size / sizeof(uint16_t);
      dataPtr->instanceCount = instCount;
      dataPtr->startIndexLocation = ibInfo.offset / sizeof(uint16_t);
      dataPtr->baseVertexLocation = vbInfo.offset / sizeof(CollisionVertex);
      dataPtr->startInstanceLocation = instances;
      instances += instCount;
    }
    multiDrawBuf.getBuf()->updateData(0, data.size() * sizeof(DrawIndexedIndirectArgs), data.data(), VBLOCK_WRITEONLY);

    d3d::multi_draw_indexed_indirect(PRIM_TRILIST, multiDrawBuf.getBuf(), batches, sizeof(DrawIndexedIndirectArgs));

    if (color)
      d3d::resource_barrier({color, RB_NONE, 0, 0});
    if (alpha)
      d3d::resource_barrier({alpha, RB_NONE, 0, 0});
    return;
  }
  //
  TIME_D3D_PROFILE(LRU_gathered_instances);
  ShaderGlobal::set_int(gi_voxelize_use_multidrawVarId, 0);
  for (uint32_t ti = 0; ti < batches; ++ti, types_counts += 2)
  {
    const uint32_t type = types_counts[0];
    const uint32_t count = types_counts[1];
    const uint32_t instCount = count * instMul;
    const LRUEntry &lruEntry = riToLRUmap[type];
    auto vbInfo = vbAllocator.get(lruEntry.vb), ibInfo = ibAllocator.get(lruEntry.ib);
    G_ASSERT(vbInfo.size && ibInfo.size);
    d3d::set_immediate_const(STAGE_VS, &offset, 1);
    d3d::drawind_instanced(PRIM_TRILIST, ibInfo.offset / sizeof(uint16_t), ibInfo.size / (3 * sizeof(uint16_t)),
      vbInfo.offset / sizeof(CollisionVertex), instCount);
    offset += instCount;
    if (color)
      d3d::resource_barrier({color, RB_NONE, 0, 0});
    if (alpha)
      d3d::resource_barrier({alpha, RB_NONE, 0, 0});
  }
}

void LRURendinstCollision::dispatchInstances(const uint32_t start_instance, const uint32_t *types_counts, uint32_t batches,
  VolTexture *color, VolTexture *alpha, uint32_t threads)
{
  // uint32_t triCount = 0;
  uint32_t offset = start_instance;
  instanceTms.setVar();
  for (uint32_t ti = 0; ti < batches; ++ti, offset += types_counts[1], types_counts += 2)
  {
    const uint32_t type = types_counts[0];
    const uint32_t count = types_counts[1];
    G_ASSERT(riToLRUmap.size() > type);
    const LRUEntry &lruEntry = riToLRUmap[type];
    auto vbInfo = vbAllocator.get(lruEntry.vb), ibInfo = ibAllocator.get(lruEntry.ib);
    G_ASSERT(vbInfo.size && ibInfo.size);
    const uint32_t tris = (ibInfo.size / (3 * sizeof(uint16_t)));
    const uint32_t renderInfo[4] = {uint32_t(ibInfo.offset / sizeof(uint16_t)), uint32_t(vbInfo.offset / sizeof(CollisionVertex)),
      uint32_t(tris | (count << 20)), offset};
    d3d::set_immediate_const(STAGE_CS, renderInfo, 4);
    // draw n triangles. We need clusters or multidispatch indirect, too much calls otherwise!
    d3d::dispatch((count * tris + threads - 1) / threads, 1, 1);
    // triCount+=count*tris;
    if (color)
      d3d::resource_barrier({color, RB_NONE, 0, 0});
    if (alpha)
      d3d::resource_barrier({alpha, RB_NONE, 0, 0});
  }
  // debug("triCount = %d instances = %d", triCount, offset);
  // d3d::set_immediate_const(STAGE_CS, &offset, 1);
  // d3d::dispatch((offset+31)/32,1,1);
}

void LRURendinstCollision::dispatchInstances(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha,
  ComputeShaderElement &cs) // expect sorted
{
  G_ASSERT(ibAllocator.getHeap().getBuf()->getFlags() & SBCF_BIND_SHADER_RES);
  TIME_D3D_PROFILE(LRU_dispatch_instances);
  if (alpha)
    d3d::set_rwtex(STAGE_CS, 6, alpha, 0, 0);
  if (color)
    d3d::set_rwtex(STAGE_CS, 7, color, 0, 0);
  cs.setStates();
  ShaderGlobal::set_buffer(gi_voxelization_vbufferVarId, vbAllocator.getHeap().getBufId());
  ShaderGlobal::set_buffer(gi_voxelization_ibufferVarId, ibAllocator.getHeap().getBufId());
  batchInstances(handles.begin(), handles.end(), [&](uint32_t start_instance, const uint32_t *types_counts, uint32_t count) {
    dispatchInstances(start_instance, types_counts, count, color, alpha, cs.getThreadGroupSizes()[0]);
  });
  if (color)
    d3d::set_rwtex(STAGE_CS, 7, 0, 0, 0);
  if (alpha)
    d3d::set_rwtex(STAGE_CS, 6, 0, 0, 0);
}

void LRURendinstCollision::draw(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha,
  uint32_t inst_mul, ShaderElement &elem)
{
  if (!handles.empty())
    drawInstances(handles, color, alpha, inst_mul, elem);
}

void LRURendinstCollision::voxelize(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha)
{
  if (handles.empty())
    return;
  ShaderGlobal::set_int(gi_voxelize_with_colorsVarId, color ? 1 : 0);
  if (voxelize_collision_cs && gi_voxelize_cs.get() &&
      (ibAllocator.getHeap().getBuf() && ibAllocator.getHeap().getBuf()->getFlags() & SBCF_BIND_SHADER_RES)) // otherwise we can't
                                                                                                             // dispatch
  {
    dispatchInstances(handles, color, alpha, *voxelize_collision_cs);
  }
  else
  {
    if (voxelizeCollisionElem)
      drawInstances(handles, color, alpha, use_geom_shader ? 1 : 3, *voxelizeCollisionElem);
  }
}

void LRURendinstCollision::clearRiInfo()
{
  reset();
  riInfo = {};
}
