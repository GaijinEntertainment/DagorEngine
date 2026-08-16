//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <math/dag_Point4.h>
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
  Float2,          // two float32
  Half2,           // two float16
  UShort2Norm,     // two uint16, divided by 65535
  Short2Fixed4096, // two sign-extended int16, divided by 4096
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

// A band in UV space outside which the bake reads the alpha as zero. At a given v the band is
// x in [lines.x * v + lines.z, lines.y * v + lines.w].
struct UvCutout
{
  Point4 lines = Point4(0, 0, 0, 1);
  bool enabled = false;
};

struct BakeInput
{
  BaseTexture *alphaTexture = nullptr;
  Sbuffer *texCoordBuffer = nullptr;
  Sbuffer *indexBuffer = nullptr;
  Sbuffer *subdivisionLevelBuffer = nullptr;

  uint32_t bakeFlags = PERFORM_SETUP_AND_BAKE | COMPUTE_ONLY;
  SamplerDesc runtimeSamplerDesc;
  UvCutout uvCutout;
  AlphaMode alphaMode = AlphaMode::Test;
  uint32_t alphaTextureChannel = 0xFFFFFFFFu;
  TexCoordFormat texCoordFormat = TexCoordFormat::Float2;
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

// A copy of the BakeInput fields the viewer needs. Taken at the start of the bake, because the viewer
// uses them some frames later, when the source is gone.
struct DebugBakeSource
{
  TEXTUREID alphaTextureId = BAD_TEXTUREID;
  uint32_t alphaTextureChannel = 3;
  float alphaCutoff = 0.5f;
  // The overlay must sample as the bake did: an impostor bakes through a transparent border, which the
  // texture's own sampler does not have.
  SamplerDesc runtimeSamplerDesc;
  // To calculate the subdivision level again: the SDK stores no level for a triangle with one state, and
  // a failed bake has only such triangles.
  float dynamicSubdivisionScale = 2.f;
  uint32_t maxSubdivisionLevel = 8;
  // The viewer masks the alpha it puts over the states in the same manner as the bake; without this the
  // alpha looks different from the states.
  UvCutout uvCutout;
  // Read one time, at the registration; the viewer keeps its own decoded copy.
  Sbuffer *texCoordBuffer = nullptr;
  uint32_t texCoordOffsetInBytes = 0;
  uint32_t texCoordStrideInBytes = 0;
  TexCoordFormat texCoordFormat = TexCoordFormat::Float2;
  Sbuffer *indexBuffer = nullptr;
  uint32_t indexBufferOffsetInBytes = 0;
  uint32_t indexStrideInBytes = 0;
  IndexFormat indexFormat = IndexFormat::UINT32;
};

// Separate id because BakeInput holds a BaseTexture*, which the viewer cannot bind or hold.
DebugBakeSource make_debug_bake_source(const BakeInput &input, TEXTUREID alpha_texture_id);

// A producer with no mesh identity, grass for one, leaves geometryIndex at this value; its label names
// the source instead. The viewer then shows no identity line.
inline constexpr uint32_t NO_GEOMETRY_INDEX = 0xFFFFFFFFu;

struct DebugBakeResultInfo
{
  const char *label = nullptr;
  uint64_t objectId = 0;
  uint32_t geometryIndex = NO_GEOMETRY_INDEX;
  uint32_t slotId = 0;
  uint32_t materialType = 0;
  bool impostor = false;
  bool secondary = false;
  // Not null marks a failed bake: the viewer shows the entry first, in red, with this text.
  const char *failReason = nullptr;
  DebugBakeSource source;
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
// Registers a result whose owner is still the producer. clear_result() removes the registration.
void debug_register_bake_result(const BakeResult &result, const DebugBakeResultInfo &info);
// Moves the buffers to the viewer, thus the entry outlives the producer. Only for a bake that made no
// usable OMM: no other code needs those buffers. Nothing unregisters an adopted entry, thus the viewer
// keeps a bounded number of them and drops an earlier one to take a new one.
void debug_adopt_bake_result(BakeResult &&result, const DebugBakeResultInfo &info);
void debug_unregister_bake_result(const BakeResult &result);
void debug_shutdown();

raytrace::OpacityMicroMapTriangleArrayBuildInfo make_array_build_info(const BakeResult &result, Sbuffer *scratch,
  uint32_t scratch_offset, uint32_t scratch_size, RaytraceBuildFlags flags = RaytraceBuildFlags::NONE);
RaytraceGeometryDescription::OpacityMicroMapLinkage make_geometry_linkage(const BakeResult &result,
  RaytraceOpacityMicroMapTriangleArray *triangle_array);

} // namespace render::omm
