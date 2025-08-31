/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "defines.h"
#include "bake_gpu_impl.h"
#include "texture_impl.h"

#include <shared/math.h>
#include <shared/bird.h>
#include <shared/cpu_raster.h>
#include <shared/util.h>

#include <array>
#include <algorithm>
#include <optional>

// SHADERS DEFINITIONS =================================================================================

#define OMM_CONSTANTS_START(name) struct name {
#define OMM_CONSTANT(type, name) type name;
#define OMM_CONSTANTS_END(name, registerIndex) };

#include "omm_global_cb.hlsli"
OMM_DECLARE_GLOBAL_CONSTANT_BUFFER

// =================================================================================

namespace omm
{
namespace Gpu
{

// OmmStaticBuffersImpl =================================================================================

static inline constexpr uint32_t ComputeStaticIndexBufferSize(uint32_t maxSubdivisionLevel) {

    uint32_t staticIndexBufferSize = 0;
    for (uint32_t levelIt = 0; levelIt <= maxSubdivisionLevel; ++levelIt)
    {
        const uint32_t numMicroTriangles    = bird::GetNumMicroTriangles(levelIt);
        const uint32_t numIndices           = 3 * numMicroTriangles;
        staticIndexBufferSize               += numIndices * 4ull;
        staticIndexBufferSize               = math::Align(staticIndexBufferSize, 4u);
    }
    return staticIndexBufferSize;
}

template<uint32_t kMaxSubdivisionLevelNum>
static inline constexpr std::array<uint32_t, kMaxSubdivisionLevelNum> ComputeStaticIndexBufferOffsets() {
    std::array<uint32_t, kMaxSubdivisionLevelNum> staticIndexBufferOffsets;
    staticIndexBufferOffsets[0] = 0;
    for (uint32_t levelIt = 1; levelIt < kMaxSubdivisionLevelNum; ++levelIt)
    {
        staticIndexBufferOffsets[levelIt] = ComputeStaticIndexBufferSize(levelIt - 1);
    }
    return staticIndexBufferOffsets;
}

static inline constexpr uint32_t ComputeStaticVertexBufferSize(uint32_t maxSubdivisionLevel) {

    uint32_t staticVertexBufferSize = 0;
    for (uint32_t levelIt = 0; levelIt <= maxSubdivisionLevel; ++levelIt)
    {
        const uint32_t N = 1u << levelIt;
        const uint32_t vertexNum = (N + 1) * (N + 2) / 2;
        staticVertexBufferSize += 4 * vertexNum;
    }
    return staticVertexBufferSize;
}

template<uint32_t kMaxSubdivisionLevelNum>
static inline constexpr std::array<uint32_t, kMaxSubdivisionLevelNum> ComputeStaticVertexBufferOffets() {

    std::array<uint32_t, kMaxSubdivisionLevelNum> staticVertexBufferOffsets;
    staticVertexBufferOffsets[0] = 0;
    for (uint32_t levelIt = 1; levelIt < kMaxSubdivisionLevelNum; ++levelIt)
    {
        staticVertexBufferOffsets[levelIt] = ComputeStaticVertexBufferSize(levelIt - 1);
    }
    return staticVertexBufferOffsets;
}

class OmmStaticBuffersImpl
{
private:
    static void         FillStaticIndexBuffer(uint32_t* outData, uint32_t levelIt);
    static ommResult    GetStaticIndexBufferData(uint8_t* data, size_t* byteSize);
    static void         FillStaticVertexBuffer(uint32_t* outData, uint32_t levelIt);
    static ommResult    GetStaticVertexBufferData(uint8_t* data, size_t* byteSize);
public:
    static constexpr uint32_t                                       kMaxSubdivisionLevel        = 9;
    static constexpr uint32_t                                       kMaxSubdivisionLevelNum     = kMaxSubdivisionLevel + 1;
    static constexpr std::array<uint32_t, kMaxSubdivisionLevelNum>  kStaticIndexBufferOffsets   = ComputeStaticIndexBufferOffsets<kMaxSubdivisionLevelNum>();
    static constexpr uint32_t                                       kStaticIndexBufferSize      = ComputeStaticIndexBufferSize(kMaxSubdivisionLevelNum);
    static constexpr std::array<uint32_t, kMaxSubdivisionLevelNum>  kStaticVertexBufferOffsets  = ComputeStaticVertexBufferOffets<kMaxSubdivisionLevelNum>();
    static constexpr uint32_t                                       kStaticVertexBufferSize     = ComputeStaticVertexBufferSize(kMaxSubdivisionLevelNum);

    static ommResult  GetStaticResourceData(ommGpuResourceType resource, uint8_t* data, size_t* byteSize);
};

void OmmStaticBuffersImpl::FillStaticIndexBuffer(uint32_t* outData, uint32_t levelIt)
{
    // Generate the topology of a tesselated triangle,
    // Primitives follow bird-curve order, 
    // vertices are row-linear
    // Below is vertex and primitive indices illustrated.
    // (Subdiv Level = 2)
    // 0
    // | \
    // | 15\
    // 1-----2
    // | \ 13| \
    // | 14\ | 12\
    // 3-----4-----5
    // | \ 4 | \ 6 | \
    // | 3 \ | 5 \ | 11\
    // 6-----7-----8-----9
    // | \ 1 | \ 7 | \ 9 | \
    // | 0 \ | 2 \ | 8 \ | 10\
    // 10----11----12----13----14

    // Additionally the primitive indices are getting shuffled to follow the bird-curve.

    const uint32_t N = 1u << levelIt;
    for (uint32_t j = 0; j < N; ++j)
    {
        uint32_t I = 2 * j + 1;
        for (uint32_t i = 0; i < I; ++i)
        {
            // Discrete barycentrics
            uint32_t u = i / 2;
            uint32_t v = N - 1 - j;
            uint32_t w = (N - 1 - u - v) - (i % 2);

            const uint32_t ocIndex = bird::dbary2index(u, v, w, levelIt);

            const int2 vCoord = { int(i) / 2,  int(j) };

            auto GetVertIdx = [](int2 idx)->uint32_t {
                return idx.x + (idx.y * (idx.y + 1)) / 2;
            };

            if (i % 2 == 0)
            {
                outData[3 * ocIndex + 0] = GetVertIdx(vCoord);
                outData[3 * ocIndex + 1] = GetVertIdx(vCoord + int2{ 1, 1 });
                outData[3 * ocIndex + 2] = GetVertIdx(vCoord + int2{ 0, 1 });
            }
            else
            {
                outData[3 * ocIndex + 0] = GetVertIdx(vCoord);
                outData[3 * ocIndex + 1] = GetVertIdx(vCoord + int2{ 1, 0 });
                outData[3 * ocIndex + 2] = GetVertIdx(vCoord + int2{ 1, 1 });
            }
        }
    }
}

ommResult OmmStaticBuffersImpl::GetStaticIndexBufferData(uint8_t* outData, size_t* outByteSize)
{
    if (outByteSize == nullptr) {
        return ommResult_INVALID_ARGUMENT;
    }

    if (outData)
    {
        if (*outByteSize < kStaticIndexBufferSize)
            return ommResult_INVALID_ARGUMENT;

        for (uint32_t levelIt = 0; levelIt < kMaxSubdivisionLevelNum; ++levelIt)
        {
            uint32_t* indexData = (uint32_t*)(outData + kStaticIndexBufferOffsets[levelIt]);
            FillStaticIndexBuffer(indexData, levelIt);
        }
    }

    *outByteSize = kStaticIndexBufferSize;
    return ommResult_SUCCESS;
}

void OmmStaticBuffersImpl::FillStaticVertexBuffer(uint32_t* outData, uint32_t levelIt)
{
    const uint32_t N = 1u << levelIt;

    for (uint32_t j = 0; j <= N; ++j)
    {
        for (uint32_t i = 0; i <= j; ++i)
        {
            uint32_t val = (j << 16u) | i;
            *(outData) = val;
            outData++;
        }
    }
}

ommResult OmmStaticBuffersImpl::GetStaticVertexBufferData(uint8_t* outData, size_t* outByteSize)
{
    if (outByteSize == nullptr) {
        return ommResult_INVALID_ARGUMENT;
    }

    if (outData)
    {
        if (*outByteSize < kStaticVertexBufferSize)
            return ommResult_INVALID_ARGUMENT;

        for (uint32_t levelIt = 0; levelIt < kMaxSubdivisionLevelNum; ++levelIt)
        {
            uint32_t* vertexData = (uint32_t*)(outData + kStaticVertexBufferOffsets[levelIt]);
            FillStaticVertexBuffer(vertexData, levelIt);
        }
    }

    *outByteSize = kStaticVertexBufferSize;
    return ommResult_SUCCESS;
}

ommResult OmmStaticBuffersImpl::GetStaticResourceData(ommGpuResourceType resource, uint8_t* outData, size_t* outByteSize)
{
    if (resource == ommGpuResourceType_STATIC_INDEX_BUFFER) {
        return GetStaticIndexBufferData(outData, outByteSize);
    }
    else if (resource == ommGpuResourceType_STATIC_VERTEX_BUFFER) {
        return GetStaticVertexBufferData(outData, outByteSize);
    }
    return ommResult_INVALID_ARGUMENT;
}

ommResult  OmmStaticBuffers::GetStaticResourceData(ommGpuResourceType resource, uint8_t* data, size_t* byteSize)
{
    return OmmStaticBuffersImpl::GetStaticResourceData(resource, data, byteSize);
}

// PipelineImpl =================================================================================

PipelineImpl::~PipelineImpl()
{}

ommResult  PipelineImpl::Validate(const ommGpuPipelineConfigDesc& config)
{
    return ommResult_SUCCESS;
}

ommResult  PipelineImpl::Validate(const ommGpuDispatchConfigDesc& config) const
{
    const uint32_t MaxSubdivLevelAPI        = kMaxSubdivLevel;
    const uint32_t MaxSubdivLevelGfx        = std::min<uint32_t>(MaxSubdivLevelAPI, OmmStaticBuffersImpl::kMaxSubdivisionLevelNum);
    const uint32_t MaxSubdivLevelCS         = 12;
    const bool computeOnly                  = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_ComputeOnly) == (uint32_t)ommGpuBakeFlags_ComputeOnly);
    const bool doSetup                      = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformSetup) == (uint32_t)ommGpuBakeFlags_PerformSetup);
    const bool doBake                       = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformBake) == (uint32_t)ommGpuBakeFlags_PerformBake);
    
    if (config.indexCount == 0)
        return m_log.InvalidArg("[Invalid Arg] - indexCount must be non-zero");
    if (config.indexCount % 3 != 0)
        return m_log.InvalidArg("[Invalid Arg] - indexCount must be multiple of 3");
    if (!computeOnly && config.maxSubdivisionLevel > MaxSubdivLevelGfx)
        return m_log.InvalidArg("[Invalid Arg] - maxSubdivisionLevel must be less than MaxSubdivLevelGfx(10) for non-compute only baking");
    if (computeOnly && config.maxSubdivisionLevel > MaxSubdivLevelCS)
        return m_log.InvalidArg("[Invalid Arg] - maxSubdivisionLevel must be less than MaxSubdivLevelCS(12)");
    if (config.enableSubdivisionLevelBuffer)
        return m_log.NotImplemented("[Invalid Arg] - enableSubdivisionLevelBuffer support is currently not implemented");
    if (config.alphaTextureChannel > 3)
        return m_log.InvalidArg("[Invalid Arg] - alphaTextureChannel must be greater than 3");
    if (!doBake && !doSetup)
        return m_log.InvalidArg("[Invalid Arg] - Either ommGpuBakeFlags_PerformBake or ommGpuBakeFlags_PerformSetup must be set");

    if (!IsCompatible(config.alphaCutoffGreater, config.globalFormat))
        return m_log.InvalidArgf("[Invalid Argument] - alphaCutoffGreater=%s is not compatible with %s", GetOpacityStateAsString(config.alphaCutoffGreater), GetFormatAsString(config.globalFormat));
    if (!IsCompatible(config.alphaCutoffLessEqual, config.globalFormat))
        return m_log.InvalidArgf("[Invalid Argument] - alphaCutoffLessEqual=%s is not compatible with %s", GetOpacityStateAsString(config.alphaCutoffLessEqual), GetFormatAsString(config.globalFormat));

    return ommResult_SUCCESS;
}

ommResult  PipelineImpl::Create(const ommGpuPipelineConfigDesc& config)
{
    RETURN_STATUS_IF_FAILED(ConfigurePipeline(config));

#ifdef OMM_ENABLE_PRECOMPILED_SHADERS_DXIL
#define ByteCodeDXIL(shaderName) g_##shaderName##_dxil, sizeof(g_##shaderName##_dxil)
#else
#define ByteCodeDXIL(shaderName) nullptr, 0
#endif

#ifdef OMM_ENABLE_PRECOMPILED_SHADERS_SPIRV
#define ByteCodeSPIRV(shaderName) g_##shaderName##_spirv, sizeof(g_##shaderName##_spirv)
#else
#define ByteCodeSPIRV(shaderName) nullptr, 0
#endif

#define ByteCodeFromName(shaderName, shaderIdentifier) { shaderName, "main", ByteCodeDXIL(shaderIdentifier), ByteCodeSPIRV(shaderIdentifier) }

    m_pipelineBuilder.SetAPI(config.renderAPI);

    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Wrap, ommTextureFilterMode_Linear} , 0} });
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Mirror, ommTextureFilterMode_Linear} , 1} });
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Clamp, ommTextureFilterMode_Linear} , 2} });
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Border, ommTextureFilterMode_Linear} , 3} });

    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Wrap, ommTextureFilterMode_Nearest} , 4}});
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Mirror, ommTextureFilterMode_Nearest} , 5} });
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Clamp, ommTextureFilterMode_Nearest} , 6} });
    m_pipelineBuilder.AddStaticSamplerDesc({ {ommSamplerDesc{ommTextureAddressMode_Border, ommTextureFilterMode_Nearest} , 7} });

    m_pipelines.ommClearBufferIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_clear_buffer.cs", omm_clear_buffer_cs),
        m_pipelines.ommClearBufferBindings.GetRanges(), m_pipelines.ommClearBufferBindings.GetNumRanges());

    m_pipelines.ommInitBuffersCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_init_buffers_cs.cs", omm_init_buffers_cs_cs),
        m_pipelines.ommInitBuffersCsBindings.GetRanges(), m_pipelines.ommInitBuffersCsBindings.GetNumRanges());

    m_pipelines.ommInitBuffersGfxIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_init_buffers_gfx.cs", omm_init_buffers_gfx_cs),
        m_pipelines.ommInitBuffersGfxBindings.GetRanges(), m_pipelines.ommInitBuffersGfxBindings.GetNumRanges());

    m_pipelines.ommWorkSetupBakeOnlyCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_work_setup_bake_only_cs.cs", omm_work_setup_bake_only_cs_cs),
        m_pipelines.ommWorkSetupBakeOnlyCsBindings.GetRanges(), m_pipelines.ommWorkSetupBakeOnlyCsBindings.GetNumRanges());

    m_pipelines.ommWorkSetupCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_work_setup_cs.cs", omm_work_setup_cs_cs),
         m_pipelines.ommWorkSetupCsBindings.GetRanges(), m_pipelines.ommWorkSetupCsBindings.GetNumRanges());

    m_pipelines.ommWorkSetupGfxIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_work_setup_gfx.cs", omm_work_setup_gfx_cs),
        m_pipelines.ommWorkSetupGfxBindings.GetRanges(), m_pipelines.ommWorkSetupGfxBindings.GetNumRanges());

    m_pipelines.ommWorkSetupBakeOnlyGfxIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_work_setup_bake_only_gfx.cs", omm_work_setup_bake_only_gfx_cs),
        m_pipelines.ommWorkSetupBakeOnlyGfxBindings.GetRanges(), m_pipelines.ommWorkSetupBakeOnlyGfxBindings.GetNumRanges());

    m_pipelines.ommPostBuildInfoBuffersIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_post_build_info.cs", omm_post_build_info_cs),
        m_pipelines.ommPostBuildInfoBindings.GetRanges(), m_pipelines.ommPostBuildInfoBindings.GetNumRanges());

    m_pipelines.ommRasterizeRIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize.vs", omm_rasterize_vs),
        ByteCodeFromName("omm_rasterize.gs", omm_rasterize_gs),
        ByteCodeFromName("omm_rasterize_ps_r.ps", omm_rasterize_ps_r_ps),
        true /*ConservativeRasterization*/,
        0 /*NumRenderTargets*/,
        m_pipelines.ommRasterizeBindings.GetRanges(), m_pipelines.ommRasterizeBindings.GetNumRanges());

    m_pipelines.ommRasterizeGIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize.vs", omm_rasterize_vs),
        ByteCodeFromName("omm_rasterize.gs", omm_rasterize_gs),
        ByteCodeFromName("omm_rasterize_ps_g.ps", omm_rasterize_ps_g_ps),
        true /*ConservativeRasterization*/,
        0 /*NumRenderTargets*/,
        m_pipelines.ommRasterizeBindings.GetRanges(), m_pipelines.ommRasterizeBindings.GetNumRanges());

    m_pipelines.ommRasterizeBIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize.vs", omm_rasterize_vs),
        ByteCodeFromName("omm_rasterize.gs", omm_rasterize_gs),
        ByteCodeFromName("omm_rasterize_ps_b.ps", omm_rasterize_ps_b_ps),
        true /*ConservativeRasterization*/,
        0 /*NumRenderTargets*/,
        m_pipelines.ommRasterizeBindings.GetRanges(), m_pipelines.ommRasterizeBindings.GetNumRanges());

    m_pipelines.ommRasterizeAIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize.vs", omm_rasterize_vs),
        ByteCodeFromName("omm_rasterize.gs", omm_rasterize_gs),
        ByteCodeFromName("omm_rasterize_ps_a.ps", omm_rasterize_ps_a_ps),
        true /*ConservativeRasterization*/,
        0 /*NumRenderTargets*/,
        m_pipelines.ommRasterizeBindings.GetRanges(), m_pipelines.ommRasterizeBindings.GetNumRanges());

    m_pipelines.ommRasterizeRCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_rasterize_cs_r.cs", omm_rasterize_cs_r_cs),
        m_pipelines.ommRasterizeCsBindings.GetRanges(), m_pipelines.ommRasterizeCsBindings.GetNumRanges());

    m_pipelines.ommRasterizeGCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_rasterize_cs_g.cs", omm_rasterize_cs_g_cs),
        m_pipelines.ommRasterizeCsBindings.GetRanges(), m_pipelines.ommRasterizeCsBindings.GetNumRanges());

    m_pipelines.ommRasterizeBCsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_rasterize_cs_b.cs", omm_rasterize_cs_b_cs),
        m_pipelines.ommRasterizeCsBindings.GetRanges(), m_pipelines.ommRasterizeCsBindings.GetNumRanges());

    m_pipelines.ommRasterizeACsIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_rasterize_cs_a.cs", omm_rasterize_cs_a_cs),
        m_pipelines.ommRasterizeCsBindings.GetRanges(), m_pipelines.ommRasterizeCsBindings.GetNumRanges());

    m_pipelines.ommCompressIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_compress.cs", omm_compress_cs),
        m_pipelines.ommCompressBindings.GetRanges(), m_pipelines.ommCompressBindings.GetNumRanges());

    m_pipelines.ommDescPatchIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_desc_patch.cs", omm_desc_patch_cs),
        m_pipelines.ommDescPatchBindings.GetRanges(), m_pipelines.ommDescPatchBindings.GetNumRanges());

    m_pipelines.ommIndexWriteIdx = m_pipelineBuilder.AddComputePipeline(
        ByteCodeFromName("omm_index_write.cs", omm_index_write_cs),
        m_pipelines.ommIndexWriteBindings.GetRanges(), m_pipelines.ommIndexWriteBindings.GetNumRanges());

    m_pipelines.ommRenderTargetClearDebugIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize_debug.vs", omm_rasterize_debug_vs),
        PipelineBuilder::ByteCode(),
        ByteCodeFromName("omm_render_target_clear.ps", omm_render_target_clear_ps),
        false /*ConservativeRasterization*/,
        1 /*NumRenderTargets*/,
        m_pipelines.ommRenderTargetClearDebugBindings.GetRanges(), m_pipelines.ommRenderTargetClearDebugBindings.GetNumRanges());

    m_pipelines.ommRasterizeDebugIdx = m_pipelineBuilder.AddGraphicsPipeline(
        ByteCodeFromName("omm_rasterize_debug.vs", omm_rasterize_debug_vs),
        PipelineBuilder::ByteCode(),
        ByteCodeFromName("omm_rasterize_debug.ps", omm_rasterize_debug_ps),
        false /*ConservativeRasterization*/,
        1 /*NumRenderTargets*/,
        m_pipelines.ommRasterizeDebugBindings.GetRanges(), m_pipelines.ommRasterizeDebugBindings.GetNumRanges());

    m_pipelineBuilder.Finalize();
    return ommResult_SUCCESS;
}

ommResult PipelineImpl::GetPipelineDesc(const ommGpuPipelineInfoDesc** outPipelineDesc)
{
    if (outPipelineDesc == nullptr)
        return m_log.InvalidArg("[Invalid Arg] - PipelineDesc is null");

    *outPipelineDesc = &m_pipelineBuilder._desc;
    return ommResult_SUCCESS;
}

ommResult PipelineImpl::GetPreDispatchInfo(const ommGpuDispatchConfigDesc& config, PreDispatchInfo& outInfo) const
{
    const bool doSetup = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformSetup) == (uint32_t)ommGpuBakeFlags_PerformSetup);
    const bool doBake = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformBake) == (uint32_t)ommGpuBakeFlags_PerformBake);
    const bool enablePostDispatchInfoStats = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_EnablePostDispatchInfoStats) == (uint32_t)ommGpuBakeFlags_EnablePostDispatchInfoStats);
    const bool computeOnly = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_ComputeOnly) == (uint32_t)ommGpuBakeFlags_ComputeOnly);
    const bool enableTexCoordDedup = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_DisableTexCoordDeduplication) == 0;
    const bool enableSpecialIndices = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_DisableSpecialIndices) != (uint32_t)ommGpuBakeFlags_DisableSpecialIndices;

    const uint32_t primitiveCount = config.indexCount / 3;
    const uint32_t defaultAlignment = 128;

    outInfo.numTransientPoolBuffers = 0;
    outInfo.scratchBuffer                   = { "scratchBuffer",                ommGpuResourceType_TRANSIENT_POOL_BUFFER, 0xFFFFFFFF, BufferHeapAlloc() };
    outInfo.scratchBuffer0                  = { "scratchBuffer0",               ommGpuResourceType_TRANSIENT_POOL_BUFFER, 0xFFFFFFFF, BufferHeapAlloc() };
    outInfo.indArgBuffer                    = { "indArgBuffer",                 ommGpuResourceType_TRANSIENT_POOL_BUFFER, 0xFFFFFFFF, BufferHeapAlloc() };
    outInfo.debugBuffer                     = { "debugBuffer",                  ommGpuResourceType_TRANSIENT_POOL_BUFFER, 0xFFFFFFFF, BufferHeapAlloc() };

    // Todo: rename this to "workItemsBuffer"
    {
        const size_t rasterItemSize = (size_t)primitiveCount * ((size_t)config.maxSubdivisionLevel + 1) * sizeof(uint32_t) * 2;
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(rasterItemSize, defaultAlignment, outInfo.rasterItemsBuffer));
    }

    // Allocate memory - Memory must be allocated first, as the available scratch memory impact the
    // worst case number of batches required to complete the workload.
    if (enableTexCoordDedup && doSetup)
    {
        // Hash table consists of one entry per primitive. Each entry contains stores a hash and primitive index.
        const uint32_t kHashTableLoadFactor = 16; // Why load factor of 16? Not sure.

        const size_t hashTableSize = (size_t)primitiveCount * kHashTableLoadFactor * kHashTableEntrySize;
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(hashTableSize, defaultAlignment, outInfo.hashTableBuffer));
    }
    else
    {
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(defaultAlignment, defaultAlignment, outInfo.hashTableBuffer));
    }

    {
        // When writing out to the final index buffer we work on index pairs (in 16 bit mode), so we must always have at least primitiveCount as multiple of two.
        const size_t tempOmmIndexBufferSize = (size_t)math::Align<size_t>(primitiveCount, 2u) * sizeof(uint32_t);
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(tempOmmIndexBufferSize, defaultAlignment, outInfo.tempOmmIndexBuffer));
    }

    if (doBake && !doSetup)
    {
        const size_t tempOmmBakeScheduleTrackerBufferSize = (size_t)math::Align<size_t>(primitiveCount, 2u) * sizeof(uint32_t);
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(tempOmmBakeScheduleTrackerBufferSize, defaultAlignment, outInfo.tempOmmBakeScheduleTrackerBuffer));
    }

    // Allocate space for the special indices buffer.
    if (enableSpecialIndices || enablePostDispatchInfoStats)
    {
        const size_t bakeResultMacroSize = (size_t)primitiveCount * sizeof(uint32_t) * 3;
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer0.Allocate(bakeResultMacroSize, defaultAlignment, outInfo.specialIndicesStateBuffer));
    }
    else
    {
        RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer0.Allocate(defaultAlignment, defaultAlignment, outInfo.specialIndicesStateBuffer));
    }

    {
        const uint32_t assertBufferSize = sizeof(uint32_t) * 1024;
        RETURN_STATUS_IF_FAILED(outInfo.debugBuffer.Allocate(assertBufferSize, defaultAlignment, outInfo.assertBuffer));
    }

    if (computeOnly)
    {
        // A buffer storing various counters
        {
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t) * size_t(config.maxSubdivisionLevel + 1), defaultAlignment, outInfo.bakeResultBufferCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t), defaultAlignment, outInfo.ommArrayAllocatorCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t), defaultAlignment, outInfo.ommDescAllocatorCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t), defaultAlignment, outInfo.IEBakeCsThreadCountBuffer));
        }

        {
            const uint32_t IEBakeCountCs = (config.maxSubdivisionLevel + 1);
            const uint32_t IEBakeCountCsSize = IEBakeCountCs * kIndirectDispatchEntryStride;
            RETURN_STATUS_IF_FAILED(outInfo.indArgBuffer.Allocate(IEBakeCountCsSize, defaultAlignment, outInfo.IEBakeCsBuffer));
        }
    }
    else
    {
        // We need this much memroy to perform the baking in a single pass. 
        // The actual amout may be less than this, in which case we have to split up the baking in multiple iterations.
        // For high subdibision levels and large primitive counts, it can become multiple GB.
        // It's for this reason required to be split up in to multiple smaller batches.
        const size_t maxNumMicroTris = bird::GetNumMicroTriangles(config.maxSubdivisionLevel);

        // The rest is reserved for raster output.
        {
            const size_t reservedMemory = outInfo.scratchBuffer.allocator.GetCurrentReservation() + outInfo.scratchBuffer0.allocator.GetCurrentReservation();

            // How much space is left (if any?)
            if (reservedMemory > (size_t)config.maxScratchMemorySize)
                return ommResult_INSUFFICIENT_SCRATCH_MEMORY;

            // remainingScratchMemory - how much memory is left if we respect the provided buget.
            const size_t remainingScratchMemory = (size_t)config.maxScratchMemorySize - reservedMemory;

            // The minimum amount of memory we require to do the baking process in a single dispatch... 
            // we can't allocate below this value.

            const size_t minScratchMemory = (size_t)maxNumMicroTris * 1 /* primitive count */ * (sizeof(uint32_t) * 2);

            if (remainingScratchMemory < minScratchMemory)
                return ommResult_INSUFFICIENT_SCRATCH_MEMORY;

            // The amount of scratch memory we need to perform the baking in a single step.
            const size_t totalScratchMemory = (size_t)maxNumMicroTris * primitiveCount * (sizeof(uint32_t) * 2);

            const size_t scratchMemorySize = std::min(totalScratchMemory, remainingScratchMemory);

            const size_t bakeResultSize = computeOnly ? defaultAlignment : scratchMemorySize;
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer0.Allocate(scratchMemorySize, defaultAlignment, outInfo.bakeResultBuffer));

            // If the scratch memory is insufficent we split the baking up in to N steps, where N is [1, primitiveCount]
            outInfo.MaxBatchCount = (uint32_t)math::DivUp<size_t>(totalScratchMemory, scratchMemorySize);

            const bool debug = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_EnableNsightDebugMode) == (uint32_t)ommGpuBakeFlags_EnableNsightDebugMode;
            if (debug)
            {
                outInfo.MaxBatchCount = primitiveCount;
            }
            OMM_ASSERT(outInfo.MaxBatchCount > 0);
            OMM_ASSERT(outInfo.MaxBatchCount <= primitiveCount);
        }

        // A buffer storing various counters
        {
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t) * size_t(config.maxSubdivisionLevel + 1), defaultAlignment, outInfo.bakeResultBufferCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t), defaultAlignment, outInfo.ommArrayAllocatorCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t), defaultAlignment, outInfo.ommDescAllocatorCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t) * (config.maxSubdivisionLevel + 1) * outInfo.MaxBatchCount, defaultAlignment, outInfo.dispatchIndirectThreadCounter));
            RETURN_STATUS_IF_FAILED(outInfo.scratchBuffer.Allocate(sizeof(uint32_t) * (config.maxSubdivisionLevel + 1) * outInfo.MaxBatchCount, defaultAlignment, outInfo.IEBakeCsThreadCountBuffer));
        }

        {
            const uint32_t indirectDrawCount = (config.maxSubdivisionLevel + 1) * outInfo.MaxBatchCount;
            const uint32_t indirectDrawSize = indirectDrawCount * kIndirectDispatchEntryStride;
            RETURN_STATUS_IF_FAILED(outInfo.indArgBuffer.Allocate(indirectDrawSize, defaultAlignment, outInfo.IEBakeBuffer));
        }

        {
            const uint32_t indirectPostCsCount = (config.maxSubdivisionLevel + 1) * outInfo.MaxBatchCount;
            const uint32_t indirectPostCsSize = indirectPostCsCount * kIndirectDispatchEntryStride;
            RETURN_STATUS_IF_FAILED(outInfo.indArgBuffer.Allocate(indirectPostCsSize, defaultAlignment, outInfo.IECompressCsBuffer));
        }
    }

    outInfo.numTransientPoolBuffers = 0;
    auto AssignPoolIndex = [&outInfo](BufferResource& resource)
    {
        if (resource.allocator.GetCurrentReservation() != 0)
            resource.indexInPool = outInfo.numTransientPoolBuffers++;
    };
    AssignPoolIndex(outInfo.scratchBuffer);
    AssignPoolIndex(outInfo.scratchBuffer0);
    AssignPoolIndex(outInfo.indArgBuffer);
    AssignPoolIndex(outInfo.debugBuffer);

    return ommResult_SUCCESS;
}

ommResult PipelineImpl::GetPreDispatchInfo(const ommGpuDispatchConfigDesc& config, ommGpuPreDispatchInfo* outPreBuildInfo) const
{
    if (!outPreBuildInfo)
        return m_log.InvalidArg("[Invalid Arg] - preBuildInfo is null");

    const bool force32BitIndices = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_Force32BitIndices) == (uint32_t)ommGpuBakeFlags_Force32BitIndices;
    const bool doBake            = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformBake) == (uint32_t)ommGpuBakeFlags_PerformBake);

    constexpr uint32_t kNumSpecialIndices = 4;

    const uint32_t primitiveCount       = config.indexCount / 3;
    const size_t maxNumMicroTris        = bird::GetNumMicroTriangles(config.maxSubdivisionLevel);
    const size_t bitsPerState           = size_t(config.globalFormat);
    const size_t vmArraySizeInBits      = size_t(primitiveCount) * std::max<size_t>(maxNumMicroTris * bitsPerState, 32u);
    ommIndexFormat outOmmIndexBufferFormat = primitiveCount < std::numeric_limits<int16_t>::max() - kNumSpecialIndices ? ommIndexFormat_UINT_16 : ommIndexFormat_UINT_32;
    if (force32BitIndices)
        outOmmIndexBufferFormat = ommIndexFormat_UINT_32;

    const size_t indexBufferFormatSize  = outOmmIndexBufferFormat == ommIndexFormat_UINT_16 ? 2 : 4;

    const size_t outMaxTheoreticalOmmArraySizeInBytes   = math::Align<size_t>(math::DivUp<size_t>(vmArraySizeInBits, 8u), 4u);

    const size_t outOmmArraySizeInBytes                 = std::min<size_t>(outMaxTheoreticalOmmArraySizeInBytes, config.maxOutOmmArraySize);
    const size_t outOmmDescSizeInBytes                  = primitiveCount * sizeof(uint64_t);
    const size_t outOmmIndexBufferSizeInBytes           = math::Align<size_t>(primitiveCount * indexBufferFormatSize, 4u);
    const size_t outOmmHistogramSizeInBytes             = (size_t(config.maxSubdivisionLevel) + 1) * 2 * sizeof(uint64_t);
    const size_t outOmmPostDispatchInfoSizeInBytes      = sizeof(ommGpuPostDispatchInfo);

    if (outOmmDescSizeInBytes > std::numeric_limits<uint32_t>::max())
        return ommResult_FAILURE;
    if (outOmmIndexBufferSizeInBytes > std::numeric_limits<uint32_t>::max())
        return ommResult_FAILURE;
    if (outOmmHistogramSizeInBytes > std::numeric_limits<uint32_t>::max())
        return ommResult_FAILURE;

    auto SafeCast = [](uint32_t& outVal, size_t val)->ommResult
    {
        if (val > std::numeric_limits<uint32_t>::max())
            return ommResult_FAILURE;
        outVal = (uint32_t)val;
        return ommResult_SUCCESS;
    };

    PreDispatchInfo info;
    RETURN_STATUS_IF_FAILED(GetPreDispatchInfo(config, info));

    outPreBuildInfo->numTransientPoolBuffers = info.numTransientPoolBuffers;

    auto AssignPoolBufferSize = [&outPreBuildInfo](const BufferResource& resource)
    {
        if (resource.indexInPool != 0xFFFFFFFF)
            outPreBuildInfo->transientPoolBufferSizeInBytes[resource.indexInPool] = resource.allocator.GetCurrentReservation();
    };
    AssignPoolBufferSize(info.scratchBuffer);
    AssignPoolBufferSize(info.scratchBuffer0);
    AssignPoolBufferSize(info.indArgBuffer);
    AssignPoolBufferSize(info.debugBuffer);

    outPreBuildInfo->outOmmIndexCount = primitiveCount;
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmArrayHistogramSizeInBytes, outOmmHistogramSizeInBytes));
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmArraySizeInBytes, outOmmArraySizeInBytes));
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmDescSizeInBytes, outOmmDescSizeInBytes));
    outPreBuildInfo->outOmmIndexBufferFormat = outOmmIndexBufferFormat;
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmIndexBufferSizeInBytes, outOmmIndexBufferSizeInBytes));
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmIndexHistogramSizeInBytes, outOmmHistogramSizeInBytes));
    RETURN_STATUS_IF_FAILED(SafeCast(outPreBuildInfo->outOmmPostDispatchInfoSizeInBytes, outOmmPostDispatchInfoSizeInBytes));

    return ommResult_SUCCESS;
}

struct ScopedLabel
{
    template<typename... TArgs>
    ScopedLabel(PassBuilder& builder, TArgs&&... args) : _builder(builder)
    {
        _builder.PushLabel(std::forward<TArgs>(args)...);
    }

    ~ScopedLabel()
    {
        _builder.PopLabel();
    }
private:
    PassBuilder& _builder;
};

#define _SCOPED_LABEL(variableName, ...) ScopedLabel variableName(m_passBuilder, __VA_ARGS__);
#define SCOPED_LABEL(...) _SCOPED_LABEL(scopedLabel_##__LINE__, __VA_ARGS__);

ommResult PipelineImpl::InitGlobalConstants(const ommGpuDispatchConfigDesc& config, const PreDispatchInfo& info, GlobalConstants& cbuffer) const
{
    ommGpuPreDispatchInfo preBuildInfo;
    RETURN_STATUS_IF_FAILED(GetPreDispatchInfo(config, &preBuildInfo));

    const uint32_t primitiveCount = config.indexCount / 3;
    const uint32_t hashTableEntryCount = info.hashTableBuffer.GetSize() / kHashTableEntrySize;

    const bool IsOmmIndexFormat16bit = preBuildInfo.outOmmIndexBufferFormat == ommIndexFormat_UINT_16;
    const bool enableSpecialIndices = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_DisableSpecialIndices) != (uint32_t)ommGpuBakeFlags_DisableSpecialIndices;
    const bool enableTexCoordDeduplication = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_DisableTexCoordDeduplication) != (uint32_t)ommGpuBakeFlags_DisableTexCoordDeduplication);
    const bool computeOnly = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_ComputeOnly) == (uint32_t)ommGpuBakeFlags_ComputeOnly);
    const bool doSetup = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformSetup) == (uint32_t)ommGpuBakeFlags_PerformSetup);
    const bool doBake = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformBake) == (uint32_t)ommGpuBakeFlags_PerformBake);
    const bool enablePostDispatchInfoStats = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_EnablePostDispatchInfoStats) == (uint32_t)ommGpuBakeFlags_EnablePostDispatchInfoStats);
    const bool enableLevelLine = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_DisableLevelLineIntersection) != (uint32_t)ommGpuBakeFlags_DisableLevelLineIntersection);
    
    // The kMaxViewportPeriods dictates how many multiples of the alpha texture we allow to be rasterized,
    // wrap sample modes may extend beyond the texture dimensions (or below zero).
    // To avoid having these primitives clipped we increase the viewport size to accomodate for this type of periodic texture mapping.

    const int32_t kViewportScale    = 5; // Increasing this to 6 fails... TODO: investigate why!

    const float2 viewportSize       = float2((2 * kViewportScale + 1) * config.alphaTextureWidth, (2 * kViewportScale + 1) * config.alphaTextureHeight);
    const float2 viewportOffset     = float2(kViewportScale * config.alphaTextureWidth, kViewportScale * config.alphaTextureHeight);

    cbuffer = {0, };
    cbuffer.IndexCount                                 = config.indexCount;
    cbuffer.PrimitiveCount                             = primitiveCount;
    cbuffer.MaxBatchCount                              = info.MaxBatchCount;
    cbuffer.MaxOutOmmArraySize                         = preBuildInfo.outOmmArraySizeInBytes;
    cbuffer.IsOmmIndexFormat16bit                      = IsOmmIndexFormat16bit;
    cbuffer.DoSetup                                    = doSetup;
    cbuffer.EnablePostDispatchInfoStats                = enablePostDispatchInfoStats && doBake;
    cbuffer.IndirectDispatchEntryStride                = kIndirectDispatchEntryStride;
    cbuffer.SamplerIndex                               = m_pipelineBuilder.GetStaticSamplerIndex(config.runtimeSamplerDesc);
    cbuffer.BakeResultBufferSize                       = info.bakeResultBuffer.GetSize();
    cbuffer.ViewportSize                               = viewportSize;
    cbuffer.ViewportOffset                             = viewportOffset;
    cbuffer.InvViewportSize                            = float2(1.f / cbuffer.ViewportSize.x, 1.f / cbuffer.ViewportSize.y);
    cbuffer.MaxNumSubdivisionLevels                    = config.maxSubdivisionLevel + 1;
    cbuffer.MaxSubdivisionLevel                        = config.maxSubdivisionLevel;
    cbuffer.OMMFormat                                   = (uint32_t)config.globalFormat;
    cbuffer.TexCoordHashTableEntryCount                = hashTableEntryCount;
    cbuffer.DynamicSubdivisionScale                    = config.dynamicSubdivisionScale;
    cbuffer.TexSize                                    = float2(config.alphaTextureWidth, config.alphaTextureHeight);
    static_assert(sizeof(float2) == sizeof(float) * 2);
    cbuffer.InvTexSize                                 = 1.f / cbuffer.TexSize;
    cbuffer.TexCoordFormat                             = (uint32_t)config.texCoordFormat;
    cbuffer.TexCoordOffset                             = config.texCoordOffsetInBytes;
    cbuffer.TexCoordStride                             = config.texCoordStrideInBytes == 0 ? GetTexCoordFormatSize(config.texCoordFormat) : config.texCoordStrideInBytes;
    cbuffer.AlphaCutoff                                = config.alphaCutoff;
    cbuffer.AlphaCutoffGreater                         = (uint32_t)config.alphaCutoffGreater;
    cbuffer.AlphaCutoffLessEqual                       = (uint32_t)config.alphaCutoffLessEqual;
    cbuffer.AlphaTextureChannel                        = config.alphaTextureChannel;
    cbuffer.FilterType                                 = (uint32_t)config.runtimeSamplerDesc.filter;
    cbuffer.EnableLevelLine                            = (uint32_t)enableLevelLine;
    cbuffer.EnableSpecialIndices                       = (uint32_t)enableSpecialIndices;
    cbuffer.EnableTexCoordDeduplication                = (uint32_t)enableTexCoordDeduplication;
    cbuffer.IEBakeBufferOffset                         = info.IEBakeBuffer.GetBufferOffset();
    cbuffer.IEBakeCsBufferOffset                       = info.IEBakeCsBuffer.GetBufferOffset();
    cbuffer.IECompressCsBufferOffset                   = info.IECompressCsBuffer.GetBufferOffset();
    cbuffer.BakeResultBufferCounterBufferOffset        = info.bakeResultBufferCounter.GetBufferOffset();
    cbuffer.OmmArrayAllocatorCounterBufferOffset       = info.ommArrayAllocatorCounter.GetBufferOffset();
    cbuffer.OmmDescAllocatorCounterBufferOffset        = info.ommDescAllocatorCounter.GetBufferOffset();
    cbuffer.DispatchIndirectThreadCountBufferOffset    = info.dispatchIndirectThreadCounter.GetBufferOffset();
    cbuffer.IEBakeCsThreadCountBufferOffset            = info.IEBakeCsThreadCountBuffer.GetBufferOffset();
    cbuffer.RasterItemsBufferOffset                    = info.rasterItemsBuffer.GetBufferOffset();
    cbuffer.HashTableBufferOffset                      = info.hashTableBuffer.GetBufferOffset();
    cbuffer.TempOmmIndexBufferOffset                   = info.tempOmmIndexBuffer.GetBufferOffset();
    cbuffer.TempOmmBakeScheduleTrackerBufferOffset     = info.tempOmmBakeScheduleTrackerBuffer.GetBufferOffset();
    cbuffer.BakeResultBufferOffset                     = info.bakeResultBuffer.GetBufferOffset();
    cbuffer.SpecialIndicesStateBufferOffset            = info.specialIndicesStateBuffer.GetBufferOffset();
    cbuffer.AssertBufferOffset                         = info.assertBuffer.GetBufferOffset();
    static_assert(sizeof(float2) == sizeof(float) * 2);
    static_assert(sizeof(uint2) == sizeof(uint32_t) * 2);
    return ommResult_SUCCESS;
}

#define BEGIN_PASS(name, type, binding) \
    { \
        auto _name = name;\
        PassBuilder::PassConfig p(m_stdAllocator, type, binding, enableValidation);

#define END_PASS() \
        m_passBuilder.FinalizePass(_name, p);\
    }

ommResult PipelineImpl::GetDispatchDesc(const ommGpuDispatchConfigDesc& config, const ommGpuDispatchChain** outDispatchDesc)
{
    if (outDispatchDesc == nullptr)
        return m_log.InvalidArg("[Invalid Arg] - dispatchDesc is null");

    RETURN_STATUS_IF_FAILED(Validate(config));

    m_passBuilder.Reset();

    ommGpuPreDispatchInfo preBuildInfo;
    RETURN_STATUS_IF_FAILED(GetPreDispatchInfo(config, &preBuildInfo));

    PreDispatchInfo info;
    GetPreDispatchInfo(config, info);

    GlobalConstants cbuffer;
    RETURN_STATUS_IF_FAILED(InitGlobalConstants(config, info, cbuffer));
    m_passBuilder.SetGlobalCbuffer((const uint8_t*)&cbuffer, sizeof(cbuffer));

    const uint32_t primitiveCount = config.indexCount / 3;
    const bool computeOnly = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_ComputeOnly) == (uint32_t)ommGpuBakeFlags_ComputeOnly);
    const bool doSetup = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformSetup) == (uint32_t)ommGpuBakeFlags_PerformSetup);
    const bool doBake = (((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_PerformBake) == (uint32_t)ommGpuBakeFlags_PerformBake);
    const bool IsOmmIndexFormat16bit = preBuildInfo.outOmmIndexBufferFormat == ommIndexFormat_UINT_16;

    bool enableValidation = false;

    {
        auto ClearBuffer = [&](const char* name, const BufferResource* resource) {
            SCOPED_LABEL(name);

            BEGIN_PASS(name,  ommGpuDispatchType_Compute, m_pipelines.ommClearBufferBindings);
                p.UseGlobalCbuffer();
                p.BindResource("u_targetBuffer", resource->type, resource->indexInPool);

                const uint32_t byteSize = resource->allocator.GetCurrentReservation();
                OMM_ASSERT(byteSize % 4 == 0);
                const uint32_t numElements = resource->allocator.GetCurrentReservation() / 4;

                CBufferWriter& lCb = p.AddComputeDispatch(m_pipelines.ommClearBufferIdx, math::DivUp<uint32_t>(numElements, 128u), 1);
                lCb.WriteDW(0 /*ClearValue*/);
                lCb.WriteDW(numElements /*NumElements*/);
                lCb.WriteDW(0 /*BufferOffset*/);
                for (uint32_t i = 0; i < 5u; ++i)
                    lCb.WriteDW(0u /*dummy*/);
            END_PASS();
        };

        auto ClearResource = [&](const char* name, const ommGpuResourceType resourceType, uint32_t byteSize) {
            SCOPED_LABEL(name);

            BEGIN_PASS(name,  ommGpuDispatchType_Compute, m_pipelines.ommClearBufferBindings);
            p.UseGlobalCbuffer();
            p.BindResource("u_targetBuffer", resourceType);

            OMM_ASSERT(byteSize % 4 == 0);
            const uint32_t numElements = byteSize / 4;

            const uint32_t threadGroupCountX = math::DivUp<uint32_t>(numElements, 128u);
            OMM_ASSERT(threadGroupCountX != 0);

            CBufferWriter& lCb = p.AddComputeDispatch(m_pipelines.ommClearBufferIdx, threadGroupCountX, 1);
            lCb.WriteDW(0 /*ClearValue*/);
            lCb.WriteDW(numElements /*NumElements*/);
            lCb.WriteDW(0 /*BufferOffset*/);
            for (uint32_t i = 0; i < 5u; ++i)
                lCb.WriteDW(0u /*dummy*/);
            END_PASS();
        };

        if (info.scratchBuffer.allocator.GetCurrentReservation() != 0)
            ClearBuffer("ClearScratchMemory0", &info.scratchBuffer);
        if (info.scratchBuffer0.allocator.GetCurrentReservation() != 0)
            ClearBuffer("ClearScratchMemory1", &info.scratchBuffer0);
        if (info.indArgBuffer.allocator.GetCurrentReservation() != 0)
            ClearBuffer("ClearScratchMemory2", &info.indArgBuffer);
        if (info.debugBuffer.allocator.GetCurrentReservation() != 0)
            ClearBuffer("ClearScratchMemory3", &info.debugBuffer);
    }

    if (computeOnly)
    {
        {
            SCOPED_LABEL("Init");

            m_passBuilder.PushPass(
                "InitCs",  ommGpuDispatchType_Compute, m_pipelines.ommInitBuffersCsBindings,
                [this, &config, &info](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();
                    p.BindSubRange("IEBakeCsBuffer", info.IEBakeCsBuffer);
                    p.BindResource("u_ommDescArrayHistogramBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM);
                    p.BindResource("u_ommIndexHistogramBuffer", ommGpuResourceType_OUT_OMM_INDEX_HISTOGRAM);
                    p.AddComputeDispatch(m_pipelines.ommInitBuffersCsIdx, math::DivUp<uint32_t>(config.maxSubdivisionLevel + 1, 128u), 1);
                }, enableValidation);
        }

        if (doSetup)
        {
            SCOPED_LABEL("WorkSetupCs");

            m_passBuilder.PushPass(
                "WorkSetupCs", ommGpuDispatchType_Compute, m_pipelines.ommWorkSetupCsBindings,
                [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();

                    p.BindResource("t_indexBuffer", ommGpuResourceType_IN_INDEX_BUFFER);
                    p.BindResource("t_texCoordBuffer", ommGpuResourceType_IN_TEXCOORD_BUFFER);
                    p.BindResource("u_ommDescArrayBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY);

                    p.BindResource("u_ommDescArrayHistogramBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM);

                    p.BindSubRange("HashTableBuffer", info.hashTableBuffer);
                    p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                    p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                    p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);
                    p.BindSubRange("IEBakeCsThreadCountBuffer", info.IEBakeCsThreadCountBuffer);
                    p.BindSubRange("IEBakeCsBuffer", info.IEBakeCsBuffer);
                    p.BindSubRange("BakeResultBufferCounterBuffer", info.bakeResultBufferCounter);
                    p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);

                    p.AddComputeDispatch(m_pipelines.ommWorkSetupCsIdx, math::DivUp<uint32_t>(primitiveCount, 128u), 1);
                }, enableValidation);
        }
        else
        {
            SCOPED_LABEL("WorkSetupBakeOnlyCs");

            m_passBuilder.PushPass(
                "WorkSetupBakeOnlyCs", ommGpuDispatchType_Compute, m_pipelines.ommWorkSetupBakeOnlyCsBindings,
                [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();

                    p.BindResource("t_ommDescArrayBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY);
                    p.BindResource("t_ommIndexBuffer", ommGpuResourceType_OUT_OMM_INDEX_BUFFER);

                    p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                    p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                    p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);
                    p.BindSubRange("TempOmmBakeScheduleTrackerBuffer", info.tempOmmBakeScheduleTrackerBuffer);
                    p.BindSubRange("IEBakeCsThreadCountBuffer", info.IEBakeCsThreadCountBuffer);
                    p.BindSubRange("BakeResultBufferCounterBuffer", info.bakeResultBufferCounter);
                    p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                    p.BindSubRange("IEBakeCsBuffer", info.IEBakeCsBuffer);

                    p.AddComputeDispatch(m_pipelines.ommWorkSetupBakeOnlyCsIdx, math::DivUp<uint32_t>(primitiveCount, 128u), 1);
                }, enableValidation);
        }

        if (doSetup)
        {
            SCOPED_LABEL("PostBuildInfo");

            m_passBuilder.PushPass(
                "PostBuildInfo",  ommGpuDispatchType_Compute, m_pipelines.ommPostBuildInfoBindings,
                [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();

                    p.BindResource("u_postBuildInfo", ommGpuResourceType_OUT_POST_DISPATCH_INFO);

                    p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                    p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);

                    p.AddComputeDispatch(m_pipelines.ommPostBuildInfoBuffersIdx, 1, 1);
                }, enableValidation);
        }

        if (doBake)
        {
            auto PushBakeRasterizeCs = [&](const char* debugName, uint32_t subdivisionLevel) {

                BEGIN_PASS(debugName,  ommGpuDispatchType_ComputeIndirect, m_pipelines.ommRasterizeCsBindings);
                p.UseGlobalCbuffer();
                p.BindResource("t_alphaTexture", ommGpuResourceType_IN_ALPHA_TEXTURE);
                p.BindResource("t_indexBuffer", ommGpuResourceType_IN_INDEX_BUFFER);
                p.BindResource("t_texCoordBuffer", ommGpuResourceType_IN_TEXCOORD_BUFFER);
                p.BindResource("u_vmArrayBuffer", ommGpuResourceType_OUT_OMM_ARRAY_DATA);

                p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                p.BindSubRange("IEBakeCsThreadCountBuffer", info.IEBakeCsThreadCountBuffer);
                p.BindSubRange("SpecialIndicesStateBuffer", info.specialIndicesStateBuffer);

                auto GetPipelineIndex = [&](uint alphaTextureChannel) {
                    if (alphaTextureChannel == 0)
                        return m_pipelines.ommRasterizeRCsIdx;
                    else if (alphaTextureChannel == 1)
                        return m_pipelines.ommRasterizeGCsIdx;
                    else if (alphaTextureChannel == 2)
                        return m_pipelines.ommRasterizeBCsIdx;
                    // else  if (alphaTextureChannel == 3)
                    OMM_ASSERT(alphaTextureChannel == 3);
                    return m_pipelines.ommRasterizeACsIdx;
                };

                CBufferWriter& lCb = p.AddComputeIndirect(GetPipelineIndex(config.alphaTextureChannel), info.IEBakeCsBuffer, kIndirectDispatchEntryStride * subdivisionLevel);
                lCb.WriteDW(subdivisionLevel /*levelIt*/);
                for (uint32_t i = 0; i < 7u; ++i)
                    lCb.WriteDW(0u /*dummy*/);
                END_PASS();
            };

            for (uint32_t subdivisionLevel = 0; subdivisionLevel <= config.maxSubdivisionLevel; ++subdivisionLevel)
            {
                SCOPED_LABEL("ResampleCs:%d", subdivisionLevel);
                PushBakeRasterizeCs("ResampleCs", subdivisionLevel);
            }
        }
        
    }
    else
    {
        {
            SCOPED_LABEL("Init");

            m_passBuilder.PushPass(
                "InitCS",  ommGpuDispatchType_Compute, m_pipelines.ommInitBuffersGfxBindings,
                [this, &config, &info](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();
                    p.BindSubRange("IEBakeBuffer", info.IEBakeBuffer);
                    p.BindSubRange("IECompressCsBuffer", info.IECompressCsBuffer);
                    p.BindResource("u_ommDescArrayHistogramBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM);
                    p.BindResource("u_ommIndexHistogramBuffer", ommGpuResourceType_OUT_OMM_INDEX_HISTOGRAM);
                    p.AddComputeDispatch(m_pipelines.ommInitBuffersGfxIdx, math::DivUp<uint32_t>((config.maxSubdivisionLevel + 1) * info.MaxBatchCount, 128u), 1);
                }, enableValidation);
        }

        {
            SCOPED_LABEL("WorkSetup");

            if (doSetup)
            {
                m_passBuilder.PushPass(
                    "WorkSetupCS", ommGpuDispatchType_Compute, m_pipelines.ommWorkSetupGfxBindings,
                    [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                    {
                        p.UseGlobalCbuffer();

                        p.BindResource("t_indexBuffer", ommGpuResourceType_IN_INDEX_BUFFER);
                        p.BindResource("t_texCoordBuffer", ommGpuResourceType_IN_TEXCOORD_BUFFER);

                        p.BindResource("u_ommDescArrayBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY);

                        p.BindResource("u_ommDescArrayHistogramBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM);

                        p.BindSubRange("HashTableBuffer", info.hashTableBuffer);
                        p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                        p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                        p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);
                        p.BindSubRange("DispatchIndirectThreadCountBuffer", info.dispatchIndirectThreadCounter);
                        p.BindSubRange("BakeResultBufferCounterBuffer", info.bakeResultBufferCounter);
                        p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                        p.BindSubRange("IEBakeBuffer", info.IEBakeBuffer);
                        p.BindSubRange("IECompressCsBuffer", info.IECompressCsBuffer);

                        p.AddComputeDispatch(m_pipelines.ommWorkSetupGfxIdx, math::DivUp<uint32_t>(primitiveCount, 128u), 1);
                    }, enableValidation);
            }
            else
            {
                m_passBuilder.PushPass(
                    "WorkSetupBakeOnlyCS", ommGpuDispatchType_Compute, m_pipelines.ommWorkSetupBakeOnlyGfxBindings,
                    [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                    {
                        p.UseGlobalCbuffer();

                        p.BindResource("t_ommDescArrayBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY);
                        p.BindResource("t_ommIndexBuffer", ommGpuResourceType_OUT_OMM_INDEX_BUFFER);

                        p.BindSubRange("TempOmmBakeScheduleTrackerBuffer", info.tempOmmBakeScheduleTrackerBuffer);
                        p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                        p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                        p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);
                        p.BindSubRange("DispatchIndirectThreadCountBuffer", info.dispatchIndirectThreadCounter);
                        p.BindSubRange("BakeResultBufferCounterBuffer", info.bakeResultBufferCounter);
                        p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                        p.BindSubRange("IEBakeBuffer", info.IEBakeBuffer);
                        p.BindSubRange("IECompressCsBuffer", info.IECompressCsBuffer);

                        p.AddComputeDispatch(m_pipelines.ommWorkSetupBakeOnlyGfxIdx, math::DivUp<uint32_t>(primitiveCount, 128u), 1);
                    }, enableValidation);
            }
        }

        if (doSetup)
        {
            SCOPED_LABEL("PostBuildInfo");

            m_passBuilder.PushPass(
                "PostBuildInfo",  ommGpuDispatchType_Compute, m_pipelines.ommPostBuildInfoBindings,
                [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
                {
                    p.UseGlobalCbuffer();

                    p.BindResource("u_postBuildInfo", ommGpuResourceType_OUT_POST_DISPATCH_INFO);

                    p.BindSubRange("OmmArrayAllocatorCounterBuffer", info.ommArrayAllocatorCounter);
                    p.BindSubRange("OmmDescAllocatorCounterBuffer", info.ommDescAllocatorCounter);

                    p.AddComputeDispatch(m_pipelines.ommPostBuildInfoBuffersIdx, 1, 1);
                }, enableValidation);
        }

        auto GetOmmRasterizeIdx = [&](uint alphaTextureChannel) {
            if (alphaTextureChannel == 0)
                return m_pipelines.ommRasterizeRIdx;
            else if (alphaTextureChannel == 1)
                return m_pipelines.ommRasterizeGIdx;
            else if (alphaTextureChannel == 2)
                return m_pipelines.ommRasterizeBIdx;
            // else  if (alphaTextureChannel == 3)
            OMM_ASSERT(alphaTextureChannel == 3);
            return m_pipelines.ommRasterizeAIdx;
        };

        const uint ommRasterizeIdx = GetOmmRasterizeIdx(config.alphaTextureChannel);

        if (doBake)
        {
            for (uint32_t levelIt = 0; levelIt <= config.maxSubdivisionLevel; levelIt++)
            {
                SCOPED_LABEL("Level %d", levelIt);

                auto GetMaxItemsPerBatch = [&info](uint32_t subdivisionLevel)
                {
                    const uint32_t numMicroTri = bird::GetNumMicroTriangles(subdivisionLevel);
                    const uint32_t rasterItemByteByteSize = (numMicroTri) * 8; // We need 2 x uint32 for each micro-VM state.
                    const size_t bakeResultBufferSize = info.bakeResultBuffer.GetSize();
                    return bakeResultBufferSize / rasterItemByteByteSize;
                };

                const uint32_t maxNumMicroTris = bird::GetNumMicroTriangles(levelIt);
                const size_t totalScratchMemory = (size_t)maxNumMicroTris * primitiveCount * (sizeof(uint32_t) * 2);
                const uint32_t maxBatchCountForLevel = (uint32_t)math::DivUp<size_t>(totalScratchMemory, info.bakeResultBuffer.GetSize());

                const uint32_t maxItemsPerBatch = (uint32_t)GetMaxItemsPerBatch(levelIt);
                OMM_ASSERT(info.MaxBatchCount >= maxBatchCountForLevel);

                for (uint32_t batchIt = 0; batchIt < maxBatchCountForLevel; batchIt++)
                {
                    const uint32_t resultBufferStride = maxNumMicroTris;

                    std::optional<ScopedLabel> batchLabel;
                    if (maxBatchCountForLevel > 1)
                        batchLabel.emplace(m_passBuilder, "Batch %d", batchIt);

                    {
                        auto PushBakeRasterize = [&](const char* debugName, uint32_t pipelineIndex, const ShaderBindings& bindings) {

                            BEGIN_PASS(debugName, ommGpuDispatchType_DrawIndexedIndirect, bindings);
                            p.UseGlobalCbuffer();
                            p.BindResource("t_alphaTexture", ommGpuResourceType_IN_ALPHA_TEXTURE);
                            p.BindResource("t_indexBuffer", ommGpuResourceType_IN_INDEX_BUFFER);
                            p.BindResource("t_texCoordBuffer", ommGpuResourceType_IN_TEXCOORD_BUFFER);

                            p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                            p.BindSubRange("BakeResultBuffer", info.bakeResultBuffer);

                            p.BindIB(ommGpuResourceType_STATIC_INDEX_BUFFER, OmmStaticBuffersImpl::kStaticIndexBufferOffsets[levelIt]);
                            p.BindVB(ommGpuResourceType_STATIC_VERTEX_BUFFER, OmmStaticBuffersImpl::kStaticVertexBufferOffsets[levelIt]);

                            p.SetViewport(
                                0, 0,
                                cbuffer.ViewportSize.x, cbuffer.ViewportSize.y);

                            const uint32_t indirectDrawStrideInBytes = kIndirectDispatchEntryStride;
                            const uint32_t offset = levelIt * info.MaxBatchCount + batchIt;

                            const uint32_t PrimitiveIdOffset = batchIt * maxItemsPerBatch;

                            CBufferWriter& lCb = p.AddDrawIndirect(pipelineIndex, info.IEBakeBuffer, offset * indirectDrawStrideInBytes);
                            lCb.WriteDW(levelIt /*levelIt*/);
                            lCb.WriteDW(resultBufferStride /*vmResultBufferStride*/);
                            lCb.WriteDW(batchIt /*batchIt*/);
                            lCb.WriteDW(PrimitiveIdOffset /*PrimitiveIdOffset*/);
                            for (uint32_t i = 0; i < 4u; ++i)
                                lCb.WriteDW(0u /*dummy*/);
                            END_PASS();
                        };

                        SCOPED_LABEL("Rasterize");
                        PushBakeRasterize("Rasterize", ommRasterizeIdx, m_pipelines.ommRasterizeBindings);

                        const bool debug = ((uint32_t)config.bakeFlags & (uint32_t)ommGpuBakeFlags_EnableNsightDebugMode) == (uint32_t)ommGpuBakeFlags_EnableNsightDebugMode;
                        if (debug)
                        {
                            SCOPED_LABEL("Debug");
                            {
                                SCOPED_LABEL("DebugBakeVSPS");
                                PushBakeRasterize("DebugBakeVSPS", m_pipelines.ommRasterizeDebugIdx, m_pipelines.ommRasterizeDebugBindings);
                            }
                            {
                                SCOPED_LABEL("ClearRenderTarget");
                                PushBakeRasterize("ClearRenderTarget", m_pipelines.ommRenderTargetClearDebugIdx, m_pipelines.ommRenderTargetClearDebugBindings);
                            }
                        }

                        {
                            SCOPED_LABEL("Compress");

                            m_passBuilder.PushPass(
                                "Compress", ommGpuDispatchType_ComputeIndirect, m_pipelines.ommCompressBindings,
                                [&](PassBuilder::PassConfig& p)
                                {
                                    p.UseGlobalCbuffer();
                                    p.BindSubRange("RasterItemsBuffer", info.rasterItemsBuffer);
                                    p.BindSubRange("BakeResultBuffer", info.bakeResultBuffer);
                                    p.BindSubRange("DispatchIndirectThreadCountBuffer", info.dispatchIndirectThreadCounter);
                                    p.BindSubRange("SpecialIndicesStateBuffer", info.specialIndicesStateBuffer);

                                    p.BindResource("u_vmArrayBuffer", ommGpuResourceType_OUT_OMM_ARRAY_DATA);

                                    const uint32_t indirectDispatchStrideInBytes = kIndirectDispatchEntryStride;
                                    const uint32_t offset = levelIt * info.MaxBatchCount + batchIt;

                                    const uint32_t PrimitiveIdOffset = batchIt * maxItemsPerBatch;

                                    CBufferWriter& lCb = p.AddComputeIndirect(m_pipelines.ommCompressIdx, info.IECompressCsBuffer, offset * indirectDispatchStrideInBytes);
                                    lCb.WriteDW(levelIt /*levelIt*/);
                                    lCb.WriteDW(batchIt /*batchIt*/);
                                    lCb.WriteDW(PrimitiveIdOffset /*PrimitiveIdOffset*/);
                                    for (uint32_t i = 0; i < 5u; ++i)
                                        lCb.WriteDW(4u /*dummy*/);
                                }, enableValidation);
                        }
                    }
                }
            }
        }
    }

    {
        SCOPED_LABEL("DescPatchCS");

        m_passBuilder.PushPass(
            "DescPatchCS", ommGpuDispatchType_Compute, m_pipelines.ommDescPatchBindings,
            [this, &config, &info, primitiveCount](PassBuilder::PassConfig& p)
            {
                p.UseGlobalCbuffer();
                p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                p.BindSubRange("HashTableBuffer", info.hashTableBuffer);
                p.BindResource("u_ommIndexHistogramBuffer", ommGpuResourceType_OUT_OMM_INDEX_HISTOGRAM);
                p.BindResource("u_postBuildInfo", ommGpuResourceType_OUT_POST_DISPATCH_INFO);
                p.BindResource("t_ommDescArrayBuffer", ommGpuResourceType_OUT_OMM_DESC_ARRAY);

                p.BindSubRange("SpecialIndicesStateBuffer", info.specialIndicesStateBuffer);

                p.AddComputeDispatch(m_pipelines.ommDescPatchIdx, math::DivUp<uint32_t>(primitiveCount, 128u), 1);
            }, enableValidation);
    }

    {
        SCOPED_LABEL("IndexWriteCS");

        m_passBuilder.PushPass(
            "IndexWriteCS", ommGpuDispatchType_Compute, m_pipelines.ommIndexWriteBindings,
            [this, &config, &info, primitiveCount, IsOmmIndexFormat16bit](PassBuilder::PassConfig& p)
            {
                p.UseGlobalCbuffer();
                p.BindSubRange("TempOmmIndexBuffer", info.tempOmmIndexBuffer);
                p.BindResource("u_ommIndexBuffer", ommGpuResourceType_OUT_OMM_INDEX_BUFFER);

                const uint32_t threadCount = IsOmmIndexFormat16bit ? math::DivUp<uint32_t>(primitiveCount, 2u) : primitiveCount;

                CBufferWriter& lCb = p.AddComputeDispatch(m_pipelines.ommIndexWriteIdx, math::DivUp<uint32_t>(threadCount, 128u), 1);
                lCb.WriteDW(threadCount /*threadCount*/);
                for (uint32_t i = 0; i < 7u; ++i)
                    lCb.WriteDW(4u /*dummy*/);
            }, enableValidation);
    }

    m_passBuilder.Finalize();

    *outDispatchDesc = &m_passBuilder._result;
    return ommResult_SUCCESS;
}

ommResult  PipelineImpl::ConfigurePipeline(const ommGpuPipelineConfigDesc& config)
{
    m_config = config;
    return ommResult_SUCCESS;
}

// BakerImpl =================================================================================

BakerImpl::~BakerImpl()
{}

ommResult BakerImpl::Create(const ommBakerCreationDesc& desc)
{
    m_bakeCreationDesc = desc;
    m_log = Logger(desc.messageInterface);
    return ommResult_SUCCESS;
}

ommResult BakerImpl::CreatePipeline(const ommGpuPipelineConfigDesc& config, ommGpuPipeline* outPipeline)
{
    RETURN_STATUS_IF_FAILED(PipelineImpl::Validate(config));

    StdAllocator<uint8_t>& memoryAllocator = GetStdAllocator();
    PipelineImpl* implementation = Allocate<PipelineImpl>(memoryAllocator, memoryAllocator, GetLog());
    RETURN_STATUS_IF_FAILED(implementation->Create(config));
    *outPipeline = CreateHandle<ommGpuPipeline, PipelineImpl>(implementation);
    return ommResult_SUCCESS;
}

ommResult BakerImpl::DestroyPipeline(ommGpuPipeline pipeline)
{
    if (pipeline == 0)
        return m_log.InvalidArg("[Invalid Arg] - pipeline is null");

    PipelineImpl* impl = GetHandleImpl< PipelineImpl>(pipeline);
    StdAllocator<uint8_t>& memoryAllocator = GetStdAllocator();
    Deallocate(memoryAllocator, impl);
    return ommResult_SUCCESS;
}

} // namespace Gpu
} // namespace omm