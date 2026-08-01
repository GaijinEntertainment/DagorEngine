//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <drv/3d/dag_sampler.h>
#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>
#include <shaders/dag_computeShaders.h>

#include <cstdint>

class BaseTexture;
class Sbuffer;

namespace render::omm
{

inline constexpr uint32_t MAX_TRANSIENT_POOL_BUFFERS = 8;
inline constexpr uint32_t MAX_PIPELINES = 32;
inline constexpr uint32_t MAX_STATIC_SAMPLERS = 8;
inline constexpr uint32_t MAX_PENDING_BAKES = 8;

enum class AlphaMode : uint32_t
{
  Test,
  Blend
};

enum class TexCoordFormat : uint32_t
{
  UV16_UNORM,
  UV16_FLOAT,
  UV32_FLOAT
};

enum class IndexFormat : uint32_t
{
  UINT16,
  UINT32,
  UINT8
};

enum class OpacityState : uint32_t
{
  Transparent,
  Opaque,
  UnknownTransparent,
  UnknownOpaque
};

enum class Format : uint32_t
{
  OC1_2_State = 1,
  OC1_4_State = 2
};

enum BakeFlags : uint32_t
{
  PERFORM_SETUP = 1u << 0,
  PERFORM_BAKE = 1u << 1,
  PERFORM_SETUP_AND_BAKE = PERFORM_SETUP | PERFORM_BAKE,
  COMPUTE_ONLY = 1u << 2,
  ENABLE_POST_DISPATCH_INFO_STATS = 1u << 3,
  DISABLE_SPECIAL_INDICES = 1u << 4,
  DISABLE_TEXCOORD_DEDUPLICATION = 1u << 5,
  FORCE_32BIT_INDICES = 1u << 6,
  DISABLE_LEVEL_LINE_INTERSECTION = 1u << 7,
  ENABLE_NSIGHT_DEBUG_MODE = 1u << 8
};

struct SamplerDesc
{
  d3d::AddressMode addressingMode = d3d::AddressMode::Wrap;
  d3d::FilterMode filter = d3d::FilterMode::Linear;
  float borderAlpha = 0.f;
};

struct BakeInput
{
  BaseTexture *alphaTexture = nullptr;
  Sbuffer *texCoordBuffer = nullptr;
  Sbuffer *indexBuffer = nullptr;
  Sbuffer *subdivisionLevelBuffer = nullptr;

  uint32_t bakeFlags = PERFORM_SETUP_AND_BAKE | COMPUTE_ONLY;
  SamplerDesc runtimeSamplerDesc;
  AlphaMode alphaMode = AlphaMode::Test;
  uint32_t alphaTextureChannel = 0xFFFFFFFFu;
  TexCoordFormat texCoordFormat = TexCoordFormat::UV32_FLOAT;
  uint32_t texCoordOffsetInBytes = 0;
  uint32_t texCoordStrideInBytes = 0;
  IndexFormat indexFormat = IndexFormat::UINT32;
  uint32_t indexCount = 0;
  uint32_t indexStrideInBytes = 0;
  uint32_t indexBufferOffsetInBytes = 0;
  float alphaCutoff = 0.5f;
  OpacityState alphaCutoffLessEqual = OpacityState::Transparent;
  OpacityState alphaCutoffGreater = OpacityState::Opaque;
  float dynamicSubdivisionScale = 2.f;
  Format globalFormat = Format::OC1_4_State;
  uint8_t maxSubdivisionLevel = 8;
  bool enableSubdivisionLevelBuffer = false;
  uint32_t maxOutOmmArraySize = 0xFFFFFFFFu;
  uint32_t maxScratchMemorySize = 256u << 20u;
};

struct BakeHandle
{
  uint32_t slot = 0xFFFFFFFFu;
  uint32_t generation = 0;
};

struct BakeResult
{
  UniqueBuf arrayData;
  UniqueBuf descArray;
  UniqueBuf indexBuffer;

  dag::Vector<raytrace::OpacityMicroMapDescription> arrayBuildDescs;
  dag::Vector<raytrace::OpacityMicroMapDescription> blasLinkageDescs;

  IndexFormat indexFormat = IndexFormat::UINT32;
  uint32_t indexCount = 0;

  uint32_t arrayDataSizeInBytes = 0;
  uint32_t descArraySizeInBytes = 0;
  uint32_t indexBufferSizeInBytes = 0;
};

struct BakeStats
{
  uint32_t totalOpaqueCount = 0;
  uint32_t totalTransparentCount = 0;
  uint32_t totalUnknownCount = 0;
  uint32_t totalFullyOpaqueCount = 0;
  uint32_t totalFullyTransparentCount = 0;
  uint32_t totalFullyUnknownCount = 0;
};

struct DebugBakeResultInfo
{
  const char *label = nullptr;
  uint64_t objectId = 0;
  uint32_t geometryIndex = 0xFFFFFFFFu;
  uint32_t slotId = 0;
  uint32_t materialType = 0;
  bool impostor = false;
  bool secondary = false;
};

enum class PendingBakeState : uint32_t
{
  Free,
  Dispatched,
  Ready
};

// Distinguishes "not ready yet" from "ready but the readback data was invalid",
// since both used to collapse to a single bool and left failed bakes stuck forever.
enum class ConsumeBakeResult : uint32_t
{
  NotReady,
  Failed,
  Ready
};

struct PendingBake
{
  PendingBakeState state = PendingBakeState::Free;
  uint32_t generation = 0;

  IndexFormat outOmmIndexBufferFormat = IndexFormat::UINT32;
  uint32_t outOmmIndexCount = 0;
  uint32_t outOmmArraySizeInBytes = 0;
  uint32_t outOmmDescSizeInBytes = 0;
  uint32_t outOmmIndexBufferSizeInBytes = 0;
  uint32_t outOmmArrayHistogramSizeInBytes = 0;
  uint32_t outOmmIndexHistogramSizeInBytes = 0;
  uint32_t outOmmPostDispatchInfoSizeInBytes = 0;
  uint32_t transientPoolBufferSizeInBytes[MAX_TRANSIENT_POOL_BUFFERS] = {};
  uint32_t numTransientPoolBuffers = 0;

  UniqueBuf outOmmArrayData;
  UniqueBuf outOmmDescArray;
  UniqueBuf outOmmDescArrayHistogram;
  UniqueBuf outOmmIndexBuffer;
  UniqueBuf outOmmIndexHistogram;
  UniqueBuf outPostDispatchInfo;
  UniqueBuf readbackOmmDescArrayHistogram;
  UniqueBuf readbackOmmIndexHistogram;
  UniqueBuf readbackPostDispatchInfo;
  UniqueBuf transientPoolBuffers[MAX_TRANSIENT_POOL_BUFFERS];
  dag::Vector<UniqueBuf> constantBuffers;
  EventQueryHolder readbackQuery;
};

struct Context
{
  void *baker = nullptr;
  void *pipeline = nullptr;
  const void *pipelineInfo = nullptr;
  int lastSdkResult = 0;

  ComputeShader computeShaders[MAX_PIPELINES];
  uint32_t programCount = 0;
  d3d::SamplerHandle staticSamplers[MAX_STATIC_SAMPLERS] = {};
  uint32_t staticSamplerRegisters[MAX_STATIC_SAMPLERS] = {};
  uint32_t staticSamplerCount = 0;

  PendingBake pendingBakes[MAX_PENDING_BAKES];
};

bool init(Context &ctx);
void shutdown(Context &ctx);

bool begin_bake(Context &ctx, const BakeInput &input, BakeHandle &out_handle);
bool has_free_bake_slot(const Context &ctx);
bool is_bake_ready(Context &ctx, BakeHandle handle);
ConsumeBakeResult consume_bake(Context &ctx, BakeHandle handle, BakeResult &out_result, BakeStats *out_stats = nullptr);
bool wait_bake(Context &ctx, BakeHandle handle, BakeResult &out_result, BakeStats *out_stats = nullptr);
void discard_bake(Context &ctx, BakeHandle handle);
void clear_result(BakeResult &result);
void debug_register_bake_result(const BakeResult &result, const DebugBakeResultInfo &info);
void debug_unregister_bake_result(const BakeResult &result);
void debug_shutdown();

raytrace::OpacityMicroMapTriangleArrayBuildInfo make_array_build_info(const BakeResult &result, Sbuffer *scratch,
  uint32_t scratch_offset, uint32_t scratch_size, RaytraceBuildFlags flags = RaytraceBuildFlags::NONE);
RaytraceGeometryDescription::OpacityMicroMapLinkage make_geometry_linkage(const BakeResult &result,
  RaytraceOpacityMicroMapTriangleArray *triangle_array);

} // namespace render::omm
