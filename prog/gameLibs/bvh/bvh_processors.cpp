// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include "bvh_context.h"

#include <drv/3d/dag_shaderConstants.h>
#include <shaders/dag_shaderBlock.h>
#include <math/dag_hlsl_floatx.h>
#include "shaders/bvh_process_tree_vertices.hlsli"

#if BVH_PROFILING_ENABLED
#define BVH_PROFILE TIME_PROFILE
#else
#define BVH_PROFILE(...)
#endif

namespace bvh
{

static bool allocate(ContextId context_id, UniqueOrReferencedBVHBuffer &buffer, uint32_t size, const String &name)
{
  if (buffer.unique)
  {
    buffer.unique->reset(d3d::buffers::create_ua_sr_byte_address(size / sizeof(uint32_t), name.data()));
    HANDLE_LOST_DEVICE_STATE(buffer.unique, false);
  }
  else if (buffer.referenced)
  {
    ProcessBufferAllocator *allocator = nullptr;
    if (auto iter = context_id->processBufferAllocators.find(size); iter == context_id->processBufferAllocators.end())
      allocator =
        &context_id->processBufferAllocators.insert({size, ProcessBufferAllocator(size, divide_up(1024 * 1024, size))}).first->second;
    else
      allocator = &iter->second;

    *buffer.referenced = allocator->allocate();
    HANDLE_LOST_DEVICE_STATE(*buffer.referenced, false);
    buffer.referenced->allocator = allocator->bufferSize;
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

void IndexProcessor::process(Sbuffer *source, UniqueBVHBufferWithOffset &processed_buffer, int index_format, int index_count,
  int index_start, int start_vertex)
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
      buffer.reset(d3d::buffers::create_ua_byte_address((64 * 1024 * 2) / 4, name.data()));
      HANDLE_LOST_DEVICE_STATE(buffer, );
    }

  bool deleteIPO = false;

  Sbuffer *indexProcessorOutput = outputs[ringCursor].get();
  if (indexProcessorOutput->getSize() >= dwordCount * 4)
    ringCursor = (ringCursor + 1) % countof(outputs);
  else
  {
    String name(32, "bvh_IndexProcessor");
    indexProcessorOutput = d3d::buffers::create_ua_byte_address(dwordCount, name.data());
    HANDLE_LOST_DEVICE_STATE(indexProcessorOutput, );
    deleteIPO = true;
  }

  d3d::set_buffer(STAGE_CS, source_const_no, source);
  d3d::set_rwbuffer(STAGE_CS, output_uav_no, indexProcessorOutput);

  set_offset(processed_buffer);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_startVarId, index_start);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_start_alignedVarId, ((index_start * 2) & 3) ? 0 : 1);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_countVarId, dwordCount);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_vertex_baseVarId, start_vertex);
  ShaderGlobal::set_int(bvh_process_dynrend_indices_sizeVarId, index_format);
  G_ASSERT(shader);
  shader->dispatchThreads(dwordCount, 1, 1);

  indexProcessorOutput->copyTo(processed_buffer.get(), 0, 0, dwordCount * 4);

  if (deleteIPO)
    del_d3dres(indexProcessorOutput);
}

bool SkinnedVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_skinned_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_skinned_vertices_output_uav_no");

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
    ShaderGlobal::set_color4(bvh_process_skinned_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_color4(bvh_process_skinned_vertices_pos_ofsVarId, args.posAdd);
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

bool TreeVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
#define GLOBAL_VARS_LIST                     \
  VAR(bvh_process_tree_vertices_is_pos_inst) \
  VAR(bvh_process_tree_vertices_is_pivoted)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    G_ASSERT(args.positionFormat == VSDT_FLOAT3);

    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_output_uav_no");
    static int pp_pos_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_pp_pos_no");
    static int pp_dir_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_pp_dir_no");
    static int sampler_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_pp_sampler_no");
    static int params_no = ShaderGlobal::get_slot_by_name("bvh_process_tree_vertices_params_no");

    static d3d::SamplerHandle ppSampler = d3d::request_sampler(d3d::SamplerInfo());

    ShaderGlobal::set_int(bvh_process_tree_vertices_is_pos_instVarId, args.tree.isPosInstance);
    ShaderGlobal::set_int(bvh_process_tree_vertices_is_pivotedVarId, args.tree.isPivoted);

    TMatrix4_vec4 worldTm;
    v_mat43_transpose_to_mat44((mat44f &)worldTm, args.worldTm);
    worldTm._44 = 1; // v_mat43_transpose_to_mat44 sets the last column to 0

    BvhTreeInstanceData params;
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

    auto cbuffer = getNextCbuffer();
    cbuffer->updateDataWithLock(0, cbuffer->getSize(), &params, VBLOCK_DISCARD);

    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());
    d3d::set_tex(STAGE_CS, pp_pos_no, args.tree.ppPosition);
    d3d::set_tex(STAGE_CS, pp_dir_no, args.tree.ppDirection);
    d3d::set_sampler(STAGE_CS, sampler_no, ppSampler);
    d3d::set_const_buffer(STAGE_CS, params_no, cbuffer, 0, 0);

    G_ASSERT(shader);
    shader->dispatchThreads(args.vertexCount, 1, 1);
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

void TreeVertexProcessor::begin() const
{
  static int bvh_process_tree_vertices_blockId = ShaderGlobal::getBlockId("bvh_process_tree_vertices_block");
  ShaderGlobal::setBlock(bvh_process_tree_vertices_blockId, ShaderGlobal::LAYER_SCENE);
  cbufferCursor = 0;
}

void TreeVertexProcessor::end() const { ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE); }

Sbuffer *TreeVertexProcessor::getNextCbuffer() const
{
  if (cbufferCursor < cbuffers.size())
    return cbuffers.data()[cbufferCursor++].get();

  eastl::string name(eastl::string::CtorSprintf(), "btc_%d", cbufferCursor);

  cbuffers.push_back();
  cbuffers.back().reset(d3d::buffers::create_one_frame_cb((sizeof(BvhTreeInstanceData) + 15) / 16, name.c_str()));
  cbufferCursor++;
  return cbuffers.back().get();
}

bool ImpostorVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, outputVertexSize * args.vertexCount * 2, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_impostor_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_impostor_vertices_output_uav_no");

    d3d::set_buffer(STAGE_CS, source_const_no, source);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, processed_buffer.get());

    set_offset(processed_buffer);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_startVarId, args.baseVertex + args.startVertex);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_strideVarId, args.vertexStride);
    ShaderGlobal::set_int(bvh_process_impostor_vertices_countVarId, args.vertexCount);
    ShaderGlobal::set_real(bvh_process_impostor_vertices_height_offsetVarId, args.impostorHeightOffset);
    ShaderGlobal::set_color4(bvh_process_impostor_vertices_scaleVarId, args.impostorScale);
    ShaderGlobal::set_color4(bvh_process_impostor_vertices_sliceTcTm1VarId, args.impostorSliceTm1);
    ShaderGlobal::set_color4(bvh_process_impostor_vertices_sliceTcTm2VarId, args.impostorSliceTm2);
    ShaderGlobal::set_color4(bvh_process_impostor_vertices_sliceClippingLines1VarId, args.impostorSliceClippingLines1);
    ShaderGlobal::set_color4(bvh_process_impostor_vertices_sliceClippingLines2VarId, args.impostorSliceClippingLines2);
    ShaderGlobal::set_color4_array(bvh_process_impostor_vertices_vertex_offsetsVarId, args.impostorOffsets, 4);
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

bool BakeTextureToVerticesProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_bake_texture_to_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_bake_texture_to_vertices_output_uav_no");

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

bool LeavesVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, outputVertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }
  else
    skip_processing = !args.recycled;

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_leaves_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_leaves_vertices_output_uav_no");

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

bool HeliRotorVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_heli_rotor_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_heli_rotor_vertices_output_uav_no");

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
    ShaderGlobal::set_color4(bvh_process_heli_rotor_vertices_paramsVarId, params);
    ShaderGlobal::set_color4(bvh_process_heli_rotor_vertices_sec_paramsVarId, secParams);
    ShaderGlobal::set_color4(bvh_process_heli_rotor_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_color4(bvh_process_heli_rotor_vertices_pos_ofsVarId, args.posAdd);
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

bool FlagVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_flag_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_flag_vertices_output_uav_no");

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
      ShaderGlobal::set_color4(bvh_process_flag_vertices_frequency_amplitudeVarId, args.flag.fixedWind.frequencyAmplitude);
      ShaderGlobal::set_color4(bvh_process_flag_vertices_wind_directionVarId, args.flag.fixedWind.windDirection);
      ShaderGlobal::set_real(bvh_process_flag_vertices_wind_strengthVarId, args.flag.fixedWind.windStrength);
      ShaderGlobal::set_real(bvh_process_flag_vertices_wave_lengthVarId, args.flag.fixedWind.waveLength);
    }
    else
    {
      ShaderGlobal::set_color4(bvh_process_flag_vertices_flagpole_pos_0VarId, args.flag.globalWind.flagpolePos0);
      ShaderGlobal::set_color4(bvh_process_flag_vertices_flagpole_pos_1VarId, args.flag.globalWind.flagpolePos1);
      ShaderGlobal::set_real(bvh_process_flag_vertices_stiffnessVarId, args.flag.globalWind.stiffness);
      ShaderGlobal::set_real(bvh_process_flag_vertices_flag_movement_scaleVarId, args.flag.globalWind.flagMovementScale);
      ShaderGlobal::set_real(bvh_process_flag_vertices_bendVarId, args.flag.globalWind.bend);
      ShaderGlobal::set_real(bvh_process_flag_vertices_deviationVarId, args.flag.globalWind.deviation);
      ShaderGlobal::set_real(bvh_process_flag_vertices_stretchVarId, args.flag.globalWind.stretch);
      ShaderGlobal::set_real(bvh_process_flag_vertices_flag_lengthVarId, args.flag.globalWind.flagLength);
      ShaderGlobal::set_real(bvh_process_flag_vertices_sway_speedVarId, args.flag.globalWind.swaySpeed);
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

bool DeformedVertexProcessor::process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer,
  ProcessArgs &args, bool skip_processing) const
{
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
    if (!allocate(context_id, processed_buffer, vertexSize * args.vertexCount, name))
      return false;
    skip_processing = false;
  }

  if (!skip_processing)
  {
    static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_process_deformed_vertices_source_const_no");
    static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_deformed_vertices_output_uav_no");

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
    ShaderGlobal::set_color4(bvh_process_deformed_vertices_pos_mulVarId, args.posMul);
    ShaderGlobal::set_color4(bvh_process_deformed_vertices_pos_ofsVarId, args.posAdd);
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

void AHSProcessor::process(Sbuffer *indices, Sbuffer *vertices, UniqueBVHBufferWithOffset &processed_buffer, int index_format,
  int index_count, int texcoord_offset, int texcoord_format, int vertex_stride, int color_offset)
{
#define GLOBAL_VARS_LIST                        \
  VAR(bvh_process_ahs_vertices_index_count)     \
  VAR(bvh_process_ahs_vertices_texcoord_offset) \
  VAR(bvh_process_ahs_vertices_texcoord_format) \
  VAR(bvh_process_ahs_vertices_color_offset)    \
  VAR(bvh_process_ahs_vertices_vertex_stride)

#define VAR(a) static int a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  static int indices_const_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_indices_const_no");
  static int vertices_const_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_vertices_const_no");
  static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_process_ahs_vertices_output_uav_no");

  BVH_PROFILE(AHSProcessor);

  G_ASSERT(indices);
  G_ASSERT(index_format == 2);

  G_UNUSED(index_format);

  int output_index_count = color_offset < 0 ? index_count : index_count * 2;

  static constexpr int maxVertexCount = 64 * 1024;
  G_ASSERT(maxVertexCount >= output_index_count);

  if (!output)
  {
    // A vertex is 2 halfs, 4 bytes, so one dword.
    output.reset(d3d::buffers::create_ua_byte_address(maxVertexCount, "bvh_AHSProcessor_output"));
    HANDLE_LOST_DEVICE_STATE(output, );
  }

  d3d::set_buffer(STAGE_CS, indices_const_no, indices);
  d3d::set_buffer(STAGE_CS, vertices_const_no, vertices);
  d3d::set_rwbuffer(STAGE_CS, output_uav_no, output.get());

  set_offset(processed_buffer);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_index_countVarId, index_count);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_texcoord_offsetVarId, texcoord_offset);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_texcoord_formatVarId, texcoord_format);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_color_offsetVarId, color_offset);
  ShaderGlobal::set_int(bvh_process_ahs_vertices_vertex_strideVarId, vertex_stride);

  G_ASSERT(shader);
  shader->dispatchThreads(index_count, 1, 1);

  processed_buffer.buffer.reset(d3d::buffers::create_persistent_sr_byte_address(output_index_count, "bvh_ahs_vertices"));
  output->copyTo(processed_buffer.get(), 0, 0, output_index_count * 4);

  d3d::resource_barrier(ResourceBarrierDesc(processed_buffer.get(), RB_RO_SRV | RB_STAGE_ALL_SHADERS));
}

} // namespace bvh