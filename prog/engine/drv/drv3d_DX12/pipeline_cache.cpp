// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_cache.h"
#include "d3d12_error_handling.h"
#include "pipeline/blk_cache.h"

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_finally.h>


using namespace drv3d_dx12;

#define DX12_ENABLE_CACHE_COMPRESSION 1

#if 0
#define CACHE_INFO_VERBOSE(...) logdbg(__VA_ARGS__)
#else
#define CACHE_INFO_VERBOSE(...)
#endif

namespace
{
struct CacheFileHeader
{
  uint32_t magic;
  uint32_t version;
  uint32_t pointerSize;
  uint32_t deviceVendorID;
  uint32_t deviceID;
  uint32_t subSystemID;
  uint32_t revisionID;
  uint32_t flags;
  uint32_t shaderHashCount;
  uint32_t computeCaches;
  uint32_t graphicsBaseCaches;
  uint32_t computeSignatureCaches;
  uint32_t graphicsSignatureCaches;
  uint32_t graphicsMeshSignatureCaches;
  uint32_t graphicsStaticStateCaches;
  uint32_t inputLayoutCaches;
  uint32_t framebufferLayoutCaches;
  uint32_t librarySize;
  uint32_t compressedContentSize;
  uint32_t uncompressedContentSize;
  uint64_t rawDriverVersion;
  dxil::HashValue headerChecksum;
  dxil::HashValue dataChecksum;
  dxil::HashValue shaderHashHash;
};

enum CacheFileFlags
{
  CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES = 1u << 0,
};

constexpr uint32_t CACHE_FILE_MAGIC = _MAKE4C('CX12');
constexpr uint32_t CACHE_FILE_VERSION = 34;
constexpr uint32_t EXPECTED_POINTER_SIZE = static_cast<uint32_t>(sizeof(void *));
// Version history:
// 1 - initial
// 2 - addition graphics and compute signature cache
// 3 - signatures support root constants
// 4 - render state update
// 5 - bindless textures
// 6 - renamed unboundedStartIndices to indicesToUnboundedSRVs
// 7 - bindless samplers
// 8 - removal of old static state and replacement with input layouts and wire frame bit
// 9 - framebuffer layout is now a separate element and pipelines reference it with ids
// 10 - bugfix in root signature generator fixed, all cached blobs are wrong
// 11 - internal structure refinement, shader header change, bindless root signature update
// 12 - added pointerSize member in header to be able to see if the 32 or 64 bit exe did write the cache file
// 13 - added driver version and to disable cache on old driver versions
// 14 - changed uav descriptor table layout of root signatures
// 15 - changed srv descriptor table layout of root signatures
// 16 - added flags field to header
// 17 - use of shader header inOutSemanticMask changed, masking rules for color outputs of graphics pipelines
//      have been updated and result in different masks and active render targets
// 18 - added support for view instancing
// 19 - mesh shader support, cache for mesh pipeline signatures
// 20 - independent blending support
// 21 - BasePipelineIdentifier only has hashes for vs and ps. Dropped hashes for gs, hs and ds as its no longer used.
//      Also changes pipeline library names.
// 22 - GraphicsPipeline::VariantCacheEntry changed
// 23 - MSAA support
// 24 - shader bin dump table / cache invalidation
// 25 - graphics root signature was using wrong type for combine masks
// 26 - InputLayout data structure updated, fixed padding issues, reduced size, fixed issues with uninitialized bits
// 27 - FramebufferLayout data structure updated, fixed padding issues, fixed issues with uninitialized bits
// 28 - Dual source blending support
// 29 - Stream output support
// 30 - Dual source blending fix
// 31 - Add useResourceDescriptorHeapIndexing and useSamplerDescriptorHeapIndexing in root signature definitions
// 32 - Bit packing of root signature definitions
// 33 - Added isMesh bit to graphics root signature definitions to properly distinguish between regular and mesh root signatures
// 34 - Shader dump dataHash now covers the bytecode, all shader identity hashes and pipeline library names changed
} // namespace

void PipelineCache::init(const SetupParameters &params)
{
  G_UNUSED(params);
#if _TARGET_PC_WIN
  D3D12_FEATURE_DATA_SHADER_CACHE cacheFlags = {};
  if (DX12_CHECK_OK(params.device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &cacheFlags, sizeof(cacheFlags))))
  {
    deviceFeatures = cacheFlags.SupportFlags & params.allowedModes;
  }

  logdbg("PipelineCache features:");
  logdbg("Per PSO cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO) != 0);
  logdbg("Pipeline cache library: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0);
  logdbg("Automatic OS in memory cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE) != 0);
  logdbg("Automatic OS on disk cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE) != 0);

  library.Reset();

  loadFromFile(params);
  if (!library && useLibrary())
  {
    // just report an error but don't abort, library is not essential
    DX12_DEBUG_RESULT(params.device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&library)));
  }

  setupBuildInPipelineLibrary(params, deviceFeatures);
#endif
}

struct MemoryWriteBuffer
{
  dag::Vector<uint8_t> buf;
  void append(const void *ptr, size_t sz)
  {
    buf.insert(buf.end(), reinterpret_cast<const uint8_t *>(ptr), reinterpret_cast<const uint8_t *>(ptr) + sz);
  }
};

void PipelineCache::shutdown(const ShutdownParameters &params)
{
  G_UNUSED(params);
#if _TARGET_PC_WIN
  FINALLY([&]() {
    library.Reset();
    initialPipelineBlob.clear();
    graphicsCache.clear();
    computeBlobs.clear();
    computeSignatures.clear();
    graphicsSignatures.clear();
    graphicsMeshSignatures.clear();
  });

  storeBuildInPipelineLibraryFile(params);
  shutdownBuildInPipelineLibrary();

  if ((hasChanged || params.alwaysGenerateBlks) && params.generateBlks)
  {
    DataBlock cacheOutBlock;
    if (auto inputLayoutOutBlock = cacheOutBlock.addNewBlock("input_layouts"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::InputLayoutEncoder> visitor{*inputLayoutOutBlock};
      for (auto &layout : inputLayouts)
      {
        visitor.encode2(layout);
      }
    }

    if (auto renderStateOutBlock = cacheOutBlock.addNewBlock("render_states"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::RenderStateEncoder> visitor{*renderStateOutBlock};
      for (auto &state : staticRenderStates)
      {
        visitor.encode2(state);
      }
    }

    if (auto framebufferLayoutOutBlock = cacheOutBlock.addNewBlock("framebuffer_layouts"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::FramebufferLayoutEncoder> visitor{*framebufferLayoutOutBlock};
      for (auto &layout : framebufferLayouts)
      {
        visitor.encode2(layout);
      }
    }

    cacheOutBlock.saveToTextFile("cache/dx12_cache.blk");
  }

  if (!hasChanged)
    return;

  dd_mkpath(params.fileName);

  FullFileSaveCB cacheFile{params.fileName, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING};
  if (!cacheFile.fileHandle)
  {
    D3D_ERROR("DX12: Failed to open %s to serialize pipeline cache, missing cache folder or "
              "insufficient privileges might be the cause for this error",
      params.fileName);
    return;
  }

  logdbg("DX12: Serializing pipeline caches to %s", params.fileName);

  CacheFileHeader fileHeader = {};
  fileHeader.magic = CACHE_FILE_MAGIC;
  fileHeader.version = CACHE_FILE_VERSION;
  fileHeader.pointerSize = EXPECTED_POINTER_SIZE;
  fileHeader.deviceVendorID = params.deviceVendorID;
  fileHeader.deviceID = params.deviceID;
  fileHeader.subSystemID = params.subSystemID;
  fileHeader.revisionID = params.revisionID;
  fileHeader.shaderHashCount = shaderInDumpCount;
  fileHeader.computeCaches = static_cast<uint32_t>(computeBlobs.size());
  fileHeader.graphicsBaseCaches = static_cast<uint32_t>(graphicsCache.size());
  fileHeader.computeSignatureCaches = static_cast<uint32_t>(computeSignatures.size());
  fileHeader.graphicsSignatureCaches = static_cast<uint32_t>(graphicsSignatures.size());
  fileHeader.graphicsMeshSignatureCaches = static_cast<uint32_t>(graphicsMeshSignatures.size());
  fileHeader.graphicsStaticStateCaches = static_cast<uint32_t>(staticRenderStates.size());
  fileHeader.inputLayoutCaches = static_cast<uint32_t>(inputLayouts.size());
  fileHeader.framebufferLayoutCaches = static_cast<uint32_t>(framebufferLayouts.size());
  fileHeader.shaderHashHash = shaderInDumpHash;
  fileHeader.flags = 0;
  if (params.rootSignaturesUsesCBVDescriptorRanges)
  {
    fileHeader.flags |= CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES;
  }
  fileHeader.rawDriverVersion = params.rawDriverVersion;
  if (library)
  {
    uint64_t libSize = library->GetSerializedSize();
    G_ASSERTF_RETURN(libSize <= eastl::numeric_limits<decltype(fileHeader.librarySize)>::max(), ,
      "DX12: Shader lib sizes of %llu bytes exceeded the bit limit of the header field", uint64_t{libSize});

    fileHeader.librarySize = static_cast<uint32_t>(libSize);
  }

  MemoryWriteBuffer mem;
  for (auto &&cc : computeBlobs)
  {
    G_ASSERTF_RETURN(cc.blob.size() <= eastl::numeric_limits<uint32_t>::max(), ,
      "DX12: cc.blob.size() %llu exceeds bit storage limit of the size field", uint64_t{cc.blob.size()});

    mem.append(&cc.hash, sizeof(cc.hash));
    auto s = static_cast<uint32_t>(cc.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(cc.blob.data(), cc.blob.size());
  }

  for (auto &&gc : graphicsCache)
  {
    G_ASSERTF_RETURN(gc.variantCache.size() <= eastl::numeric_limits<uint32_t>::max(), ,
      "DX12: gc.variantCache.size() %llu exceeds bit storage limit of the size field", uint64_t{gc.variantCache.size()});

    mem.append(&gc.ident, sizeof(gc.ident));
    auto s = static_cast<uint32_t>(gc.variantCache.size());
    mem.append(&s, sizeof(s));
    for (auto &&v : gc.variantCache)
    {
      G_ASSERTF_RETURN(v.blob.size() <= eastl::numeric_limits<uint32_t>::max(), ,
        "DX12: v.blob.size() %llu exceeds bit storage limit of the size field", uint64_t{v.blob.size()});

      mem.append(&v.topology, sizeof(v.topology));
      mem.append(&v.framebufferLayoutIndex, sizeof(v.framebufferLayoutIndex));
      mem.append(&v.inputLayoutIndex, sizeof(v.inputLayoutIndex));
      mem.append(&v.isWireFrame, sizeof(v.isWireFrame));
      mem.append(&v.staticRenderStateIndex, sizeof(v.staticRenderStateIndex));
      auto s2 = static_cast<uint32_t>(v.blob.size());
      mem.append(&s2, sizeof(s2));
      mem.append(v.blob.data(), v.blob.size());
    }
  }

  for (auto &&cs : computeSignatures)
  {
    G_ASSERTF_RETURN(cs.blob.size() <= eastl::numeric_limits<uint32_t>::max(), ,
      "DX12: cs.blob.size() %llu exceeds bit storage limit of the size field", uint64_t{cs.blob.size()});

    mem.append(&cs.def, sizeof(cs.def));
    auto s = static_cast<uint32_t>(cs.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(cs.blob.data(), cs.blob.size());
  }

  for (auto &&gs : graphicsSignatures)
  {
    G_ASSERTF_RETURN(gs.blob.size() <= eastl::numeric_limits<uint32_t>::max(), ,
      "DX12: gs.blob.size() %llu exceeds bit storage limit of the size field", uint64_t{gs.blob.size()});

    mem.append(&gs.def, sizeof(gs.def));
    auto s = static_cast<uint32_t>(gs.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(gs.blob.data(), gs.blob.size());
  }

  for (auto &&gs : graphicsMeshSignatures)
  {
    G_ASSERTF_RETURN(gs.blob.size() <= eastl::numeric_limits<uint32_t>::max(), ,
      "DX12: gs.blob.size() %llu exceeds bit storage limit of the size field", uint64_t{gs.blob.size()});

    mem.append(&gs.def, sizeof(gs.def));
    auto s = static_cast<uint32_t>(gs.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(gs.blob.data(), gs.blob.size());
  }

  for (auto &&gs : staticRenderStates)
    mem.append(&gs, sizeof(gs));

  for (auto &&il : inputLayouts)
    mem.append(&il, sizeof(il));

  for (auto &&fbl : framebufferLayouts)
    mem.append(&fbl, sizeof(fbl));

  if (library)
  {
    dag::Vector<uint8_t> blob;
    blob.resize(library->GetSerializedSize());
    library->Serialize(blob.data(), blob.size());
    mem.append(blob.data(), blob.size());
  }

  dag::Vector<uint8_t> compressedMemory;
#if DX12_ENABLE_CACHE_COMPRESSION
  compressedMemory.resize(mem.buf.size() * 2);
  // level 5 is the time/size sweet spot for this payload: near-best
  // ratio at a fraction of the compression time.
  auto compSize = zstd_compress(compressedMemory.data(), compressedMemory.size(), mem.buf.data(), mem.buf.size(), 5);
  // if we encountered an error or for some reason the data is not smaller,
  // use uncompressed instead
  if (compSize >= mem.buf.size())
    compSize = 0;
  compressedMemory.resize(compSize);
#endif

  G_ASSERTF_RETURN(mem.buf.size() <= eastl::numeric_limits<decltype(fileHeader.uncompressedContentSize)>::max(), ,
    "DX12: library memory size of %llu bytes, exceeds bit limit of header field", uint64_t{mem.buf.size()});

  fileHeader.compressedContentSize = static_cast<uint32_t>(compressedMemory.size());
  fileHeader.uncompressedContentSize = static_cast<uint32_t>(mem.buf.size());
  fileHeader.dataChecksum = dxil::HashValue::calculate(mem.buf.data(), mem.buf.size());
  fileHeader.headerChecksum = dxil::HashValue::calculate(&fileHeader, 1);

  logdbg("D12: Writing pipeline cache with %u bytes of size",
    sizeof(fileHeader) + (fileHeader.compressedContentSize ? fileHeader.compressedContentSize : fileHeader.uncompressedContentSize));
  cacheFile.write(&fileHeader, sizeof(fileHeader));
  if (!compressedMemory.empty())
    cacheFile.write(compressedMemory.data(), compressedMemory.size());
  else
    cacheFile.write(mem.buf.data(), mem.buf.size());
  cacheFile.close();
#endif
}

GraphicsPipelineBaseCacheId PipelineCache::getGraphicsPipeline(const BasePipelineIdentifier &ident)
{
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  auto ref =
    eastl::find_if(begin(graphicsCache), end(graphicsCache), [=](const GraphicsPipeline &pipe) { return pipe.ident == ident; });

  if (ref == end(graphicsCache))
  {
    ref = graphicsCache.insert(ref, GraphicsPipeline{});
    ref->ident = ident;
  }

  return GraphicsPipelineBaseCacheId{ref - eastl::begin(graphicsCache)};
}

bool PipelineCache::containsGraphicsPipeline(const BasePipelineIdentifier &ident)
{
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  auto ref =
    eastl::find_if(begin(graphicsCache), end(graphicsCache), [=](const GraphicsPipeline &pipe) { return pipe.ident == ident; });

  return ref != end(graphicsCache);
}

size_t PipelineCache::addGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
  const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, ID3D12PipelineState *pipeline)
{
  G_UNUSED(base_id);
  G_UNUSED(topology);
  G_UNUSED(input_layout);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(pipeline);
#if _TARGET_PC_WIN
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  auto &base = graphicsCache[base_id.get()];
  auto inputLayoutIndex = getInputLayoutIndex(input_layout);
  auto staticRenderStateIndex = getStaticRenderStateIndex(static_state);
  auto framebufferLayoutIndex = getFramebufferLayoutIndex(fb_layout);
  auto ref = eastl::find_if(begin(base.variantCache), end(base.variantCache),
    [=](const GraphicsPipeline::VariantCacheEntry &vce) //
    {
      return vce.topology == topology && vce.inputLayoutIndex == inputLayoutIndex && (vce.isWireFrame != 0) == is_wire_frame &&
             vce.staticRenderStateIndex == staticRenderStateIndex && vce.framebufferLayoutIndex == framebufferLayoutIndex;
    });
  if (ref == end(base.variantCache))
  {
    ref = base.variantCache.insert(ref, GraphicsPipeline::VariantCacheEntry{});
    ref->topology = topology;
    ref->inputLayoutIndex = inputLayoutIndex;
    ref->isWireFrame = is_wire_frame;
    ref->staticRenderStateIndex = staticRenderStateIndex;
    ref->framebufferLayoutIndex = framebufferLayoutIndex;
  }
  if (usePSOBlobs())
  {
    ComPtr<ID3DBlob> blob;
    if (DX12_CHECK_OK(pipeline->GetCachedBlob(&blob)))
    {
      if (blob->GetBufferSize() > 1)
      {
        auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
        auto to = from + blob->GetBufferSize();
        ref->blob.assign(from, to);
      }
    }
  }
  else if (library)
  {
    GraphicsPipelineVariantName name;
    name.generate(base.ident, topology, inputLayoutIndex, is_wire_frame, staticRenderStateIndex, framebufferLayoutIndex);
    library->StorePipeline(name.str, pipeline);
  }
  hasChanged = true;
  return static_cast<size_t>(ref - begin(base.variantCache));
#endif
  return 0;
}

size_t PipelineCache::addGraphicsMeshPipelineVariant(GraphicsPipelineBaseCacheId base_id, bool is_wire_frame,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, ID3D12PipelineState *pipeline)
{
#if _TARGET_PC_WIN
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  // Uses vertex pipeline entries for now, topology is set to undefined and input layout index to 0.
  auto &base = graphicsCache[base_id.get()];
  auto staticRenderStateIndex = getStaticRenderStateIndex(static_state);
  auto framebufferLayoutIndex = getFramebufferLayoutIndex(fb_layout);
  auto ref = eastl::find_if(begin(base.variantCache), end(base.variantCache),
    [=](const GraphicsPipeline::VariantCacheEntry &vce) //
    {
      return (vce.isWireFrame != 0) == is_wire_frame && vce.staticRenderStateIndex == staticRenderStateIndex &&
             vce.framebufferLayoutIndex == framebufferLayoutIndex;
    });
  if (ref == end(base.variantCache))
  {
    ref = base.variantCache.insert(ref, GraphicsPipeline::VariantCacheEntry{});
    ref->topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    ref->inputLayoutIndex = 0;
    ref->isWireFrame = is_wire_frame;
    ref->staticRenderStateIndex = staticRenderStateIndex;
    ref->framebufferLayoutIndex = framebufferLayoutIndex;
  }
  if (usePSOBlobs())
  {
    ComPtr<ID3DBlob> blob;
    if (DX12_CHECK_OK(pipeline->GetCachedBlob(&blob)))
    {
      if (blob->GetBufferSize() > 1)
      {
        auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
        auto to = from + blob->GetBufferSize();
        ref->blob.assign(from, to);
      }
    }
  }
  else if (library)
  {
    GraphicsPipelineVariantName name;
    name.generate(base.ident, D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, 0, is_wire_frame, staticRenderStateIndex,
      framebufferLayoutIndex);
    library->StorePipeline(name.str, pipeline);
  }
  hasChanged = true;
  return static_cast<size_t>(ref - begin(base.variantCache));
#else
  G_UNUSED(base_id);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(pipeline);
#endif
  return 0;
}

ComPtr<ID3D12PipelineState> PipelineCache::loadGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id,
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topology, const InputLayout &input_layout, bool is_wire_frame,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, D3D12_PIPELINE_STATE_STREAM_DESC desc,
  D3D12_CACHED_PIPELINE_STATE &blob_target)
{
  G_UNUSED(base_id);
  G_UNUSED(topology);
  G_UNUSED(input_layout);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(desc);
  G_UNUSED(blob_target);
#if _TARGET_PC_WIN
  // The pipeline library is thread-safe except for loading the same pipeline concurrently. So we might exclude LoadPipeline from the
  // lock scope.
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  ComPtr<ID3D12PipelineState> result;
  auto &base = graphicsCache[base_id.get()];
  auto inputLayoutIndex = getInputLayoutIndex(input_layout);
  auto staticRenderStateIndex = getStaticRenderStateIndex(static_state);
  auto framebufferLayoutIndex = getFramebufferLayoutIndex(fb_layout);
  if (usePSOBlobs())
  {
    auto ref = eastl::find_if(begin(base.variantCache), end(base.variantCache),
      [=](const GraphicsPipeline::VariantCacheEntry &vce) //
      {
        return vce.topology == topology && vce.inputLayoutIndex == inputLayoutIndex && (vce.isWireFrame != 0) == is_wire_frame &&
               vce.staticRenderStateIndex == staticRenderStateIndex && vce.framebufferLayoutIndex == framebufferLayoutIndex;
      });
    if (ref != end(base.variantCache))
    {
      if (!ref->blob.empty())
      {
        blob_target.pCachedBlob = ref->blob.data();
        blob_target.CachedBlobSizeInBytes = ref->blob.size();
        CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache hit");
      }
      else
      {
        CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss (no blob)");
      }
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss (no entry)");
    }
  }
  else if (library)
  {
    GraphicsPipelineVariantName name;
    name.generate(base.ident, topology, inputLayoutIndex, is_wire_frame, staticRenderStateIndex, framebufferLayoutIndex);
    auto errorCode = library->LoadPipeline(name.str, &desc, COM_ARGS(&result));
    if (SUCCEEDED(errorCode))
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache hit");
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss");
    }
  }
  return result;
#endif
  return nullptr;
}

ComPtr<ID3D12PipelineState> PipelineCache::loadGraphicsMeshPipelineVariant(GraphicsPipelineBaseCacheId base_id, bool is_wire_frame,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, D3D12_PIPELINE_STATE_STREAM_DESC desc,
  D3D12_CACHED_PIPELINE_STATE &blob_target)
{
#if _TARGET_PC_WIN
  OSSpinlockScopedLock lock(pipelineCacheGuard);

  ComPtr<ID3D12PipelineState> result;
  auto &base = graphicsCache[base_id.get()];
  auto staticRenderStateIndex = getStaticRenderStateIndex(static_state);
  auto framebufferLayoutIndex = getFramebufferLayoutIndex(fb_layout);
  if (usePSOBlobs())
  {
    auto ref = eastl::find_if(begin(base.variantCache), end(base.variantCache),
      [=](const GraphicsPipeline::VariantCacheEntry &vce) //
      {
        return (vce.isWireFrame != 0) == is_wire_frame && vce.staticRenderStateIndex == staticRenderStateIndex &&
               vce.framebufferLayoutIndex == framebufferLayoutIndex;
      });
    if (ref != end(base.variantCache))
    {
      if (!ref->blob.empty())
      {
        blob_target.pCachedBlob = ref->blob.data();
        blob_target.CachedBlobSizeInBytes = ref->blob.size();
        CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache hit");
      }
      else
      {
        CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss (no blob)");
      }
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss (no entry)");
    }
  }
  else if (library)
  {
    GraphicsPipelineVariantName name;
    name.generate(base.ident, D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, 0, is_wire_frame, staticRenderStateIndex,
      framebufferLayoutIndex);
    auto errorCode = library->LoadPipeline(name.str, &desc, COM_ARGS(&result));
    if (SUCCEEDED(errorCode))
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache hit");
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss");
    }
  }
  return result;
#else
  G_UNUSED(base_id);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(desc);
  G_UNUSED(blob_target);
#endif
  return nullptr;
}

void PipelineCache::addCompute(const dxil::HashValue &shader, ID3D12PipelineState *pipeline)
{
  G_UNUSED(shader);
  G_UNUSED(pipeline);
#if _TARGET_PC_WIN
  OSSpinlockScopedLock lock(pipelineCacheGuard);
  if (usePSOBlobs())
  {
    ComPtr<ID3DBlob> blob;
    if (DX12_CHECK_OK(pipeline->GetCachedBlob(&blob)))
    {
      if (blob->GetBufferSize() > 1)
      {
        auto ref = eastl::find_if(begin(computeBlobs), end(computeBlobs),
          [&](const ComputeBlob &blob_info) { return blob_info.hash == shader; });
        auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
        auto to = from + blob->GetBufferSize();
        if (ref != end(computeBlobs))
        {
          CACHE_INFO_VERBOSE("DX12: Updating compute pipeline cache");
          ref->blob.assign(from, to);
        }
        else
        {
          CACHE_INFO_VERBOSE("DX12: New compute pipeline cache entry");
          ComputeBlob newComputeBlob;
          newComputeBlob.hash = shader;
          newComputeBlob.blob.assign(from, to);
          computeBlobs.push_back(eastl::move(newComputeBlob));
        }
      }
    }
  }
  else if (library)
  {
    ComputePipelineName name = shader;
    library->StorePipeline(name.str, pipeline);
  }
  hasChanged = true;
#endif
}

ComPtr<ID3D12PipelineState> PipelineCache::loadCompute(const dxil::HashValue &shader, D3D12_PIPELINE_STATE_STREAM_DESC desc,
  D3D12_CACHED_PIPELINE_STATE &blob_target)
{
  G_UNUSED(shader);
  G_UNUSED(desc);
  G_UNUSED(blob_target);
#if _TARGET_PC_WIN
  OSSpinlockScopedLock lock(pipelineCacheGuard);
  ComPtr<ID3D12PipelineState> result;
  if (usePSOBlobs())
  {
    auto ref =
      eastl::find_if(begin(computeBlobs), end(computeBlobs), [&](const ComputeBlob &blob_info) { return blob_info.hash == shader; });
    if (ref != end(computeBlobs))
    {
      blob_target.pCachedBlob = ref->blob.data();
      blob_target.CachedBlobSizeInBytes = ref->blob.size();
      CACHE_INFO_VERBOSE("DX12: Compute pipeline cache hit");
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Compute pipeline cache miss");
    }
  }
  else if (library)
  {
    ComputePipelineName name = shader;
    auto errorCode = library->LoadPipeline(name.str, &desc, COM_ARGS(&result));
    if (SUCCEEDED(errorCode))
    {
      CACHE_INFO_VERBOSE("DX12: Compute pipeline cache hit");
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Compute pipeline cache miss");
    }
  }
  return result;
#endif
  return nullptr;
}

namespace
{
struct MemoryReadBuffer
{
  dag::Vector<uint8_t> buf;
  size_t location = 0;
  template <typename T>
  const T *readRange(size_t cnt = 1)
  {
    auto left = buf.size() - location;
    if (cnt * sizeof(T) > left)
      return nullptr;
    auto ptr = buf.data() + location;
    location += sizeof(T) * cnt;
    return reinterpret_cast<const T *>(ptr);
  }
  template <typename T>
  bool read(T &target)
  {
    auto left = buf.size() - location;
    if (sizeof(T) > left)
      return false;
    target = *reinterpret_cast<const T *>(buf.data() + location);
    location += sizeof(T);
    return true;
  }
};
} // namespace

void PipelineCache::preRecovery()
{
#if _TARGET_PC_WIN
  if (library && hasChanged)
  {
    // try to recover pipeline library cache blob and reuse it later
    dag::Vector<uint8_t> newBlob;
    auto sz = library->GetSerializedSize();
    if (sz)
    {
      newBlob.resize(sz);
      library->Serialize(newBlob.data(), newBlob.size());
      // delete lib object and then swap memory blob to reuse later
      library.Reset();
      initialPipelineBlob = eastl::move(newBlob);
    }
  }
  if (library)
  {
    library.Reset();
  }
  preRecoveryBuildInPipelineLibrary();
#endif
}

void PipelineCache::recover(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes)
{
#if _TARGET_PC_WIN
  D3D12_FEATURE_DATA_SHADER_CACHE cacheFlags = {};
  if (DX12_CHECK_OK(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &cacheFlags, sizeof(cacheFlags))))
  {
    deviceFeatures = cacheFlags.SupportFlags & allowed_modes;
  }

  logdbg("PipelineCache features:");
  logdbg("Per PSO cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO) != 0);
  logdbg("Pipeline cache library: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0);
  logdbg("Automatic OS in memory cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE) != 0);
  logdbg("Automatic OS on disk cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE) != 0);

  library.Reset();

  if (useLibrary())
  {
    // just report an error but don't abort, library is not essential
    DX12_DEBUG_RESULT(device->CreatePipelineLibrary(initialPipelineBlob.data(), initialPipelineBlob.size(), COM_ARGS(&library)));
    if (!library)
    {
      DX12_DEBUG_RESULT(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&library)));
    }
  }

  recoverBuildInPipelineLibrary(device, deviceFeatures);
#else
  G_UNUSED(device);
  G_UNUSED(allowed_modes);
#endif
}

bool PipelineCache::onBindumpLoad(D3DDevice *device, eastl::span<const dxil::HashValue> all_shader_hashes)
{
  logdbg("DX12: onBindumpLoad...");
  auto newHash = dxil::HashValue::calculate(all_shader_hashes.data(), all_shader_hashes.size());

  if (static_cast<uint32_t>(all_shader_hashes.size()) == shaderInDumpCount && newHash == shaderInDumpHash)
  {
    logdbg("DX12: Full match!");
    return true;
  }

  logdbg("DX12: No match, resetting cache");
  // not matching
  shaderInDumpCount = static_cast<uint32_t>(all_shader_hashes.size());
  shaderInDumpHash = newHash;
  // only need to clear out pipelines
  graphicsCache.clear();
  computeBlobs.clear();
  computeSignatures.clear();
  graphicsSignatures.clear();
  graphicsMeshSignatures.clear();
#if !_TARGET_SCARLETT
  library.Reset();

  if (useLibrary())
  {
    DX12_DEBUG_RESULT(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&library)));
  }
#else
  G_UNUSED(device);
#endif
  initialPipelineBlob.clear();
  hasChanged = true;
  return false;
}

#if _TARGET_PC_WIN
namespace
{
struct BuildInFormatNameStore
{
  static constexpr uint32_t name_len = 255;
  wchar_t name[name_len + 1];
  BuildInFormatNameStore(const wchar_t *prefix, DXGI_FORMAT format)
  {
    _snwprintf(name, name_len, L"%s%X", prefix, static_cast<uint32_t>(format));
    name[name_len] = L'\0';
  }
};
const wchar_t blit_prefix[] = L"B:";
const wchar_t clear_prefix[] = L"C:";
} // namespace

dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> PipelineCache::loadPipeline(FormatBasedBuildInPipelineType type,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, DXGI_FORMAT out_format)
{
  if (!buildInPipelineLibrary)
  {
    return dag::Unexpected<HRESULT>{DXGI_ERROR_UNSUPPORTED};
  }
  dag::Vector<DXGI_FORMAT> *formatTable = nullptr;
  const wchar_t *prefix = nullptr;
  switch (type)
  {
    case FormatBasedBuildInPipelineType::Blit:
      prefix = blit_prefix;
      formatTable = &blitOutputFormats;
      break;
    case FormatBasedBuildInPipelineType::Clear:
      prefix = clear_prefix;
      formatTable = &clearOutputFormats;
      break;
  }
  if (!prefix || !formatTable)
  {
    D3D_ERROR("DX12: PipelineCache::loadPipeline invalid FormatBasedBuildInPipelineType value %u", static_cast<uint32_t>(type));
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }

  if (formatTable->end() == eastl::find(formatTable->begin(), formatTable->end(), out_format))
  {
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }
  BuildInFormatNameStore name{prefix, out_format};
  ComPtr<ID3D12PipelineState> result;
  auto errorCode = buildInPipelineLibrary->LoadPipeline(name.name, &desc, COM_ARGS(&result));
  if (!result)
  {
    return dag::Unexpected<HRESULT>{errorCode};
  }
  return result;
}

dag::Expected<void, HRESULT> PipelineCache::storePipeline(FormatBasedBuildInPipelineType type, ID3D12PipelineState *pipeline,
  DXGI_FORMAT out_format)
{
  if (!buildInShouldRecord)
  {
    return {};
  }
  dag::Vector<DXGI_FORMAT> *formatTable = nullptr;
  const wchar_t *prefix = nullptr;
  switch (type)
  {
    case FormatBasedBuildInPipelineType::Blit:
      prefix = blit_prefix;
      formatTable = &blitOutputFormats;
      break;
    case FormatBasedBuildInPipelineType::Clear:
      prefix = clear_prefix;
      formatTable = &clearOutputFormats;
      break;
  }
  if (!prefix || !formatTable)
  {
    D3D_ERROR("DX12: PipelineCache::storePipeline invalid FormatBasedBuildInPipelineType value %u", static_cast<uint32_t>(type));
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }
  // always add the format
  if (formatTable->end() == eastl::find(formatTable->begin(), formatTable->end(), out_format))
  {
    formatTable->push_back(out_format);
    buildInUpdated = true;
  }
  if (!buildInPipelineLibrary)
  {
    return {};
  }
  BuildInFormatNameStore name{prefix, out_format};
  auto errorCode = buildInPipelineLibrary->StorePipeline(name.name, pipeline);
  if (FAILED(errorCode))
  {
    return dag::Unexpected<HRESULT>{errorCode};
  }
  buildInUpdated = true;
  return {};
}

const dag::Vector<DXGI_FORMAT> *PipelineCache::getKnownPipelineFormats(FormatBasedBuildInPipelineType type)
{
  switch (type)
  {
    case FormatBasedBuildInPipelineType::Blit: return &blitOutputFormats;
    case FormatBasedBuildInPipelineType::Clear: return &clearOutputFormats;
  }
  D3D_ERROR("DX12: PipelineCache::getKnownPipelineFormats invalid FormatBasedBuildInPipelineType value %u",
    static_cast<uint32_t>(type));
  return nullptr;
}

bool PipelineCache::hasFormatBasedBuildInLibrary() { return static_cast<bool>(buildInPipelineLibrary); }

bool PipelineCache::hasExistingFormatBasedBuildInLibrary() { return 0 < loadedBuildInPipelineLibraryBlob.size(); }
#endif

bool PipelineCache::loadFromFile(const SetupParameters &params)
{
  shaderInDumpCount = 0;
  shaderInDumpHash = {};

  FullFileLoadCB cacheFile{params.fileName, DF_IGNORE_MISSING | DF_READ};
  if (!cacheFile.fileHandle)
  {
    logwarn("DX12: Failed to open %s to load pipeline cache", params.fileName);
    return false;
  }

  CacheFileHeader fileHeader = {};
  auto ln = cacheFile.tryRead(&fileHeader, sizeof(fileHeader));
  if (ln != sizeof(fileHeader) || (fileHeader.magic != CACHE_FILE_MAGIC))
  {
    logwarn("DX12: Pipeline cache file is invalid");
    return false;
  }

  if (fileHeader.version != CACHE_FILE_VERSION)
  {
    // begin outdated is okay.
    logdbg("DX12: Pipeline cache file is outdated");
    return false;
  }

  if ((0 != (fileHeader.flags & CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES)) != params.rootSignaturesUsesCBVDescriptorRanges)
  {
    // If used root signature mode does not match, we can not reuse cache
    logdbg("DX12: Pipeline incompatible root signature layout mode (CBV root descriptors vs "
           "descriptor ranges)");
    return false;
  }

  if ((fileHeader.deviceVendorID != params.deviceVendorID) || (fileHeader.deviceID != params.deviceID) ||
      (fileHeader.subSystemID != params.subSystemID) || (fileHeader.revisionID != params.revisionID))
  {
    // there is a chance that we might preload incompatible stuff, so we better throw away
    // the cache and rebuild everything from scratch.
    logwarn("DX12: Pipeline cache is incompatible, restoration aborted");
    return false;
  }

  dxil::HashValue compareHash = fileHeader.headerChecksum;
  fileHeader.headerChecksum = dxil::HashValue{};
  if (compareHash != dxil::HashValue::calculate(&fileHeader, 1))
  {
    logwarn("DX12: Pipeline cache file is corrupted");
    return false;
  }

  MemoryReadBuffer data;
  data.buf.resize(fileHeader.uncompressedContentSize);
  if (fileHeader.compressedContentSize)
  {
    dag::Vector<uint8_t> compressedData;
    compressedData.resize(fileHeader.compressedContentSize);
    ln = cacheFile.tryRead(compressedData.data(), compressedData.size());
    if (ln != compressedData.size())
    {
      logwarn("DX12: Error while reading data");
      return false;
    }
    auto outSize = zstd_decompress(data.buf.data(), data.buf.size(), compressedData.data(), compressedData.size());
    if (outSize != data.buf.size())
    {
      logwarn("DX12: Error while decompressing data");
      return false;
    }
  }
  else
  {
    ln = cacheFile.tryRead(data.buf.data(), data.buf.size());
    if (ln != data.buf.size())
    {
      logwarn("DX12: Error while reading data");
      return false;
    }
  }

  if (fileHeader.dataChecksum != dxil::HashValue::calculate(data.buf.data(), data.buf.size()))
  {
    logwarn("DX12: Pipeline cache file is corrupted");
    return false;
  }

  bool shouldLoadBlobsOrLibrary = true;
  // we have to drop the pipeline library/blob when the generator of the cache file was a different
  // bit version than we are running now, as drivers might fail to load 32 bit versions into 64 bit
  // versions and vice versa.
  if (fileHeader.pointerSize != EXPECTED_POINTER_SIZE)
  {
    shouldLoadBlobsOrLibrary = false;
    logwarn("DX12: Detected mismatch of pointer size of running executable (%u) and loaded cache "
            "(%u), drivers might fail to load cache library/blob so they will be ignored.",
      EXPECTED_POINTER_SIZE, fileHeader.pointerSize);
  }

  if (fileHeader.rawDriverVersion != params.rawDriverVersion)
  {
    shouldLoadBlobsOrLibrary = false;
    logwarn("DX12: Detected driver version mismatch, drivers might fail to load cache library/blob "
            "so they will be ignored.");
  }

  uint32_t computeBlobsLoaded = 0;
  size_t computeBlobsSize = 0;

  for (uint32_t cbi = 0; cbi < fileHeader.computeCaches; ++cbi)
  {
    ComputeBlob blob = {};
    if (!data.read(blob.hash))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      return false;
    }
    uint32_t sz = 0;
    if (!data.read(sz))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      return false;
    }
    auto pso = data.readRange<uint8_t>(sz);
    if (!pso)
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      return false;
    }
    if (usePSOBlobs() && shouldLoadBlobsOrLibrary)
    {
      blob.blob.assign(pso, pso + sz);
      computeBlobs.push_back(eastl::move(blob));
      ++computeBlobsLoaded;
      computeBlobsSize += sz;
    }
  }

  uint32_t graphicsWithAnyVariant = 0;
  uint32_t graphicsVairantsFound = 0;
  uint32_t graphicsVariantBlobsLoaded = 0;
  size_t graphicsVariantBlobsSize = 0;

  for (uint32_t gbci = 0; gbci < fileHeader.graphicsBaseCaches; ++gbci)
  {
    GraphicsPipeline blob = {};
    if (!data.read(blob.ident))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      return false;
    }
    uint32_t sz = 0;
    if (!data.read(sz))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      return false;
    }

    graphicsVairantsFound += sz;
    graphicsWithAnyVariant += sz > 0 ? 1 : 0;

    for (uint32_t vi = 0; vi < sz; ++vi)
    {
      GraphicsPipeline::VariantCacheEntry vce = {};
      if (!data.read(vce.topology))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.framebufferLayoutIndex))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.inputLayoutIndex))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.isWireFrame))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.staticRenderStateIndex))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      uint32_t psoSz = 0;
      if (!data.read(psoSz))
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      auto pso = data.readRange<uint8_t>(psoSz);
      if (!pso)
      {
        logwarn("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (usePSOBlobs() && shouldLoadBlobsOrLibrary)
      {
        vce.blob.assign(pso, pso + psoSz);
        graphicsVariantBlobsLoaded++;
        graphicsVariantBlobsSize += psoSz;
      }
      blob.variantCache.push_back(eastl::move(vce));
    }
    graphicsCache.push_back(eastl::move(blob));
  }

  size_t computeSignatureBlobSize = 0;

  for (uint32_t csi = 0; csi < fileHeader.computeSignatureCaches; ++csi)
  {
    ComputeSignatureBlob csb = {};
    if (!data.read(csb.def))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      return false;
    }
    uint32_t sigSz = 0;
    if (!data.read(sigSz))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      return false;
    }
    auto sig = data.readRange<uint8_t>(sigSz);
    if (!sig)
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      return false;
    }
    csb.blob.assign(sig, sig + sigSz);
    computeSignatures.push_back(eastl::move(csb));
    computeSignatureBlobSize += sigSz;
  }

  size_t graphicsSignatureBlobSize = 0;

  for (uint32_t gsi = 0; gsi < fileHeader.graphicsSignatureCaches; ++gsi)
  {
    GraphicsSignatureBlob gsb = {};
    if (!data.read(gsb.def))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      return false;
    }
    uint32_t sigSz = 0;
    if (!data.read(sigSz))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      return false;
    }
    auto sig = data.readRange<uint8_t>(sigSz);
    if (!sig)
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      return false;
    }
    gsb.blob.assign(sig, sig + sigSz);
    graphicsSignatures.push_back(eastl::move(gsb));
    graphicsSignatureBlobSize += sigSz;
  }

  size_t graphicsMeshSignatureBlobSize = 0;

  for (uint32_t gsi = 0; gsi < fileHeader.graphicsMeshSignatureCaches; ++gsi)
  {
    GraphicsSignatureBlob gsb = {};
    if (!data.read(gsb.def))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      graphicsMeshSignatures.clear();
      return false;
    }
    uint32_t sigSz = 0;
    if (!data.read(sigSz))
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      graphicsMeshSignatures.clear();
      return false;
    }
    auto sig = data.readRange<uint8_t>(sigSz);
    if (!sig)
    {
      logwarn("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      graphicsMeshSignatures.clear();
      return false;
    }
    gsb.blob.assign(sig, sig + sigSz);
    graphicsMeshSignatures.push_back(eastl::move(gsb));
    graphicsMeshSignatureBlobSize += sigSz;
  }

  staticRenderStates.resize(fileHeader.graphicsStaticStateCaches);
  for (uint32_t i = 0; i < fileHeader.graphicsStaticStateCaches; ++i)
  {
    if (!data.read(staticRenderStates[i]))
    {
      logwarn("DX12: Error while decoding static render state cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      staticRenderStates.clear();
      return false;
    }
  }

  inputLayouts.resize(fileHeader.inputLayoutCaches);
  for (uint32_t i = 0; i < fileHeader.inputLayoutCaches; ++i)
  {
    if (!data.read(inputLayouts[i]))
    {
      logwarn("DX12: Error while decoding input layout cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      staticRenderStates.clear();
      inputLayouts.clear();
      return false;
    }
  }

  framebufferLayouts.resize(fileHeader.framebufferLayoutCaches);
  for (uint32_t i = 0; i < fileHeader.framebufferLayoutCaches; ++i)
  {
    if (!data.read(framebufferLayouts[i]))
    {
      logwarn("DX12: Error while decoding framebuffer layout cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      staticRenderStates.clear();
      inputLayouts.clear();
      framebufferLayouts.clear();
      return false;
    }
  }

  if (fileHeader.librarySize && useLibrary() && shouldLoadBlobsOrLibrary)
  {
    auto lib = data.readRange<uint8_t>(fileHeader.librarySize);
    // for whatever reason the pipeline object will not copy the blob
    // but will later reference it, so the original blob has to be
    // kept around.
    initialPipelineBlob.assign(lib, lib + fileHeader.librarySize);

#if _TARGET_PC_WIN
    if (DX12_DEBUG_FAIL(
          params.device->CreatePipelineLibrary(initialPipelineBlob.data(), initialPipelineBlob.size(), COM_ARGS(&library))))
    {
      // delete old blob, for some reason we could not restore a lib from it
      initialPipelineBlob.clear();
    }
#endif
  }

  logdbg("DX12: Pipeline cache load statistics:");
  logdbg("DX12: Restored %u compute pipelines with %u bytes from cache", computeBlobsLoaded, computeBlobsSize);
  logdbg("DX12: Restored %u compute pipeline signatures with %u bytes from cache", fileHeader.computeSignatureCaches,
    computeSignatureBlobSize);
  logdbg("DX12: Restored %u graphics pipeline variants of %u (%u) base pipelines", graphicsVairantsFound, graphicsWithAnyVariant,
    fileHeader.graphicsBaseCaches);
  logdbg("DX12: Restored %u graphics pipelines with %u bytes from cache", graphicsVariantBlobsLoaded, graphicsVariantBlobsSize);
  logdbg("DX12: Restored %u graphics pipeline signatures with %u bytes from cache", fileHeader.graphicsSignatureCaches,
    graphicsSignatureBlobSize);
  logdbg("DX12: Restored %u graphics mesh pipeline signatures with %u bytes from cache", fileHeader.graphicsMeshSignatureCaches,
    graphicsMeshSignatureBlobSize);
  logdbg("DX12: Restored %u graphics pipeline static render states", staticRenderStates.size());
  logdbg("DX12: Restored %u graphics pipeline input layouts", inputLayouts.size());
  logdbg("DX12: Restored %u graphics pipeline framebuffer layouts", framebufferLayouts.size());
  logdbg("DX12: Restored pipeline library with %u bytes from cache", initialPipelineBlob.size());

  hasChanged = false;

  shaderInDumpCount = fileHeader.shaderHashCount;
  shaderInDumpHash = fileHeader.shaderHashHash;

  return true;
}

#if _TARGET_PC_WIN
void PipelineCache::setupBuildInPipelineLibrary(const SetupParameters &params, const D3D12_SHADER_CACHE_SUPPORT_FLAGS cache_support)
{
  buildInShouldRecord = params.recordBuildInCache;

  loadBuildInPipelineLibraryFile(params);

  recoverBuildInPipelineLibrary(params.device, cache_support);
}

namespace
{
enum class ReaderWrapperErrorCode
{
  GenericReadError,
};
template <typename S>
class ReadWrapper
{
  S &source;

public:
  ReadWrapper(S &s) : source{s} {}
  using ErrorCode = ReaderWrapperErrorCode;
  dag::Expected<void, ErrorCode> readBytes(void *ptr, size_t sz)
  {
    if (sz != source.tryRead(ptr, sz))
    {
      return dag::Unexpected{ErrorCode::GenericReadError};
    }
    return {};
  }
  template <typename T>
  dag::Expected<void, ErrorCode> readArray(T *ptr, size_t cnt)
  {
    return readBytes(ptr, sizeof(T) * cnt);
  }
  template <typename T>
  dag::Expected<T, ErrorCode> readValue()
  {
    T value;
    return readBytes(&value, sizeof(T)).and_then([&]() -> dag::Expected<T, ErrorCode> { return eastl::move(value); });
  }
  template <typename T>
  dag::Expected<T, ErrorCode> readContainer(uint32_t count)
  {
    T data;
    data.resize(count);
    return readArray(data.data(), data.size()).and_then([&]() -> dag::Expected<T, ErrorCode> { return eastl::move(data); });
  }
};
} // namespace

void PipelineCache::loadBuildInPipelineLibraryFile(const SetupParameters &params)
{
  buildInUpdated = false;

  if (!params.buildInCacheFileName || '\0' == params.buildInCacheFileName[0])
  {
    logwarn("DX12: Can not de-serialize build in pipeline cache, as no name was provided");
    return;
  }

  FullFileLoadCB cacheFile{params.buildInCacheFileName, DF_IGNORE_MISSING | DF_READ};
  if (!cacheFile.fileHandle)
  {
    logwarn("DX12: Failed to open %s to load build in pipeline cache", params.buildInCacheFileName);
    return;
  }

  ReadWrapper reader{cacheFile};

  [[maybe_unused]] auto readResult =
    reader.readValue<BuildInPipelineLibraryFileHeader>()
      .transform_error([&](auto error_code) {
        logwarn("DX12: Failed to load build in pipeline cache, can't read header");
        return error_code;
      })
      .and_then([&](const auto &header) -> dag::Expected<bool, ReaderWrapperErrorCode> {
        if ((BuildInPipelineLibraryFileHeader::magic_value != header.magic) ||
            (BuildInPipelineLibraryFileHeader::version_value != header.version))
        {
          logwarn("DX12: Failed to load build in pipeline cache, header versioning check failed");
          return dag::Unexpected{ReaderWrapperErrorCode::GenericReadError};
        }

        return reader.readValue<ShaderHashValue>()
          .transform_error([&](auto error_code) {
            logwarn("DX12: Failed to load build in pipeline cache, can't read header hash");
            return error_code;
          })
          .and_then([&](const auto &header_hash) -> dag::Expected<bool, ReaderWrapperErrorCode> {
            if (header_hash != ShaderHashValue::calculate(&header, 1))
            {
              logwarn("DX12: Failed to load build in pipeline cache, header integrity check failed");
              return dag::Unexpected{ReaderWrapperErrorCode::GenericReadError};
            }

            logdbg("DX12: Restoring pipeline cache for build in pipelines");
            return reader.readContainer<decltype(blitOutputFormats)>(header.blitCount)
              .transform_error([&](auto error_code) {
                logwarn("DX12: Failed to load blit output formats");
                return error_code;
              })
              .and_then([&](decltype(blitOutputFormats) &&bof) -> dag::Expected<bool, ReaderWrapperErrorCode> {
                if (ShaderHashValue::calculate(bof.data(), bof.size()) != header.blitFormatsHash)
                {
                  logwarn("DX12: Failed to load blit output formats, hash mismatch");
                  return dag::Unexpected{ReaderWrapperErrorCode::GenericReadError};
                }
                blitOutputFormats = eastl::move(bof);
                logdbg("DX12: Restored blit formats: %u", header.blitCount);

                return reader.readContainer<decltype(clearOutputFormats)>(header.clearCount)
                  .transform_error([&](auto error_code) {
                    logwarn("DX12: Failed to load clear output formats");
                    return error_code;
                  })
                  .and_then([&](decltype(clearOutputFormats) &&cof) -> dag::Expected<bool, ReaderWrapperErrorCode> {
                    if (ShaderHashValue::calculate(cof.data(), cof.size()) != header.clearFormatsHash)
                    {
                      logwarn("DX12: Failed to load clear output formats, hash mismatch");
                      return dag::Unexpected{ReaderWrapperErrorCode::GenericReadError};
                    }
                    clearOutputFormats = eastl::move(cof);
                    logdbg("DX12: Restored clear formats: %u", header.clearCount);

                    if (0 == (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY))
                    {
                      logdbg("DX12: Skipping restore of build in pipeline library, pipeline libraries disabled");
                      return false;
                    }

                    if ((params.blitByteCodeHash != header.blitHash) || (params.clearByteCodeHash != header.clearHash))
                    {
                      logdbg("DX12: Skipping restore of build in pipeline library, hash mismatch");
                      return false;
                    }

                    return reader.readContainer<decltype(loadedBuildInPipelineLibraryBlob)>(header.librarySize)
                      .transform_error([&](auto error_code) {
                        logwarn("DX12: Failed to load pipeline library");
                        return error_code;
                      })
                      .transform([&](decltype(loadedBuildInPipelineLibraryBlob) &&lbiplb) {
                        ByteUnits loadedLibrarySizeInBytes{header.librarySize};
                        logdbg("DX12: Loaded pipeline library size: %.4f %s", loadedLibrarySizeInBytes.units(),
                          loadedLibrarySizeInBytes.name());
                        loadedBuildInPipelineLibraryBlob = eastl::move(lbiplb);
                        return true;
                      });
                  });
              });
          });
      });
}

namespace
{
template <typename T>
class WriteWrapper
{
  T &target;
  bool ok = true;

public:
  WriteWrapper(T &t) : target{t} {}
  void writeBytes(const void *ptr, size_t sz) { ok = ok && ((0 == sz) || ((nullptr != ptr) && (sz == target.tryWrite(ptr, sz)))); }
  template <typename U>
  void write(const U &value)
  {
    return writeBytes(&value, sizeof(U));
  }
  bool close()
  {
    target.close();
    return ok;
  }
};
} // namespace

void PipelineCache::storeBuildInPipelineLibraryFile(const ShutdownParameters &params)
{
  if (!buildInUpdated)
  {
    logdbg("DX12: Skipping serializing build in pipeline cache, no changes");
    return;
  }
  if (!params.buildInCacheFileName || '\0' == params.buildInCacheFileName[0])
  {
    logwarn("DX12: Can not serialize build in pipeline cache, as no name was provided");
    return;
  }
  BuildInPipelineLibraryFileHeader header{
    .magic = BuildInPipelineLibraryFileHeader::magic_value,
    .version = BuildInPipelineLibraryFileHeader::version_value,
    .blitHash = params.blitByteCodeHash,
    .clearHash = params.clearByteCodeHash,
    .blitFormatsHash = ShaderHashValue::calculate(blitOutputFormats.data(), blitOutputFormats.size()),
    .clearFormatsHash = ShaderHashValue::calculate(clearOutputFormats.data(), clearOutputFormats.size()),
    .blitCount = static_cast<uint32_t>(blitOutputFormats.size()),
    .clearCount = static_cast<uint32_t>(clearOutputFormats.size()),
    .librarySize = static_cast<uint32_t>(buildInPipelineLibrary ? buildInPipelineLibrary->GetSerializedSize() : 0),
  };
  DynamicArray<uint8_t> buffer;
  if (buildInPipelineLibrary)
  {
    if (buildInPipelineLibrary->GetSerializedSize() == header.librarySize)
    {
      buffer.resize(header.librarySize);

      if (FAILED(buildInPipelineLibrary->Serialize(buffer.data(), buffer.size())))
      {
        header.librarySize = 0;
        buffer.resize(0);
      }
    }
    else
    {
      // Chances to end up here is pretty much 0. If we still end up here, there is something wrong with the library and we better
      // drop it anyways.
      header.librarySize = 0;
    }
  }

  FullFileSaveCB cacheFile{params.buildInCacheFileName, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING};
  if (!cacheFile.fileHandle)
  {
    logwarn("DX12: Failed to open %s to serialize pipeline cache, missing cache folder or insufficient privileges might be the cause "
            "for this error",
      params.buildInCacheFileName);
    return;
  }

  WriteWrapper writer{cacheFile};
  writer.write(header);
  writer.write(ShaderHashValue::calculate(&header, 1));
  writer.writeBytes(blitOutputFormats.data(), sizeof(blitOutputFormats[0]) * blitOutputFormats.size());
  writer.writeBytes(clearOutputFormats.data(), sizeof(clearOutputFormats[0]) * clearOutputFormats.size());
  writer.writeBytes(buffer.data(), buffer.size());

  if (writer.close())
  {
    logdbg("DX12: Written build in pipeline cache to <%s>", params.buildInCacheFileName);
    logdbg("DX12: Blit formats: %u", header.blitCount);
    logdbg("DX12: Clear formats: %u", header.clearCount);
    ByteUnits librarySizeInBytes{header.librarySize};
    logdbg("DX12: Pipeline library size: %.4f %s", librarySizeInBytes.units(), librarySizeInBytes.name());
  }
  else
  {
    logwarn("DX12: Error while attempting to write the build in pipeline cache file to <%s>", params.buildInCacheFileName);
  }
}

void PipelineCache::preRecoveryBuildInPipelineLibrary()
{
  if (!buildInPipelineLibrary)
  {
    return;
  }

  DynamicArray<uint8_t> rescue;
  if (buildInUpdated)
  {
    logdbg("DX12: Attempting to rescue updated build in pipeline library binary");
    // attempt to rescue the changed shader library binary
    rescue.resize(buildInPipelineLibrary->GetSerializedSize());
    if (0 != rescue.size())
    {
      if (FAILED(buildInPipelineLibrary->Serialize(rescue.data(), rescue.size())))
      {
        rescue.resize(0);
      }
    }
    if (0 != rescue.size())
    {
      logdbg("DX12: Successfully rescued build in pipeline library binary");
    }
    else
    {
      logwarn("DX12: Failed to rescue build in pipeline library binary");
    }
  }
  buildInPipelineLibrary.Reset();
  if (0 != rescue.size())
  {
    loadedBuildInPipelineLibraryBlob = eastl::move(rescue);
  }
}

void PipelineCache::recoverBuildInPipelineLibrary(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS cache_support)
{
  if (0 == (cache_support & D3D12_SHADER_CACHE_SUPPORT_LIBRARY))
  {
    logdbg("DX12: Pipeline library disabled, not creating library for build in pipelines");
    // just in case the blob is not empty
    loadedBuildInPipelineLibraryBlob.resize(0);
    return;
  }

  if (0 != loadedBuildInPipelineLibraryBlob.size())
  {
    if (FAILED(device->CreatePipelineLibrary(loadedBuildInPipelineLibraryBlob.data(), loadedBuildInPipelineLibraryBlob.size(),
          COM_ARGS(&buildInPipelineLibrary))))
    {
      loadedBuildInPipelineLibraryBlob.resize(0);
      logwarn("DX12: Failed to restore pipeline library");
    }
    else
    {
      logdbg("DX12: Restored pipeline library");
    }
  }

  if (!buildInPipelineLibrary && buildInShouldRecord)
  {
    // load may not be able to create a restored pipeline library, so we have to create a new one when we need one
    if (SUCCEEDED(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&buildInPipelineLibrary))))
    {
      logdbg("DX12: Created new pipeline library for build in pipelines");
    }
    else
    {
      logwarn("DX12: Failed to create new pipeline library for build in pipelines");
    }
  }
}

void PipelineCache::shutdownBuildInPipelineLibrary()
{
  buildInPipelineLibrary.Reset();
  loadedBuildInPipelineLibraryBlob.resize(0);
}
#endif
