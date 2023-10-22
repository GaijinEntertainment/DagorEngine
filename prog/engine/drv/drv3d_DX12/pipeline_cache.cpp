#include "device.h"

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_finally.h>

using namespace drv3d_dx12;

#define DX12_ENABLE_CACHE_COMPRESSION 1

#if 0
#define CACHE_INFO_VERBOSE(...) debug(__VA_ARGS__)
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
};

enum CacheFileFlags
{
  CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES = 1u << 0,
};

constexpr uint32_t CACHE_FILE_MAGIC = _MAKE4C('CX12');
constexpr uint32_t CACHE_FILE_VERSION = 21;
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
// 17 - use of shader header inOutSemanticMask chaged, masking rules for color outputs of grpahics pipelines
//      have been updated and result in different masks and active render targets
// 18 - added support for view instancing
// 19 - mesh shader support, cache for mesh pipeline signatures
// 20 - independent blending support
// 21 - BasePipelineIdentifier only has hashes for vs and ps. Dropped hashes for gs, hs and ds as its no longer used.
//      Also changes pipeline library names.
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

  debug("PipelineCache features:");
  debug("Per PSO cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO) != 0);
  debug("Pipeline cache library: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0);
  debug("Automatic OS in memory cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE) != 0);
  debug("Automatic OS on disk cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE) != 0);

  library.Reset();

  loadFromFile(params);
  if (!library && useLibrary())
  {
    // just report an error but don't abort, library is not essential
    DX12_DEBUG_RESULT(params.device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&library)));
  }
#endif
}

struct MemoryWriteBuffer
{
  eastl::vector<uint8_t> buf;
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

  if (!hasChanged)
    return;

  dd_mkpath(params.fileName);

  FullFileSaveCB cacheFile{params.fileName, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING};
  if (!cacheFile.fileHandle)
  {
    logerr("DX12: Failed to open %s to serialize pipeline cache, missing cache folder or "
           "insufficient privileges might be the cause for this error",
      params.fileName);
    return;
  }

  debug("DX12: Serializing pipeline caches to %s", params.fileName);

  CacheFileHeader fileHeader = {};
  fileHeader.magic = CACHE_FILE_MAGIC;
  fileHeader.version = CACHE_FILE_VERSION;
  fileHeader.pointerSize = EXPECTED_POINTER_SIZE;
  fileHeader.deviceVendorID = params.deviceVendorID;
  fileHeader.deviceID = params.deviceID;
  fileHeader.subSystemID = params.subSystemID;
  fileHeader.revisionID = params.revisionID;
  fileHeader.computeCaches = static_cast<uint32_t>(computeBlobs.size());
  fileHeader.graphicsBaseCaches = static_cast<uint32_t>(graphicsCache.size());
  fileHeader.computeSignatureCaches = static_cast<uint32_t>(computeSignatures.size());
  fileHeader.graphicsSignatureCaches = static_cast<uint32_t>(graphicsSignatures.size());
  fileHeader.graphicsMeshSignatureCaches = static_cast<uint32_t>(graphicsMeshSignatures.size());
  fileHeader.graphicsStaticStateCaches = static_cast<uint32_t>(staticRenderStates.size());
  fileHeader.inputLayoutCaches = static_cast<uint32_t>(inputLayouts.size());
  fileHeader.framebufferLayoutCaches = static_cast<uint32_t>(framebufferLayouts.size());
  fileHeader.flags = 0;
  if (params.rootSignaturesUsesCBVDescriptorRanges)
  {
    fileHeader.flags |= CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES;
  }
  fileHeader.rawDriverVersion = params.rawDriverVersion;
  if (library)
  {
    fileHeader.librarySize = static_cast<uint32_t>(library->GetSerializedSize());
  }

  MemoryWriteBuffer mem;
  for (auto &&cc : computeBlobs)
  {
    mem.append(&cc.hash, sizeof(cc.hash));
    auto s = static_cast<uint32_t>(cc.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(cc.blob.data(), cc.blob.size());
  }

  for (auto &&gc : graphicsCache)
  {
    mem.append(&gc.ident, sizeof(gc.ident));
    auto s = static_cast<uint32_t>(gc.variantCache.size());
    mem.append(&s, sizeof(s));
    for (auto &&v : gc.variantCache)
    {
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
    mem.append(&cs.def, sizeof(cs.def));
    auto s = static_cast<uint32_t>(cs.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(cs.blob.data(), cs.blob.size());
  }

  for (auto &&gs : graphicsSignatures)
  {
    mem.append(&gs.def, sizeof(gs.def));
    auto s = static_cast<uint32_t>(gs.blob.size());
    mem.append(&s, sizeof(s));
    mem.append(gs.blob.data(), gs.blob.size());
  }

  for (auto &&gs : graphicsMeshSignatures)
  {
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
    eastl::vector<uint8_t> blob;
    blob.resize(library->GetSerializedSize());
    library->Serialize(blob.data(), blob.size());
    mem.append(blob.data(), blob.size());
  }

  eastl::vector<uint8_t> compressedMemory;
#if DX12_ENABLE_CACHE_COMPRESSION
  compressedMemory.resize(mem.buf.size() * 2);
  auto compSize = zstd_compress(compressedMemory.data(), compressedMemory.size(), mem.buf.data(), mem.buf.size(), 20);
  // if we encountered an error or for some reason the data is not smaller,
  // use uncompressed instead
  if (compSize >= mem.buf.size())
    compSize = 0;
  compressedMemory.resize(compSize);
#endif

  fileHeader.compressedContentSize = static_cast<uint32_t>(compressedMemory.size());
  fileHeader.uncompressedContentSize = static_cast<uint32_t>(mem.buf.size());
  fileHeader.dataChecksum = dxil::HashValue::calculate(mem.buf.data(), mem.buf.size());
  fileHeader.headerChecksum = dxil::HashValue::calculate(&fileHeader, 1);

  debug("D12: Writing pipeline cache with %u bytes of size",
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
  auto ref =
    eastl::find_if(begin(graphicsCache), end(graphicsCache), [=](const GraphicsPipeline &pipe) { return pipe.ident == ident; });

  return ref != end(graphicsCache);
}

size_t PipelineCache::getGraphicsPipelineVariantCount(GraphicsPipelineBaseCacheId base_id)
{
  return graphicsCache[base_id.get()].variantCache.size();
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

size_t PipelineCache::removeGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id, size_t index)
{
  auto &base = graphicsCache[base_id.get()];
  auto &variants = base.variantCache;
  variants.erase(begin(variants) + index);
  hasChanged = true;
  // TODO: This does not queue regeneration of the pipeline library, as it has no remove functionality.
  //       So to remove one, we have to create a new empty pipeline library, and migrate all still existing
  //       entries over, which may be very time consuming. Better approach is possibly to not store the
  //       library in the cache and start with a empty one on next run.
  return variants.size();
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

D3D12_PRIMITIVE_TOPOLOGY_TYPE
PipelineCache::getGraphicsPipelineVariantDesc(GraphicsPipelineBaseCacheId base_id, size_t index, InputLayout &input_layout,
  bool &is_wire_frame, RenderStateSystem::StaticState &static_state, FramebufferLayout &fb_layout) const
{
  auto &base = graphicsCache[base_id.get()];
  auto &variant = base.variantCache[index];
  fb_layout = framebufferLayouts[variant.framebufferLayoutIndex];
  static_state = staticRenderStates[variant.staticRenderStateIndex];
  input_layout = inputLayouts[variant.inputLayoutIndex];
  is_wire_frame = variant.isWireFrame != 0;
  return variant.topology;
}

ComPtr<ID3D12PipelineState> PipelineCache::loadGraphicsPipelineVariantFromIndex(GraphicsPipelineBaseCacheId base_id, size_t index,
  D3D12_PIPELINE_STATE_STREAM_DESC desc, D3D12_CACHED_PIPELINE_STATE &blob_target)
{
  G_UNUSED(base_id);
  G_UNUSED(index);
  G_UNUSED(desc);
  G_UNUSED(blob_target);
#if _TARGET_PC_WIN
  ComPtr<ID3D12PipelineState> result;
  auto &base = graphicsCache[base_id.get()];
  auto &variant = base.variantCache[index];
  if (usePSOBlobs())
  {
    if (!variant.blob.empty())
    {
      blob_target.pCachedBlob = variant.blob.data();
      blob_target.CachedBlobSizeInBytes = variant.blob.size();
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache hit");
    }
    else
    {
      CACHE_INFO_VERBOSE("DX12: Graphics pipeline cache miss (no blob)");
    }
  }
  else if (library)
  {
    GraphicsPipelineVariantName name;
    name.generate(base.ident, variant.topology, variant.inputLayoutIndex, variant.isWireFrame != 0, variant.staticRenderStateIndex,
      variant.framebufferLayoutIndex);
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

void PipelineCache::addCompute(const dxil::HashValue &shader, ID3D12PipelineState *pipeline)
{
  G_UNUSED(shader);
  G_UNUSED(pipeline);
#if _TARGET_PC_WIN
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
  eastl::vector<uint8_t> buf;
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
    eastl::vector<uint8_t> newBlob;
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
#endif
}

void PipelineCache::recover(ID3D12Device1 *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes)
{
#if _TARGET_PC_WIN
  D3D12_FEATURE_DATA_SHADER_CACHE cacheFlags = {};
  if (DX12_CHECK_OK(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &cacheFlags, sizeof(cacheFlags))))
  {
    deviceFeatures = cacheFlags.SupportFlags & allowed_modes;
  }

  debug("PipelineCache features:");
  debug("Per PSO cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO) != 0);
  debug("Pipeline cache library: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0);
  debug("Automatic OS in memory cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE) != 0);
  debug("Automatic OS on disk cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE) != 0);

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
#else
  G_UNUSED(device);
  G_UNUSED(allowed_modes);
#endif
}

bool PipelineCache::loadFromFile(const SetupParameters &params)
{
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
    logerr("DX12: Pipeline cache file is invalid");
    return false;
  }

  if (fileHeader.version != CACHE_FILE_VERSION)
  {
    // begin outdated is okay.
    debug("DX12: Pipeline cache file is outdated");
    return false;
  }

  if ((0 != (fileHeader.flags & CFF_ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES)) != params.rootSignaturesUsesCBVDescriptorRanges)
  {
    // If used root signature mode does not match, we can not reuse cache
    debug("DX12: Pipeline incompatible root signature layout mode (CBV root descriptors vs "
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
    logerr("DX12: Pipeline cache file is corrupted");
    return false;
  }

  MemoryReadBuffer data;
  data.buf.resize(fileHeader.uncompressedContentSize);
  if (fileHeader.compressedContentSize)
  {
    eastl::vector<uint8_t> compressedData;
    compressedData.resize(fileHeader.compressedContentSize);
    ln = cacheFile.tryRead(compressedData.data(), compressedData.size());
    if (ln != compressedData.size())
    {
      logerr("DX12: Error while reading data");
      return false;
    }
    auto outSize = zstd_decompress(data.buf.data(), data.buf.size(), compressedData.data(), compressedData.size());
    if (outSize != data.buf.size())
    {
      logerr("DX12: Error while decompressing data");
      return false;
    }
  }
  else
  {
    ln = cacheFile.tryRead(data.buf.data(), data.buf.size());
    if (ln != data.buf.size())
    {
      logerr("DX12: Error while reading data");
      return false;
    }
  }

  if (fileHeader.dataChecksum != dxil::HashValue::calculate(data.buf.data(), data.buf.size()))
  {
    logerr("DX12: Pipeline cache file is corrupted");
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
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      return false;
    }
    uint32_t sz = 0;
    if (!data.read(sz))
    {
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      return false;
    }
    auto pso = data.readRange<uint8_t>(sz);
    if (!pso)
    {
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      return false;
    }
    uint32_t sz = 0;
    if (!data.read(sz))
    {
      logerr("DX12: Error while decoding pipeline cache");
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
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.framebufferLayoutIndex))
      {
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.inputLayoutIndex))
      {
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.isWireFrame))
      {
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      if (!data.read(vce.staticRenderStateIndex))
      {
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      uint32_t psoSz = 0;
      if (!data.read(psoSz))
      {
        logerr("DX12: Error while decoding pipeline cache");
        computeBlobs.clear();
        graphicsCache.clear();
        return false;
      }
      auto pso = data.readRange<uint8_t>(psoSz);
      if (!pso)
      {
        logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      return false;
    }
    uint32_t sigSz = 0;
    if (!data.read(sigSz))
    {
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      return false;
    }
    auto sig = data.readRange<uint8_t>(sigSz);
    if (!sig)
    {
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      return false;
    }
    uint32_t sigSz = 0;
    if (!data.read(sigSz))
    {
      logerr("DX12: Error while decoding pipeline cache");
      computeBlobs.clear();
      graphicsCache.clear();
      computeSignatures.clear();
      graphicsSignatures.clear();
      return false;
    }
    auto sig = data.readRange<uint8_t>(sigSz);
    if (!sig)
    {
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding pipeline cache");
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
      logerr("DX12: Error while decoding static render state cache");
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
      logerr("DX12: Error while decoding input layout cache");
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
      logerr("DX12: Error while decoding framebuffer layout cache");
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

  debug("DX12: Pipeline cache load statistics:");
  debug("DX12: Restored %u compute pipelines with %u bytes from cache", computeBlobsLoaded, computeBlobsSize);
  debug("DX12: Restored %u compute pipeline signatures with %u bytes from cache", fileHeader.computeSignatureCaches,
    computeSignatureBlobSize);
  debug("DX12: Restored %u graphics pipeline variants of %u (%u) base pipelines", graphicsVairantsFound, graphicsWithAnyVariant,
    fileHeader.graphicsBaseCaches);
  debug("DX12: Restored %u graphics pipelines with %u bytes from cache", graphicsVariantBlobsLoaded, graphicsVariantBlobsSize);
  debug("DX12: Restored %u graphics pipeline signatures with %u bytes from cache", fileHeader.graphicsSignatureCaches,
    graphicsSignatureBlobSize);
  debug("DX12: Restored %u graphics mesh pipeline signatures with %u bytes from cache", fileHeader.graphicsMeshSignatureCaches,
    graphicsMeshSignatureBlobSize);
  debug("DX12: Restored %u graphics pipeline static render states", staticRenderStates.size());
  debug("DX12: Restored %u graphics pipeline input layouts", inputLayouts.size());
  debug("DX12: Restored %u graphics pipeline framebuffer layouts", framebufferLayouts.size());
  debug("DX12: Restored pipeline library with %u bytes from cache", initialPipelineBlob.size());

  hasChanged = false;

  return true;
}
