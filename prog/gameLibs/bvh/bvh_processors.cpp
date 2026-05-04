// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include "bvh_context.h"

#include <drv/3d/dag_shaderConstants.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_hlsl_floatx.h>
#include "shaders/bvh_process_tree_vertices.hlsli"
#include "shaders/bvh_skinned_instance_data_batched.hlsli"

#if BVH_PROFILING_ENABLED
#define BVH_PROFILE TIME_PROFILE
#else
#define BVH_PROFILE(...)
#endif

namespace bvh
{

static bool allocate(ContextId context_id, UniqueOrReferencedBVHBuffer &buffer, uint32_t &bindless_id, uint32_t size,
  const String &name)
{
  if (buffer.unique)
  {
    buffer.unique->reset(
      d3d::buffers::create_ua_sr_byte_address(size / sizeof(uint32_t), name.data(), d3d::buffers::Init::No, RESTAG_BVH));
    context_id->holdBuffer(buffer.unique->get(), bindless_id);
    HANDLE_LOST_DEVICE_STATE(buffer.unique, false);
  }
  else if (buffer.referenced)
  {
    static constexpr int pageSize = 4 << 20;

    int allocatorUsed = -1;
    LinearHeapAllocatorSbuffer::RegionId id = {};
    LinearHeapAllocatorSbuffer::Region region = {};
    for (auto [index, allocator] : enumerate(context_id->processBufferAllocator))
    {
      id = allocator.first.allocateInHeap(size);
      region = allocator.first.get(id);
      if (region.size)
      {
        allocatorUsed = index;
        break;
      }
    }

    if (allocatorUsed < 0)
    {
      logdbg("BVH is allocating new process buffer allocator. We have %d allocators now. With the page size of %dMB, total allocation "
             "size is now %dMB",
        context_id->processBufferAllocator.size() + 1, pageSize >> 20,
        ((context_id->processBufferAllocator.size() + 1) * pageSize) >> 20);
      auto &[allocator, bindlessId] =
        context_id->processBufferAllocator.emplace_back(eastl::pair<LinearHeapAllocatorSbuffer, uint32_t>{
          LinearHeapAllocatorSbuffer{SbufferHeapManager(String(32, "bvh_process_buffer_%d", context_id->processBufferAllocator.size()),
                                       d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE, SBCF_UA_SR_BYTE_ADDRESS),
            RESTAG_BVH},
          -1});
      id = allocator.allocate(size, pageSize);
      region = allocator.get(id);
      G_ASSERT_RETURN(region.size, false);
      allocatorUsed = context_id->processBufferAllocator.size() - 1;

      context_id->holdBuffer(allocator.getHeap().getBuf(), bindlessId);
    }

    buffer.referenced->allocator = allocatorUsed;
    buffer.referenced->allocId = id;
    buffer.referenced->buffer = context_id->processBufferAllocator[allocatorUsed].first.getHeap().getBuf();
    buffer.referenced->size = region.size;
    buffer.referenced->offset = region.offset;
    bindless_id = context_id->processBufferAllocator[allocatorUsed].second;
    HANDLE_LOST_DEVICE_STATE(*buffer.referenced, false);
  }
  else
  {
    G_ASSERT_RETURN(false, false);
  }

  return true;
}

static void set_offset(UniqueOrReferencedBVHBuffer &buf)
{
  static int bvh_process_target_offsetVarId = get_shader_variable_id("bvh_process_target_offset");

  if (!buf)
    ShaderGlobal::set_int(bvh_process_target_offsetVarId, 0);
  if (buf.unique)
    ShaderGlobal::set_int(bvh_process_target_offsetVarId, 0);
  if (buf.referenced)
    ShaderGlobal::set_int(bvh_process_target_offsetVarId, buf.referenced->offset);
}

static void set_offset(UniqueBVHBufferWithOffset &buf)
{
  static int bvh_process_target_offsetVarId = get_shader_variable_id("bvh_process_target_offset");

  ShaderGlobal::set_int(bvh_process_target_offsetVarId, buf.offset);
}

static void set_offset(BVHGeometryBufferWithOffset &buf, bool ib)
{
  static int bvh_process_target_offsetVarId = get_shader_variable_id("bvh_process_target_offset");

  ShaderGlobal::set_int(bvh_process_target_offsetVarId, ib ? 0 : buf.vbOffset);
}

static void set_source_offset(int off)
{
  static int bvh_process_source_offsetVarId = get_shader_variable_id("bvh_process_source_offset");

  ShaderGlobal::set_int(bvh_process_source_offsetVarId, off);
}

void IndexProcessor::process(Sbuffer *source, BVHGeometryBufferWithOffset &processed_buffer, int index_format, int index_count,
  int index_start, int start_vertex, ContextId context_id)
{
#define GLOBAL_VARS_LIST                         \
  VAR(bvh_process_dynrend_indices_start)         \
  VAR(bvh_process_dynrend_indices_start_aligned) \
  VAR(bvh_process_dynrend_indices_count)         \
  VAR(bvh_process_dynrend_indices_vertex_base)   \
  VAR(bvh_process_dynrend_indices_size)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_dynrend_indices_source_const_no");
  static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_dynrend_indices_output_uav_no");

  BVH_PROFILE(IndexProcessor);

  G_ASSERT(index_format == 2);
  auto dwordCount = (index_count + 1) / 2;

  if (!outputs[0])
    for (auto [index, buffer] : enumerate(outputs))
    {
      // A 16 bit index buffer can't be larger than this.
      String name(32, "bvh_IndexProcessor_%u", ++index);
      buffer.reset(d3d::buffers::create_ua_byte_address((64 * 1024 * 2) / 4, name.data(), RESTAG_BVH));
      HANDLE_LOST_DEVICE_STATE(buffer, );
    }

  bool deleteIPO = false;

  Sbuffer *indexProcessorOutput = outputs[ringCursor].get();
  if (indexProcessorOutput->getSize() >= dwordCount * 4)
    ringCursor = (ringCursor + 1) % countof(outputs);
  else
  {
    String name(32, "bvh_IndexProcessor");
    indexProcessorOutput = d3d::buffers::create_ua_byte_address(dwordCount, name.data(), RESTAG_BVH);
    HANDLE_LOST_DEVICE_STATE(indexProcessorOutput, );
    deleteIPO = true;
  }

  d3d::set_buffer(STAGE_CS, source_const_no, source);
  d3d::set_rwbuffer(STAGE_CS, output_uav_no, indexProcessorOutput);

  set_offset(processed_buffer, true);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_startVarId, index_start);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_start_alignedVarId, ((index_start * 2) & 3) ? 0 : 1);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_countVarId, dwordCount);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_vertex_baseVarId, start_vertex);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_sizeVarId, index_format);
  G_ASSERT(shader);
  shader->dispatchThreads(dwordCount, 1, 1);

  indexProcessorOutput->copyTo(processed_buffer.getIndexBuffer(context_id), processed_buffer.ibOffset, 0, dwordCount * 4);

  if (deleteIPO)
    del_d3dres(indexProcessorOutput);
}

bool SkinnedVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                                \
  VAR(bvh_process_skinned_vertices_start)               \
  VAR(bvh_process_skinned_vertices_stride)              \
  VAR(bvh_process_skinned_vertices_count)               \
  VAR(bvh_process_skinned_vertices_processed_stride)    \
  VAR(bvh_process_skinned_vertices_position_offset)     \
  VAR(bvh_process_skinned_vertices_skin_indices_offset) \
  VAR(bvh_process_skinned_vertices_skin_weights_offset) \
  VAR(bvh_process_skinned_vertices_normal_offset)       \
  VAR(bvh_process_skinned_vertices_color_offset)        \
  VAR(bvh_process_skinned_vertices_texcoord_offset)     \
  VAR(bvh_process_skinned_vertices_texcoord_size)       \
  VAR(bvh_process_skinned_vertices_position_packed)     \
  VAR(bvh_process_skinned_vertices_pos_mul)             \
  VAR(bvh_process_skinned_vertices_pos_ofs)             \
  VAR(bvh_process_skinned_vertices_inv_wtm)             \
  VAR(bvh_process_skinned_vertices_pos_format_half)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(SkinnedVertexProcessor);

  G_ASSERT(args.setTransformsFn);
  G_ASSERT(args.positionFormat == VSDT_SHORT4N || args.positionFormat == VSDT_FLOAT3);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.colorOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_skinnedVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_skinned_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_skinned_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    args.setTransformsFn();

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_processed_strideVarId, vertexSize);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_position_offsetVarId, args.positionOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_skin_indices_offsetVarId, args.indicesOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_skin_weights_offsetVarId, args.weightsOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_normal_offsetVarId, args.normalOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_color_offsetVarId, args.colorOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_texcoord_offsetVarId, args.texcoordOffset);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_texcoord_sizeVarId, tcSize);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_position_packedVarId, args.positionFormat != VSDT_FLOAT3 ? 1 : 0);
    ShaderGlobal::set_float4(bvh_process_skinned_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_float4(bvh_process_skinned_vertices_pos_ofsVarId, args.posAdd);
    ShaderGlobal::set_float4x4(bvh_process_skinned_vertices_inv_wtmVarId, args.invWorldTm);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_pos_format_halfVarId, args.positionFormat == VSDT_SHORT4N ? 1 : 0);
    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;
  args.positionOffset = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  if (args.colorOffset != -1)
  {
    args.colorOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

bool SkinnedVertexProcessorBatched::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source);
  BVH_PROFILE(SkinnedVertexProcessorBatched);

  G_ASSERT(args.setTransformsFn);
  G_ASSERT(args.positionFormat == VSDT_SHORT4N || args.positionFormat == VSDT_FLOAT3);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.colorOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_skinnedVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    auto &dispatchData = dispatchDataMapping[processed_buffer.get()];

    dispatchData.maxVertexCount = max(args.vertexCount, dispatchData.maxVertexCount);

    // TODO Callback was made for non-batched variant, so it doesn't have a return value
    // So as a workaround it sets to a global variable, and then we copy it
    args.setTransformsFn();
    uint32_t instanceOffset = lastInstanceOffset;

    BvhSkinnedInstanceData &params = dispatchData.instanceData.push_back();
    params.inv_wtm = args.invWorldTm;
    params.target_offset = processed_buffer.referenced ? processed_buffer.referenced->offset : 0;
    params.source_offset = source_offset;
    params.start_vertex = args.baseVertex + args.startVertex;
    params.vertex_stride = args.vertexStride;
    params.vertex_count = args.vertexCount;
    params.processed_vertex_stride = vertexSize;
    params.position_offset = args.positionOffset;
    params.skin_indices_offset = args.indicesOffset;
    params.skin_weights_offset = args.weightsOffset;
    params.color_offset = args.colorOffset;
    params.normal_offset = args.normalOffset;
    params.texcoord_offset = args.texcoordOffset;
    params.texcoord_size = tcSize;
    params.instance_offset = instanceOffset;
    params.source_slot = source_buffer_bindless;
    params.pos_format_half = args.positionFormat == VSDT_SHORT4N ? 1 : 0;
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;
  args.positionOffset = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  if (args.colorOffset != -1)
  {
    args.colorOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

void SkinnedVertexProcessorBatched::begin() const {}

void SkinnedVertexProcessorBatched::end(bool is_protype_building) const
{
#define GLOBAL_VARS_LIST                       \
  VAR(bvh_process_skinned_vertices_params_buf) \
  VAR(bvh_process_skinned_vertices_is_building_prototype)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_skinned_vertices_output_uav_no");

  TIME_D3D_PROFILE(SkinnedVertexProcessorBatched::end)
  updateData();

  int dispatches = 0;
  int instances = 0;
  {
    ShaderGlobal::set_buffer(bvh_process_skinned_vertices_params_bufVarId, instanceDataBuffer);
    ShaderGlobal::set_int(bvh_process_skinned_vertices_is_building_prototypeVarId, is_protype_building);
    for (auto &[targetBuffer, dispatchData] : dispatchDataMapping)
    {
      if (dispatchData.instanceData.empty())
        continue;
      G_ASSERT(dispatchData.maxVertexCount > 0);

      d3d::set_rwbuffer(STAGE_CS, output_uav_no, targetBuffer);
      uint32_t immediateConst[] = {(uint32_t)instances, (uint32_t)dispatchData.instanceData.size()};
      d3d::set_immediate_const(STAGE_CS, immediateConst, 2);

      G_ASSERT(shader);
      shader->dispatchThreads(dispatchData.maxVertexCount, dispatchData.instanceData.size(), 1);
      dispatches++;
      instances += dispatchData.instanceData.size();
    }
    d3d::set_immediate_const(STAGE_CS, nullptr, 0);
  }
  DA_PROFILE_TAG(SkinnedVertexProcessorBatched::end, "dispatches: %d, instances: %d", dispatches, instances);

  // Do NOT free memory, it'll just need to be reallocated next frame and that's slow
  for (auto &[_, dispatchData] : dispatchDataMapping)
  {
    dispatchData.instanceData.clear();
    dispatchData.maxVertexCount = 0;
  }
}

void SkinnedVertexProcessorBatched::updateData() const
{
  TIME_D3D_PROFILE(updateData);

  uint32_t targetSize = 0;
  for (auto &[_, dispatchData] : dispatchDataMapping)
    targetSize += dispatchData.instanceData.size();

  size_t bufferSize = instanceDataBuffer ? instanceDataBuffer->getNumElements() : 0;
  if (bufferSize < targetSize)
  {
    instanceDataBuffer = dag::buffers::create_one_frame_sr_structured(sizeof(BvhSkinnedInstanceData), targetSize,
      "bvh_skinned_vertex_processor_instance_data", RESTAG_BVH);
    debug("[FRAMEMEM] Allocated buffer for BVH: 'bvh_skinned_vertex_processor_instance_data': %d",
      sizeof(BvhSkinnedInstanceData) * targetSize);
  }

  if (targetSize > 0)
  {
    auto upload = lock_sbuffer<uint8_t>(instanceDataBuffer.getBuf(), 0, targetSize * sizeof(BvhSkinnedInstanceData),
      VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    HANDLE_LOST_DEVICE_STATE(upload, );

    auto cursor = upload.get();
    for (auto &[_, dispatchData] : dispatchDataMapping)
    {
      if (dispatchData.instanceData.empty())
        continue;
      size_t dataSize = data_size(dispatchData.instanceData);
      memcpy(cursor, dispatchData.instanceData.data(), dataSize);
      cursor += dataSize;
    }
  }
}

SkinnedVertexProcessorBatched::DispatchData::~DispatchData() = default;


TreeVertexProcessor::VariantKey TreeVertexProcessor::packVariants(bool is_pos_instance, bool is_pivoted)
{
  return uint32_t(is_pos_instance) | (uint32_t(is_pivoted) << 1);
}

void TreeVertexProcessor::unpackVariants(VariantKey key, bool &is_pos_instance, bool &is_pivoted)
{
  is_pos_instance = key & 1;
  is_pivoted = key & (1 << 1);
}

bool TreeVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source);
  BVH_PROFILE(TreeVertexProcessor);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_treeVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    G_ASSERT(args.positionFormat == VSDT_FLOAT3);

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    VariantKey variantKey = packVariants(args.tree.isPosInstance, args.tree.isPivoted);
    auto &dispatchData = dispatchDataMapping[variantKey][processed_buffer.get()];

    dispatchData.maxVertexCount = max(args.vertexCount, dispatchData.maxVertexCount);

    BvhTreeInstanceData &params = dispatchData.instanceData.push_back();
    params.wtm = worldTm.transpose();
    params.packed_itm = args.invWorldTm;
    params.wind_per_level_angle_rot_max = args.tree.ppWindPerLevelAngleRotMax;
    params.wind_channel_strength = args.tree.windChannelStrength;
    params.wind_noise_speed_base = args.tree.ppWindNoiseSpeedBase;
    params.wind_noise_speed_level_mul = args.tree.ppWindNoiseSpeedLevelMul;
    params.wind_angle_rot_base = args.tree.ppWindAngleRotBase;
    params.wind_angle_rot_level_mul = args.tree.ppWindAngleRotLevelMul;
    params.wind_parent_contrib = args.tree.ppWindParentContrib;
    params.wind_motion_damp_base = args.tree.ppWindMotionDampBase;
    params.wind_motion_damp_level_mul = args.tree.ppWindMotionDampLevelMul;
    params.AnimWindScale = args.tree.AnimWindScale;
    params.apply_tree_wind = args.tree.apply_tree_wind ? 1 : 0;
    params.target_offset = processed_buffer.referenced ? processed_buffer.referenced->offset : 0;
    params.start_vertex = args.baseVertex + args.startVertex;
    params.vertex_stride = args.vertexStride;
    params.vertex_count = args.vertexCount;
    params.processed_vertex_stride = vertexSize;
    params.position_offset = args.positionOffset;
    params.color_offset = args.colorOffset;
    params.normal_offset = args.normalOffset;
    params.texcoord_offset = args.texcoordOffset;
    params.texcoord_size = tcSize;
    params.indirect_texcoord_offset = args.secTexcoordOffset;
    params.perInstanceRenderAdditionalData = args.tree.perInstanceRenderAdditionalData;
    params.sourceOffset = source_offset;
    params.ground_snap_params = float4(args.tree.groundSnapHeightSoft, safeinv(args.tree.groundSnapHeightSoft),
      args.tree.groundSnapHeightFull, safeinv(args.tree.groundSnapHeightFull));
    params.ground_bend_params = float4(args.tree.groundBendHeight, safeinv(args.tree.groundBendHeight),
      args.tree.groundBendTangentOffset, args.tree.groundBendNormalOffset);
    params.ground_snap_normal_offset = args.tree.groundSnapNormalOffset;
    params.ground_snap_limit = args.tree.groundSnapLimit;
    G_ASSERTF(!args.tree.isPivoted || (args.tree.ppPositionBindless != 0xFFFFFFFF && args.tree.ppDirectionBindless != 0xFFFFFFFF),
      "Pivoted tree has invalid bindless pivot textures!");
    params.pp_pos_tex_slot = args.tree.ppPositionBindless;
    params.pp_dir_tex_slot = args.tree.ppDirectionBindless;
    params.source_slot = source_buffer_bindless;
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;
  args.positionOffset = 0;
  args.colorOffset = -1;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

void TreeVertexProcessor::begin() const {}

void TreeVertexProcessor::updateData() const
{
  TIME_D3D_PROFILE(updateData);

  uint32_t targetSize = 0;
  for (auto &[_, mapping] : dispatchDataMapping)
    for (auto &[_, dispatchData] : mapping)
      targetSize += dispatchData.instanceData.size();

  size_t bufferSize = instanceDataBuffer ? instanceDataBuffer->getNumElements() : 0;
  if (bufferSize < targetSize)
  {
    instanceDataBuffer = dag::create_sbuffer(sizeof(BvhTreeInstanceData), targetSize,
      SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED | SBCF_DYNAMIC, 0, "bvh_tree_vertex_processor_instance_data", RESTAG_BVH);
    // TODO: trim this buffer periodically
    debug("[FRAMEMEM] Allocated buffer for BVH: 'bvh_tree_vertex_processor_instance_data': %d",
      sizeof(BvhTreeInstanceData) * targetSize);
  }

  if (targetSize > 0)
  {
    auto upload = lock_sbuffer<uint8_t>(instanceDataBuffer.getBuf(), 0, targetSize * sizeof(BvhTreeInstanceData),
      VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    HANDLE_LOST_DEVICE_STATE(upload, );

    auto cursor = upload.get();
    for (auto &[variantKey, mapping] : dispatchDataMapping)
    {
      for (auto &[targetBuffer, dispatchData] : mapping)
      {
        if (dispatchData.instanceData.empty())
          continue;
        size_t dataSize = data_size(dispatchData.instanceData);
        memcpy(cursor, dispatchData.instanceData.data(), dataSize);
        cursor += dataSize;
      }
    }
  }
}

void TreeVertexProcessor::end(bool is_prototype_buidling) const
{
  G_UNUSED(is_prototype_buidling);
#define GLOBAL_VARS_LIST                     \
  VAR(bvh_process_tree_vertices_is_pos_inst) \
  VAR(bvh_process_tree_vertices_is_pivoted)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST
  static int bvh_process_tree_vertices_blockId = ShaderGlobal::getBlockId("bvh_process_tree_vertices_block");
  static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_output_uav_no");
  static int sampler_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_pp_sampler_no");
  static int params_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_params_no");
  static d3d::SamplerHandle ppSampler = d3d::request_sampler(d3d::SamplerInfo());


  TIME_D3D_PROFILE(TreeVertexProcessor::end)
  updateData();

  int dispatches = 0;
  int instances = 0;
  {
    SCENE_LAYER_GUARD(bvh_process_tree_vertices_blockId);
    d3d::set_sampler(STAGE_CS, sampler_no, ppSampler);
    d3d::set_buffer(STAGE_CS, params_no, instanceDataBuffer.getBuf());
    for (auto &[variantKey, mapping] : dispatchDataMapping)
    {
      bool isPosInst;
      bool isPivoted;
      unpackVariants(variantKey, isPosInst, isPivoted);
      ShaderGlobal::set_int(bvh_process_tree_vertices_is_pos_instVarId, isPosInst);
      ShaderGlobal::set_int(bvh_process_tree_vertices_is_pivotedVarId, isPivoted);

      for (auto &[targetBuffer, dispatchData] : mapping)
      {
        if (dispatchData.instanceData.empty())
          continue;
        G_ASSERT(dispatchData.maxVertexCount > 0);
        d3d::set_rwbuffer(STAGE_CS, output_uav_no, targetBuffer);
        uint32_t immediateConst[] = {(uint32_t)instances, (uint32_t)dispatchData.instanceData.size()};
        d3d::set_immediate_const(STAGE_CS, immediateConst, 2);

        G_ASSERT(shader);
        shader->dispatchThreads(dispatchData.maxVertexCount, dispatchData.instanceData.size(), 1);
        dispatches++;
        instances += dispatchData.instanceData.size();
      }
    }
    d3d::set_immediate_const(STAGE_CS, nullptr, 0);
  }
  DA_PROFILE_TAG(TreeVertexProcessor::end, "dispatches: %d, instances: %d", dispatches, instances);

  // Do NOT free memory, it'll just need to be reallocated next frame and that's slow
  for (auto &[_, mapping] : dispatchDataMapping)
    for (auto &[_, dispatchData] : mapping)
    {
      dispatchData.instanceData.clear();
      dispatchData.maxVertexCount = 0;
    }
}

TreeVertexProcessor::DispatchData::~DispatchData() = default;

bool ImpostorVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                                 \
  VAR(bvh_process_impostor_vertices_start)               \
  VAR(bvh_process_impostor_vertices_stride)              \
  VAR(bvh_process_impostor_vertices_count)               \
  VAR(bvh_process_impostor_vertices_scale)               \
  VAR(bvh_process_impostor_vertices_height_offset)       \
  VAR(bvh_process_impostor_vertices_sliceTcTm1)          \
  VAR(bvh_process_impostor_vertices_sliceTcTm2)          \
  VAR(bvh_process_impostor_vertices_sliceClippingLines1) \
  VAR(bvh_process_impostor_vertices_sliceClippingLines2) \
  VAR(bvh_process_impostor_vertices_vertex_offsets)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(ImpostorVertexProcessor);

  static constexpr int outputVertexSize = sizeof(Point3) + sizeof(Point2) + sizeof(Point3) + sizeof(uint32_t);

  G_ASSERT(args.vertexStride == sizeof(Point3));
  G_ASSERT(args.positionOffset == 0);
  G_ASSERT(args.positionFormat == VSDT_FLOAT3);

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_instanceVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, outputVertexSize * args.vertexCount * 2, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_impostor_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_impostor_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_float(bvh_process_impostor_vertices_height_offsetVarId, args.impostorHeightOffset);
    ShaderGlobal::set_float4(bvh_process_impostor_vertices_scaleVarId, args.impostorScale);
    ShaderGlobal::set_float4(bvh_process_impostor_vertices_sliceTcTm1VarId, args.impostorSliceTm1);
    ShaderGlobal::set_float4(bvh_process_impostor_vertices_sliceTcTm2VarId, args.impostorSliceTm2);
    ShaderGlobal::set_float4(bvh_process_impostor_vertices_sliceClippingLines1VarId, args.impostorSliceClippingLines1);
    ShaderGlobal::set_float4(bvh_process_impostor_vertices_sliceClippingLines2VarId, args.impostorSliceClippingLines2);
    ShaderGlobal::set_float4_array(bvh_process_impostor_vertices_vertex_offsetsVarId, args.impostorOffsets, 4);
    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.texcoordFormat = VSDT_FLOAT2;
  args.texcoordOffset = sizeof(Point3);
  args.colorOffset = args.texcoordOffset + sizeof(Point2);
  args.normalOffset = args.colorOffset + sizeof(Point3);
  args.vertexStride = outputVertexSize;
  args.startVertex = 0;

  return !skip_processing;
}

bool BakeTextureToVerticesProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                                     \
  VAR(bvh_process_bake_texture_to_vertices_start)            \
  VAR(bvh_process_bake_texture_to_vertices_stride)           \
  VAR(bvh_process_bake_texture_to_vertices_count)            \
  VAR(bvh_process_bake_texture_to_vertices_processed_stride) \
  VAR(bvh_process_bake_texture_to_vertices_position_offset)  \
  VAR(bvh_process_bake_texture_to_vertices_position_size)    \
  VAR(bvh_process_bake_texture_to_vertices_normal_offset)    \
  VAR(bvh_process_bake_texture_to_vertices_color_offset)     \
  VAR(bvh_process_bake_texture_to_vertices_texcoord_offset)  \
  VAR(bvh_process_bake_texture_to_vertices_texcoord_format)  \
  VAR(bvh_process_bake_texture_to_vertices_texcoord_size)    \
  VAR(bvh_process_bake_texture_to_vertices_texture_lod)      \
  VAR(bvh_process_bake_texture_to_vertices_texture)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  G_ASSERT(args.texture == BAD_TEXTUREID || args.texcoordOffset != MeshInfo::invalidOffset);
  G_ASSERT(get_managed_res_loaded_lev(args.texture) >= args.textureLevel);

  BVH_PROFILE(BakeTextureToVerticesProcessor);

  unsigned posSize, tcSize;
  channel_size(args.positionFormat, posSize);
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = posSize + sizeof(uint32_t) + tcSize;
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_bakeTextureProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_bake_texture_to_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_bake_texture_to_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    if (args.setTransformsFn)
      args.setTransformsFn();

    int loadedLevel = get_managed_res_loaded_lev(args.texture);
    int usedMip = loadedLevel - args.textureLevel;

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_processed_strideVarId, vertexSize);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_position_offsetVarId, args.positionOffset);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_position_sizeVarId, posSize);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_normal_offsetVarId, args.normalOffset);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_color_offsetVarId, args.colorOffset);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_texcoord_offsetVarId, args.texcoordOffset);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_texcoord_formatVarId, args.texcoordFormat);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_texcoord_sizeVarId, tcSize);
    ShaderGlobal::set_int(bvh_process_bake_texture_to_vertices_texture_lodVarId, usedMip);

    ShaderGlobal::set_texture(bvh_process_bake_texture_to_vertices_textureVarId, args.texture);
    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.vertexStride = vertexSize;
  args.startVertex = 0;

  args.positionOffset = 0;
  int offset = posSize;

  args.colorOffset = offset;
  offset += sizeof(uint32_t);

  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }

  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }

  G_ASSERT(offset == vertexSize);

  return !skip_processing;
}

bool LeavesVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                  \
  VAR(bvh_process_leaves_vertices_start)  \
  VAR(bvh_process_leaves_vertices_stride) \
  VAR(bvh_process_leaves_vertices_count)  \
  VAR(bvh_process_leaves_vertices_wtm)    \
  VAR(bvh_process_leaves_vertices_itm)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(LeavesVertexProcessor);

  // Position + texcoord + normal
  static constexpr int outputVertexSize = sizeof(uint32_t) * 2 + sizeof(uint32_t) + sizeof(uint32_t);

  // TODO add asserts for input format
  G_ASSERT(args.vertexStride == 24);
  G_ASSERT(args.positionOffset == 0);
  G_ASSERT(args.positionFormat == VSDT_HALF4);
  G_ASSERT(args.texcoordOffset == 8);
  G_ASSERT(args.texcoordFormat == bvhAttributeShort2TC);
  G_ASSERT(args.secTexcoordOffset == 12);
  G_ASSERT(args.normalOffset == 20);

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_leavesVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, outputVertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }
  else
    skip_processing = !args.recycled;

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_leaves_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_leaves_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_leaves_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_leaves_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_leaves_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_float4x4(bvh_process_leaves_vertices_wtmVarId, worldTm.transpose());
    ShaderGlobal::set_float4x4(bvh_process_leaves_vertices_itmVarId, args.invWorldTm);

    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.normalOffset = args.secTexcoordOffset;
  args.vertexStride = outputVertexSize;
  args.startVertex = 0;

  return !skip_processing;
}

bool HeliRotorVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                                \
  VAR(bvh_process_heli_rotor_vertices_start)            \
  VAR(bvh_process_heli_rotor_vertices_stride)           \
  VAR(bvh_process_heli_rotor_vertices_count)            \
  VAR(bvh_process_heli_rotor_vertices_processed_stride) \
  VAR(bvh_process_heli_rotor_vertices_position_offset)  \
  VAR(bvh_process_heli_rotor_vertices_params)           \
  VAR(bvh_process_heli_rotor_vertices_sec_params)       \
  VAR(bvh_process_heli_rotor_vertices_normal_offset)    \
  VAR(bvh_process_heli_rotor_vertices_color_offset)     \
  VAR(bvh_process_heli_rotor_vertices_texcoord_offset)  \
  VAR(bvh_process_heli_rotor_vertices_texcoord_size)    \
  VAR(bvh_process_heli_rotor_vertices_pos_mul)          \
  VAR(bvh_process_heli_rotor_vertices_pos_ofs)          \
  VAR(bvh_process_heli_rotor_vertices_wtm)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(HeliRotorVertexProcessor);

  G_ASSERT(args.getHeliParamsFn);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.colorOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_heliRotorVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_heli_rotor_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_heli_rotor_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    Point4 params = Point4::ZERO, secParams = Point4::ZERO;
    args.getHeliParamsFn(params, secParams);

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_processed_strideVarId, vertexSize);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_position_offsetVarId, args.positionOffset);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_normal_offsetVarId, args.normalOffset);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_color_offsetVarId, args.colorOffset);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_texcoord_offsetVarId, args.texcoordOffset);
    ShaderGlobal::set_int(bvh_process_heli_rotor_vertices_texcoord_sizeVarId, tcSize);
    ShaderGlobal::set_float4(bvh_process_heli_rotor_vertices_paramsVarId, params);
    ShaderGlobal::set_float4(bvh_process_heli_rotor_vertices_sec_paramsVarId, secParams);
    ShaderGlobal::set_float4(bvh_process_heli_rotor_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_float4(bvh_process_heli_rotor_vertices_pos_ofsVarId, args.posAdd);
    ShaderGlobal::set_float4x4(bvh_process_heli_rotor_vertices_wtmVarId, worldTm.transpose());
    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;
  args.positionOffset = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  if (args.colorOffset != -1)
  {
    args.colorOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

bool FlagVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                             \
  VAR(bvh_process_flag_vertices_start)               \
  VAR(bvh_process_flag_vertices_stride)              \
  VAR(bvh_process_flag_vertices_count)               \
  VAR(bvh_process_flag_vertices_processed_stride)    \
  VAR(bvh_process_flag_vertices_position_offset)     \
  VAR(bvh_process_flag_vertices_normal_offset)       \
  VAR(bvh_process_flag_vertices_color_offset)        \
  VAR(bvh_process_flag_vertices_texcoord_offset)     \
  VAR(bvh_process_flag_vertices_texcoord_size)       \
  VAR(bvh_process_flag_vertices_wtm)                 \
  VAR(bvh_process_flag_vertices_itm)                 \
  VAR(bvh_process_flag_vertices_hash_val)            \
  VAR(bvh_process_flag_vertices_wind_type)           \
  VAR(bvh_process_flag_vertices_frequency_amplitude) \
  VAR(bvh_process_flag_vertices_wind_direction)      \
  VAR(bvh_process_flag_vertices_wind_strength)       \
  VAR(bvh_process_flag_vertices_wave_length)         \
  VAR(bvh_process_flag_vertices_flagpole_pos_0)      \
  VAR(bvh_process_flag_vertices_flagpole_pos_1)      \
  VAR(bvh_process_flag_vertices_stiffness)           \
  VAR(bvh_process_flag_vertices_flag_movement_scale) \
  VAR(bvh_process_flag_vertices_bend)                \
  VAR(bvh_process_flag_vertices_deviation)           \
  VAR(bvh_process_flag_vertices_stretch)             \
  VAR(bvh_process_flag_vertices_flag_length)         \
  VAR(bvh_process_flag_vertices_sway_speed)          \
  VAR(bvh_process_flag_vertices_width_type)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(FlagsVertexProcessor);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.colorOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_flagVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_flag_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_flag_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_flag_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_flag_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_flag_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_flag_vertices_processed_strideVarId, vertexSize);
    ShaderGlobal::set_int(bvh_process_flag_vertices_position_offsetVarId, args.positionOffset);
    ShaderGlobal::set_int(bvh_process_flag_vertices_normal_offsetVarId, args.normalOffset);
    ShaderGlobal::set_int(bvh_process_flag_vertices_color_offsetVarId, args.colorOffset);
    ShaderGlobal::set_int(bvh_process_flag_vertices_texcoord_offsetVarId, args.texcoordOffset);
    ShaderGlobal::set_int(bvh_process_flag_vertices_texcoord_sizeVarId, tcSize);
    ShaderGlobal::set_float4x4(bvh_process_flag_vertices_wtmVarId, worldTm.transpose());
    ShaderGlobal::set_float4x4(bvh_process_flag_vertices_itmVarId, args.invWorldTm);

    ShaderGlobal::set_int(bvh_process_flag_vertices_hash_valVarId, args.flag.hashVal);
    ShaderGlobal::set_int(bvh_process_flag_vertices_wind_typeVarId, args.flag.windType);

    if (args.flag.windType == 0)
    {
      ShaderGlobal::set_float4(bvh_process_flag_vertices_frequency_amplitudeVarId, args.flag.fixedWind.frequencyAmplitude);
      ShaderGlobal::set_float4(bvh_process_flag_vertices_wind_directionVarId, args.flag.fixedWind.windDirection);
      ShaderGlobal::set_float(bvh_process_flag_vertices_wind_strengthVarId, args.flag.fixedWind.windStrength);
      ShaderGlobal::set_float(bvh_process_flag_vertices_wave_lengthVarId, args.flag.fixedWind.waveLength);
    }
    else
    {
      ShaderGlobal::set_float4(bvh_process_flag_vertices_flagpole_pos_0VarId, args.flag.globalWind.flagpolePos0);
      ShaderGlobal::set_float4(bvh_process_flag_vertices_flagpole_pos_1VarId, args.flag.globalWind.flagpolePos1);
      ShaderGlobal::set_float(bvh_process_flag_vertices_stiffnessVarId, args.flag.globalWind.stiffness);
      ShaderGlobal::set_float(bvh_process_flag_vertices_flag_movement_scaleVarId, args.flag.globalWind.flagMovementScale);
      ShaderGlobal::set_float(bvh_process_flag_vertices_bendVarId, args.flag.globalWind.bend);
      ShaderGlobal::set_float(bvh_process_flag_vertices_deviationVarId, args.flag.globalWind.deviation);
      ShaderGlobal::set_float(bvh_process_flag_vertices_stretchVarId, args.flag.globalWind.stretch);
      ShaderGlobal::set_float(bvh_process_flag_vertices_flag_lengthVarId, args.flag.globalWind.flagLength);
      ShaderGlobal::set_float(bvh_process_flag_vertices_sway_speedVarId, args.flag.globalWind.swaySpeed);
      ShaderGlobal::set_int(bvh_process_flag_vertices_width_typeVarId, args.flag.globalWind.widthType);
    }

    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;
  args.positionOffset = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  if (args.colorOffset != -1)
  {
    args.colorOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

bool DeformedVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
#define GLOBAL_VARS_LIST                              \
  VAR(bvh_process_deformed_vertices_start)            \
  VAR(bvh_process_deformed_vertices_stride)           \
  VAR(bvh_process_deformed_vertices_count)            \
  VAR(bvh_process_deformed_vertices_processed_stride) \
  VAR(bvh_process_deformed_vertices_position_offset)  \
  VAR(bvh_process_deformed_vertices_normal_offset)    \
  VAR(bvh_process_deformed_vertices_color_offset)     \
  VAR(bvh_process_deformed_vertices_texcoord_offset)  \
  VAR(bvh_process_deformed_vertices_texcoord_size)    \
  VAR(bvh_process_deformed_vertices_pos_mul)          \
  VAR(bvh_process_deformed_vertices_pos_ofs)          \
  VAR(bvh_process_deformed_vertices_wtm)              \
  VAR(bvh_process_deformed_vertices_itm)              \
  VAR(bvh_process_deformed_vertices_params)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(FlagsVertexProcessor);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.colorOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  G_ASSERT(args.getDeformParamsFn);

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_deformedVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_deformed_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_deformed_vertices_output_uav_no");

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    float deformId; // uint32_t bitwise
    Point2 simParams;
    args.getDeformParamsFn(deformId, simParams);
    Point4 deformaParams = {deformId, simParams.x, simParams.y, 0};

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_processed_strideVarId, vertexSize);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_position_offsetVarId, args.positionOffset);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_normal_offsetVarId, args.normalOffset);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_color_offsetVarId, args.colorOffset);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_texcoord_offsetVarId, args.texcoordOffset);
    ShaderGlobal::set_int(bvh_process_deformed_vertices_texcoord_sizeVarId, tcSize);
    ShaderGlobal::set_float4(bvh_process_deformed_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_float4(bvh_process_deformed_vertices_pos_ofsVarId, args.posAdd);
    ShaderGlobal::set_float4x4(bvh_process_deformed_vertices_wtmVarId, worldTm.transpose());
    ShaderGlobal::set_float4x4(bvh_process_deformed_vertices_itmVarId, args.invWorldTm);
    ShaderGlobal::set_int4(bvh_process_deformed_vertices_paramsVarId, *((IPoint4 *)&deformaParams)); // hmap id is coded as uint, which
                                                                                                     // can cause false positive nan
                                                                                                     // alerts

    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  if (args.colorOffset != -1)
  {
    args.colorOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

void AHSProcessor::process(ContextId context_id, const BVHGeometryBufferWithOffset &geometry,
  UniqueBVHBufferWithOffset &processed_buffer, uint32_t &bindless_id, int index_format, int index_count, int texcoord_offset,
  int texcoord_format, int vertex_stride, int color_offset)
{
#define GLOBAL_VARS_LIST                        \
  VAR(bvh_process_ahs_vertices_index_count)     \
  VAR(bvh_process_ahs_vertices_texcoord_offset) \
  VAR(bvh_process_ahs_vertices_texcoord_format) \
  VAR(bvh_process_ahs_vertices_color_offset)    \
  VAR(bvh_process_ahs_vertices_vertex_stride)   \
  VAR(bvh_process_ahs_vertices_ib_offset)       \
  VAR(bvh_process_ahs_vertices_vb_offset)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  static int indices_const_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_indices_const_no");
  static int vertices_const_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_vertices_const_no");
  static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_output_uav_no");

  BVH_PROFILE(AHSProcessor);

  G_ASSERT(geometry);
  G_ASSERT(index_format == 2);

  G_UNUSED(index_format);

  int output_index_count = color_offset < 0 ? index_count : index_count * 2;

  static constexpr int maxVertexCount = 64 * 1024;
  G_ASSERT(maxVertexCount >= output_index_count);

  if (!output)
  {
    // A vertex is 2 halfs, 4 bytes, so one dword.
    output.reset(d3d::buffers::create_ua_byte_address(maxVertexCount, "bvh_AHSProcessor_output", RESTAG_BVH));
    HANDLE_LOST_DEVICE_STATE(output, );
  }

  d3d::set_buffer(STAGE_CS, indices_const_no, geometry.getIndexBuffer(context_id));
  d3d::set_buffer(STAGE_CS, vertices_const_no, geometry.getVertexBuffer(context_id));
  d3d::set_rwbuffer(STAGE_CS, output_uav_no, output.get());

  set_offset(processed_buffer);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_index_countVarId, index_count);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_texcoord_offsetVarId, texcoord_offset);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_texcoord_formatVarId, texcoord_format);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_color_offsetVarId, color_offset);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_vertex_strideVarId, vertex_stride);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_ib_offsetVarId, geometry.ibOffset);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_vb_offsetVarId, geometry.vbOffset);

  G_ASSERT(shader);
  shader->dispatchThreads(index_count, 1, 1);

  processed_buffer.buffer.reset(
    d3d::buffers::create_persistent_sr_byte_address(output_index_count, "bvh_ahs_vertices", d3d::buffers::Init::No, RESTAG_BVH));
  output->copyTo(processed_buffer.get(), 0, 0, output_index_count * 4);

  context_id->holdBuffer(processed_buffer.buffer.get(), bindless_id);

  d3d::resource_barrier(ResourceBarrierDesc(processed_buffer.get(), RB_RO_SRV | RB_STAGE_ALL_SHADERS));
}

bool SplineGenVertexProcessor::process(ContextId context_id, Sbuffer *source, int source_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processed_buffer, uint32_t &bindless_id, ProcessArgs &args, bool skip_processing) const
{
  G_UNUSED(source_buffer_bindless);
  G_UNUSED(source_offset);
  G_UNUSED(source);
#define GLOBAL_VARS_LIST                     \
  VAR(bvh_process_splinegen_vertices_start)  \
  VAR(bvh_process_splinegen_vertices_stride) \
  VAR(bvh_process_splinegen_vertices_count)  \
  VAR(bvh_process_splinegen_vertices_processed_stride)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  BVH_PROFILE(SplineGenVertexProcessor);

  unsigned tcSize;
  channel_size(args.texcoordFormat == bvhAttributeShort2TC ? VSDT_SHORT2 : args.texcoordFormat, tcSize);

  auto vertexSize = sizeof(Point3);
  if (args.normalOffset != -1)
    vertexSize += sizeof(uint32_t);
  if (args.texcoordOffset != -1)
    vertexSize += tcSize;

  G_ASSERT(args.getSplineDataFn);

  if (processed_buffer.needAllocation())
  {
    String name(32, "bvh_splinegenVertexProcessor_%u", ++counter);
    if (!allocate(context_id, processed_buffer, bindless_id, vertexSize * args.vertexCount, name))
      return false;

    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_splinegen_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_splinegen_vertices_output_uav_no");

    uint32_t startVertex = 0;
    auto vertexBuffer = args.getSplineDataFn(startVertex);

    set_source_offset(source_offset);
    d3d::set_buffer(STAGE_CS, source_const_no, vertexBuffer);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_splinegen_vertices_startVarId, startVertex);
    ShaderGlobal::set_int(bvh_process_splinegen_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_splinegen_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_int(bvh_process_splinegen_vertices_processed_strideVarId, vertexSize);

    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
  }

  args.positionFormat = VSDT_FLOAT3;
  args.vertexStride = vertexSize;
  args.startVertex = 0;

  int offset = sizeof(Point3);
  if (args.texcoordOffset != -1)
  {
    args.texcoordOffset = offset;
    offset += tcSize;
  }
  if (args.normalOffset != -1)
  {
    args.normalOffset = offset;
    offset += sizeof(uint32_t);
  }
  return !skip_processing;
}

} // namespace bvh