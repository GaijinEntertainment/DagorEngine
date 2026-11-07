// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline_cache.h"
#include "d3d12_error_handling.h"
#include "variant_vector.h"

#include <util/dag_base85.h>
#include <3d/gpu_pipeline_serialization.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_addressWait.h>


using namespace drv3d_dx12;


#if _TARGET_PC_WIN
namespace
{
struct UngroupedPipelineLibraryFileHeader
{
  // (U)n(g)rouped (P)ipeline (L)ibrary
  static constexpr uint32_t magic_value = 'UgPL';
  // Versions:
  // 1: Initial version
  //    - Support for dedicated hash arrays for vertex, pixel and compute shaders
  //    - Support for pipeline library to store all pipelines
  //    - Integrity of vertex, pixel and compute shader hash arrays
  //    - Layout is: FileHeader (this type), hash of FileHeader as ShaderHashValue, vertex shader hash array, pixel shader hash
  //      array, compute shader hash array, shader library blob.
  static constexpr uint32_t version_value = 1;

  // Header identifier, hash to match magic_value.
  uint32_t magic;
  // File version, hash to match version_value.
  uint32_t version;
  ShaderHashValue vertexShaderArrayHash;
  ShaderHashValue pixelShaderArrayHash;
  ShaderHashValue computeShaderArrayHash;
  uint32_t vertexShaderArraySize;
  uint32_t pixelShaderArraySize;
  uint32_t computeShaderArraySize;
  uint32_t librarySize;
  uint64_t runIndex;
};

struct ShaderDumpPipelineLibraryFileHeader
{
  // (S)hader (D)ump (P)ipeline (L)ibrary
  static constexpr uint32_t magic_value = 'SDPL';
  // Versions:
  // 1: Initial version
  //    - Support for dedicated hash arrays for vertex, pixel and compute shaders
  //    - Support for pipeline library to store all pipelines
  //    - Integrity of vertex, pixel and compute shader hash arrays
  //    - Layout is: FileHeader (this type), hash of FileHeader as ShaderHashValue, vertex shader hash array, pixel shader hash
  //      array, compute shader hash array, shader library blob.
  static constexpr uint32_t version_value = 1;
  // Header identifier, hash to match magic_value.
  uint32_t magic;
  // File version, hash to match version_value.
  uint32_t version;
  ShaderHashValue vertexShaderHashArrayHash;
  ShaderHashValue pixelShaderHashArrayHash;
  ShaderHashValue computeShaderHashArrayHash;
  uint32_t vertexShaderHashArraySize;
  uint32_t pixelShaderHashArraySize;
  uint32_t computeShaderHashArraySize;
  uint32_t librarySize;
};

struct RootSignatureProperties
{
  bool usesConstBufferRootDescriptors = false;
};

struct GroupPipelineNameStore
{
  eastl::wstring name;

  GroupPipelineNameStore(const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const RootSignatureProperties &properties)
  {
    const auto *bytes = reinterpret_cast<const uint8_t *>(&properties);
    auto blob = serialize_pipeline({.shaderHashes = true}, desc, {bytes, sizeof(properties)});

    name.reserve(calculate_max_base_85_string_length(blob.size()));
    eastl::span<const uint8_t> data{blob};
    while (data.size() >= 4)
    {
      auto encodedBlock = encode_32_bit_word_to_base_85_block(uint32_t(data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
      for (auto c : encodedBlock.data)
      {
        if ('\0' == c)
        {
          break;
        }
        name += static_cast<wchar_t>(c);
      }
      data = data.subspan(4);
    }
    if (!data.empty())
    {
      uint32_t word = data[0];
      if (data.size() > 1)
      {
        word |= uint32_t(data[1]) << 8;
      }
      if (data.size() > 2)
      {
        word |= uint32_t(data[2]) << 16;
      }
      // can not be > 3
      auto encodedBlock = encode_32_bit_word_to_base_85_block(word);
      for (auto c : encodedBlock.data)
      {
        if ('\0' == c)
        {
          break;
        }
        name += static_cast<wchar_t>(c);
      }
    }
  }
};

// Generates a cache file name from a dump file name, the dump file name has usually a sub directory (so its a relative path).
// The generator takes the name and "normalizes" it by converting all non alpha numeric characters into a underscore, this
// ensures that the name will be ok for the file system. The name gets prefixed with "cache/", the relative cache folder
// and post fixed with the extension ".cache".
struct GroupCacheFileName
{
  eastl::string name;
  GroupCacheFileName(eastl::string_view dump_name)
  {
    // -2 because sizeof of the pre and post fix include the null terminator each
    name.reserve(sizeof(DUMP_CACHE_FILE_PREFIX) + sizeof(DUMP_CACHE_FILE_POSTFIX) + dump_name.length() - 2);
    name = DUMP_CACHE_FILE_PREFIX;
    for (auto c : dump_name)
    {
      if (isalnum(static_cast<unsigned char>(c)))
      {
        name += c;
      }
      else
      {
        name += '_';
      }
    }
    name += DUMP_CACHE_FILE_POSTFIX;
  }
};

enum class ReaderWrapperErrorCode
{
  GenericReadError,
};

template <typename T>
class ReadWrapper
{
  T &source;

public:
  ReadWrapper(T &t) : source{t} {}
  using ErrorCode = ReaderWrapperErrorCode;
  dag::Expected<void, ErrorCode> readBytes(void *ptr, size_t sz)
  {
    if (sz != source.tryRead(ptr, sz))
    {
      return dag::Unexpected{ErrorCode::GenericReadError};
    }
    return {};
  }
  template <typename U>
  dag::Expected<void, ErrorCode> readArray(U *ptr, size_t cnt)
  {
    return readBytes(ptr, sizeof(U) * cnt);
  }
  template <typename U>
  dag::Expected<U, ErrorCode> readValue()
  {
    U value;
    return readBytes(&value, sizeof(U)).and_then([&]() -> dag::Expected<U, ErrorCode> { return eastl::move(value); });
  }
  template <typename U>
  dag::Expected<U, ErrorCode> readContainer(uint32_t count)
  {
    U data;
    data.resize(count);
    return readArray(data.data(), data.size()).and_then([&]() -> dag::Expected<U, ErrorCode> { return eastl::move(data); });
  }
};

template <typename T>
class WriteWrapper
{
  T &target;
  bool ok = true;

public:
  WriteWrapper(T &t) : target{t} {}
  void writeBytes(const void *ptr, size_t sz) { ok = ok && ((0 == sz) || ((nullptr != ptr) && (sz == target.tryWrite(ptr, sz)))); }
  template <typename U>
  void writeArray(const U *ptr, size_t count)
  {
    writeBytes(ptr, sizeof(U) * count);
  }
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

bool load_shader_cache_for_group(const auto &group, eastl::string_view dump_name)
{
  GroupCacheFileName fileName{dump_name};
  FullFileLoadCB cacheFile{fileName.name.c_str(), DF_IGNORE_MISSING | DF_READ};
  if (!cacheFile.fileHandle)
  {
    logwarn("DX12: Failed to open <%s> to load shader dump pipeline cache", fileName.name);
    return false;
  }

  ReadWrapper reader{cacheFile};
  auto headerReadResult = reader.readValue<ShaderDumpPipelineLibraryFileHeader>();
  if (!headerReadResult)
  {
    logwarn("DX12: Failed to read <%s> file header", fileName.name);
    return false;
  }
  auto &header = headerReadResult.value();

  if ((ShaderDumpPipelineLibraryFileHeader::magic_value != header.magic) ||
      (ShaderDumpPipelineLibraryFileHeader::version_value != header.version))
  {
    logwarn("DX12: File header <%s> incompatible", fileName.name);
    return false;
  }

  auto headerHashReadResult = reader.readValue<ShaderHashValue>();
  if (!headerHashReadResult)
  {
    logwarn("DX12: Failed to read <%s> file header hash", fileName.name);
    return false;
  }
  if (ShaderHashValue::calculate(&headerReadResult.value(), 1) != headerHashReadResult.value())
  {
    logwarn("DX12: File header hash <%s> not matching", fileName.name);
    return false;
  }

  auto vertexShaderHashArrayReadResult = reader.readContainer<DynamicArray<dxil::HashValue>>(header.vertexShaderHashArraySize);
  if (!vertexShaderHashArrayReadResult)
  {
    logwarn("DX12: Failed to read <%s> vertex shader hash array", fileName.name);
    return false;
  }
  if (ShaderHashValue::calculate(vertexShaderHashArrayReadResult->data(), vertexShaderHashArrayReadResult->size()) !=
      header.vertexShaderHashArrayHash)
  {
    logwarn("DX12: Vertex shader hash array hash <%s> not matching", fileName.name);
    return false;
  }

  auto pixelShaderHashArrayReadResult = reader.readContainer<DynamicArray<dxil::HashValue>>(header.pixelShaderHashArraySize);
  if (!pixelShaderHashArrayReadResult)
  {
    logwarn("DX12: Failed to read <%s> pixel shader hash array", fileName.name);
    return false;
  }
  if (ShaderHashValue::calculate(pixelShaderHashArrayReadResult->data(), pixelShaderHashArrayReadResult->size()) !=
      header.pixelShaderHashArrayHash)
  {
    logwarn("DX12: Pixel shader hash array hash <%s> not matching", fileName.name);
    return false;
  }

  auto computeShaderHashArrayReadResult = reader.readContainer<DynamicArray<dxil::HashValue>>(header.computeShaderHashArraySize);
  if (!computeShaderHashArrayReadResult)
  {
    logwarn("DX12: Failed to read <%s> compute shader hash array", fileName.name);
    return false;
  }
  if (ShaderHashValue::calculate(computeShaderHashArrayReadResult->data(), computeShaderHashArrayReadResult->size()) !=
      header.computeShaderHashArrayHash)
  {
    logwarn("DX12: Compute shader hash array hash <%s> not matching", fileName.name);
    return false;
  }

  auto libraryBlobReadResult = reader.readContainer<DynamicArray<uint8_t>>(header.librarySize);
  if (!libraryBlobReadResult)
  {
    logwarn("DX12: Failed to read <%s> pipeline library blob", fileName.name);
    return false;
  }

  group->constantState.libraryBlob = eastl::move(libraryBlobReadResult.value());
  group->constantState.vertexShaderSet.insert(vertexShaderHashArrayReadResult->begin(), vertexShaderHashArrayReadResult->end());
  group->constantState.pixelShaderSet.insert(pixelShaderHashArrayReadResult->begin(), pixelShaderHashArrayReadResult->end());
  group->constantState.computeShaderSet.insert(computeShaderHashArrayReadResult->begin(), computeShaderHashArrayReadResult->end());
  return true;
}

void store_cache_for_pipeline_group(const auto &group, eastl::string_view dump_name, dag::AtomicInteger<uint32_t> &issue_counter,
  dag::AtomicInteger<uint32_t> &pending_counter)
{
  if (!group->hasLibrary())
  {
    return;
  }

  if (!group->hasChanged())
  {
    logdbg("DX12: No changes for group for <%s>, skipping store", dump_name);
    return;
  }

  auto libSerializeResult = group->serializeLibrary();
  if (!libSerializeResult)
  {
    logwarn("DX12: Failed to obtain pipeline library blob for <%s>: %s", dump_name,
      dxgi_error_code_to_string(libSerializeResult.error()));
    return;
  }

  auto &libraryBlob = libSerializeResult.value();
  auto vertexShaderHashArray = group->serializeVertexShaderInfos();
  auto pixelShaderHashArray = group->serializePixelShaderInfo();
  auto computeShaderHashArray = group->serializeComputeShaderInfo();

  const ShaderDumpPipelineLibraryFileHeader header{
    .magic = ShaderDumpPipelineLibraryFileHeader::magic_value,
    .version = ShaderDumpPipelineLibraryFileHeader::version_value,
    .vertexShaderHashArrayHash = ShaderHashValue::calculate(vertexShaderHashArray.data(), vertexShaderHashArray.size()),
    .pixelShaderHashArrayHash = ShaderHashValue::calculate(pixelShaderHashArray.data(), pixelShaderHashArray.size()),
    .computeShaderHashArrayHash = ShaderHashValue::calculate(computeShaderHashArray.data(), computeShaderHashArray.size()),
    .vertexShaderHashArraySize = static_cast<uint32_t>(vertexShaderHashArray.size()),
    .pixelShaderHashArraySize = static_cast<uint32_t>(pixelShaderHashArray.size()),
    .computeShaderHashArraySize = static_cast<uint32_t>(computeShaderHashArray.size()),
    .librarySize = static_cast<uint32_t>(libraryBlob.size()),
  };

  // all those size checks are unlikely to happen, if they hit those limits its most likely an indicator that something is broken so
  // still log them
  if (uint64_t{header.vertexShaderHashArraySize} != vertexShaderHashArray.size())
  {
    logwarn("DX12: Too many vertex shader hashes to store for <%s>", dump_name);
    return;
  }
  if (uint64_t{header.pixelShaderHashArraySize} != pixelShaderHashArray.size())
  {
    logwarn("DX12: Too many pixel shader hashes to store for <%s>", dump_name);
    return;
  }
  if (uint64_t{header.computeShaderHashArraySize} != computeShaderHashArray.size())
  {
    logwarn("DX12: Too many compute shader hashes to store for <%s>", dump_name);
    return;
  }
  if (uint64_t{header.librarySize} != libraryBlob.size())
  {
    logwarn("DX12: Pipeline library blob too big to store for <%s>", dump_name);
    return;
  }

  GroupCacheFileName fileName{dump_name};
  issue_counter.fetch_add(1);
  execute_in_new_thread(
    [fileName, header, vsha = eastl::move(vertexShaderHashArray), psha = eastl::move(pixelShaderHashArray),
      csha = eastl::move(computeShaderHashArray), lb = eastl::move(libraryBlob), &issue_counter, &pending_counter](auto) {
      pending_counter.fetch_add(1);
      issue_counter.fetch_sub(1);
      ::notify_one(issue_counter);

      dd_mkpath(fileName.name.c_str());
      FullFileSaveCB cacheFile{fileName.name.c_str(), DF_WRITE | DF_CREATE | DF_IGNORE_MISSING};
      if (cacheFile.fileHandle)
      {
        auto headerHash = ShaderHashValue::calculate(&header, 1);

        WriteWrapper writer{cacheFile};
        writer.write(header);
        writer.write(headerHash);
        writer.writeArray(vsha.data(), vsha.size());
        writer.writeArray(psha.data(), psha.size());
        writer.writeArray(csha.data(), csha.size());
        writer.writeArray(lb.data(), lb.size());

        if (writer.close())
        {
          logdbg("DX12: Written shader dump group cache file <%s>", fileName.name);
          logdbg("DX12: vertex shader hash count: %u", header.vertexShaderHashArraySize);
          logdbg("DX12: pixel shader hash count: %u", header.pixelShaderHashArraySize);
          logdbg("DX12: compute shader hash count: %u", header.computeShaderHashArraySize);
          ByteUnits librarySizeInBytes{header.librarySize};
          logdbg("DX12: Pipeline library size: %.4f %s", librarySizeInBytes.units(), librarySizeInBytes.name());
        }
        else
        {
          logwarn("DX12: Error while attempting to write shader dump group cache file to <%s>", fileName.name);
        }
      }
      else
      {
        logwarn(
          "DX12: Failed to open <%s> to serialize pipeline cache, missing cache folder or insufficient privileges might be the cause "
          "for this error",
          fileName.name);
      }
      pending_counter.fetch_sub(1);
      ::notify_one(pending_counter);
    },
    "DX12: Pipeline group cache write");
}

dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> load_graphics_pipeline_variant_from_store(auto &store,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident, const RootSignatureProperties &properties,
  auto &&...write_params)
{
  auto access = store.access();
  if (!access->hasLibrary())
  {
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }

  auto vsInCacheLoc = access->hasVertexShader(ident.vs);
  auto psInCacheLoc = access->hasPixelShader(ident.ps);

  GroupPipelineNameStore name{desc, properties};
  HRESULT errorCode = E_INVALIDARG;
  ComPtr<ID3D12PipelineState> pipeline;

  // TODO this can be improved
  if (decltype(vsInCacheLoc)::Missing != vsInCacheLoc && decltype(psInCacheLoc)::Missing != psInCacheLoc)
  {
    errorCode = access->constantState.library->LoadPipeline(name.name.c_str(), &desc, COM_ARGS(&pipeline));
    if (pipeline)
    {
      // store may track data that needs to rewritten on load
      if (access->needsLoadRewrite())
      {
        access->rewriteVertexShaderInfo(ident.vs, eastl::move(write_params)...);
        access->rewritePixelShaderInfo(ident.ps, eastl::move(write_params)...);
      }

      if (access->isInReplacementMode())
      {
        auto errorCode = access->dynamicState.library->StorePipeline(name.name.c_str(), pipeline.Get());
        if (SUCCEEDED(errorCode))
        {
          access->replacementVertexShaderUse(ident.vs);
          access->replacementPixelShaderUse(ident.ps);
        }
        else
        {
          logwarn("DX12: Store into replacement library failed: %s", dxgi_error_code_to_string(errorCode));
        }
      }
    }
  }

  if (!pipeline && access->isInReplacementMode() &&
      (decltype(vsInCacheLoc)::InDynamicStore == vsInCacheLoc && decltype(psInCacheLoc)::InDynamicStore == psInCacheLoc))
  {
    errorCode = access->dynamicState.library->LoadPipeline(name.name.c_str(), &desc, COM_ARGS(&pipeline));
    // when it comes of dynamic store, the cache entries are already up to date and no extra update is needed
  }

  if (!pipeline)
  {
    return dag::Unexpected<HRESULT>{errorCode};
  }

  return eastl::move(pipeline);
}

dag::Expected<void, HRESULT> add_graphics_pipeline_vairiant_to_store(auto &store, ID3D12PipelineState *pipeline,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident, const RootSignatureProperties &properties,
  auto &&...add_params)
{
  auto access = store.access();
  if (!access->hasLibrary())
  {
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }
  GroupPipelineNameStore name{desc, properties};
  auto errorCode = access->dynamicState.library->StorePipeline(name.name.c_str(), pipeline);
  if (FAILED(errorCode))
  {
    return dag::Unexpected{errorCode};
  }

  access->addVertexShaderInfo(ident.vs, eastl::move(add_params)...);
  access->addPixelShaderInfo(ident.ps, eastl::move(add_params)...);
  return {};
}

dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> load_compute_pipeline_from_store(auto &store,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier, const RootSignatureProperties &properties,
  auto &&...write_params)
{
  auto access = store.access();
  if (!access->hasLibrary())
  {
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }

  GroupPipelineNameStore name{desc, properties};
  HRESULT errorCode = E_INVALIDARG;
  ComPtr<ID3D12PipelineState> pipeline;

  auto csInCacheLoc = access->hasComputeShader(shader_identifier);
  // TODO this can be improved
  if (decltype(csInCacheLoc)::Missing != csInCacheLoc)
  {
    errorCode = access->constantState.library->LoadPipeline(name.name.c_str(), &desc, COM_ARGS(&pipeline));

    if (pipeline)
    {
      if (access->needsLoadRewrite())
      {
        access->rewriteComputeShaderInfo(shader_identifier, eastl::move(write_params)...);
      }

      if (access->isInReplacementMode())
      {
        auto errorCode = access->dynamicState.library->StorePipeline(name.name.c_str(), pipeline.Get());
        if (SUCCEEDED(errorCode))
        {
          access->replacementComputeShaderUse(shader_identifier);
        }
        else
        {
          logwarn("DX12: Store into replacement library failed: %s", dxgi_error_code_to_string(errorCode));
        }
      }
    }
  }

  if (!pipeline && access->isInReplacementMode() && decltype(csInCacheLoc)::InDynamicStore == csInCacheLoc)
  {
    errorCode = access->dynamicState.library->LoadPipeline(name.name.c_str(), &desc, COM_ARGS(&pipeline));
    // when it comes of dynamic store, the cache entries are already up to date and no extra update is needed
  }

  if (!pipeline)
  {
    return dag::Unexpected<HRESULT>{errorCode};
  }

  return eastl::move(pipeline);
}

dag::Expected<void, HRESULT> add_compute_to_store(auto &store, ID3D12PipelineState *pipeline,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier, const RootSignatureProperties &properties,
  auto &&...add_params)
{
  auto access = store.access();
  if (!access->hasLibrary())
  {
    return dag::Unexpected<HRESULT>{E_INVALIDARG};
  }
  GroupPipelineNameStore name{desc, properties};
  auto errorCode = access->dynamicState.library->StorePipeline(name.name.c_str(), pipeline);
  if (FAILED(errorCode))
  {
    return dag::Unexpected{errorCode};
  }

  access->addComputeShaderInfo(shader_identifier, eastl::move(add_params)...);
  return {};
}

bool has_base_pipeline_in_store(auto &store, const BasePipelineIdentifier &ident)
{
  auto readAccess = store.access();
  auto vsResult = readAccess->hasVertexShader(ident.vs);
  auto psResult = readAccess->hasPixelShader(ident.ps);
  return decltype(vsResult)::Missing != vsResult && decltype(psResult)::Missing != psResult;
}

bool has_compute_pipeline_in_store(auto &store, const dxil::HashValue &shader_identifier)
{
  auto readAccess = store.access();
  auto csResult = readAccess->hasComputeShader(shader_identifier);
  return decltype(csResult)::Missing != csResult;
}

bool create_default_libraries(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS features, auto &store)
{
  if (0 == (D3D12_SHADER_CACHE_SUPPORT_LIBRARY & features))
  {
    return true;
  }
  if (store->hasLibrary())
  {
    return true;
  }
  ComPtr<ID3D12PipelineLibrary1> loadedLib;
  if (FAILED(DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&loadedLib)))))
  {
    return false;
  }
  if (!store->constantState.library)
  {
    store->constantState.library = loadedLib;
  }
  if (!store->dynamicState.library)
  {
    store->dynamicState.library = loadedLib;
  }
  return true;
}

bool recover_for_store(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS features, const auto &store, const eastl::string_view &what)
{
  return store->recover(device, features)
    .transform([&](auto from_blob_result) {
      if (FAILED(from_blob_result))
      {
        logwarn("DX12: Failed to restore pipeline library for <%s> pipelines: %s", what, dxgi_error_code_to_string(from_blob_result));
      }
      else if (S_OK == from_blob_result)
      {
        logdbg("DX12: Restored pipeline library for <%s> pipelines", what);
      }
      else
      {
        logdbg("DX12: New pipeline library for <%s> pipelines", what);
      }
      return true;
    })
    .transform_error([&](auto error_code) {
      if (E_NOINTERFACE != error_code)
      {
        logwarn("DX12: Failed to create new pipeline library for <%s> pipelines: %s", what, dxgi_error_code_to_string(error_code));
      }
      return error_code;
    })
    .value_or(false);
}

bool setup_for_group(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS features, const auto &group, const eastl::string_view &what,
  eastl::span<const dxil::HashValue> all_shader_hashes)
{
  if (!recover_for_store(device, features, group, what))
  {
    return false;
  }

  uint32_t vsHit = 0;
  uint32_t psHit = 0;
  uint32_t csHit = 0;
  uint32_t miss = 0;
  for (auto &hash : all_shader_hashes)
  {
    if (0 < group->constantState.vertexShaderSet.count(hash))
    {
      ++vsHit;
    }
    else if (0 < group->constantState.pixelShaderSet.count(hash))
    {
      ++psHit;
    }
    else if (0 < group->constantState.computeShaderSet.count(hash))
    {
      ++csHit;
    }
    else
    {
      ++miss;
    }
  }
  logdbg("DX12: Disk cache compare to shader dump loaded:");
  logdbg("DX12: Disk cache has %u vertex shaders not in shader dump", group->constantState.vertexShaderSet.size() - vsHit);
  logdbg("DX12: Disk cache has %u pixel shaders not in shader dump (at least one will always not be in shader dump, because of 'null "
         "pixel shader')",
    group->constantState.pixelShaderSet.size() - psHit);
  logdbg("DX12: Disk cache has %u compute shaders not in shader dump", group->constantState.computeShaderSet.size() - csHit);
  logdbg("DX12: Disk cache does not cover %u shaders found in shader dump", miss);
  const uint32_t totalHit = vsHit + psHit + csHit;
  const auto totalSize = group->constantState.vertexShaderSet.size() + group->constantState.pixelShaderSet.size() +
                         group->constantState.computeShaderSet.size();
  // when less than 75% of all shader hashes are still part of the dump, we start the replacement process into a new lib
  constexpr uint32_t lib_replacement_threshold_rate = 75;
  if (totalSize && (totalHit * 100 / totalSize < lib_replacement_threshold_rate))
  {
    ComPtr<ID3D12PipelineLibrary1> replacementLib;
    if (SUCCEEDED(DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&replacementLib)))))
    {
      group->dynamicState.library = replacementLib;
      logdbg("DX12: Disk cache has %u%% of shaders in cache that are not part of the shader dump, starting replacement of pipeline "
             "library",
        100 - totalHit * 100 / totalSize);
    }
  }
  else
  {
    logdbg("DX12: Using disk cache as is");
  }

  return true;
}

bool recover_ungrouped_pipeline_library(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS features, const auto &write_access,
  const eastl::string_view &what, uint64_t ungrouped_pipelines_run_index)
{
  if (!recover_for_store(device, features, write_access, what))
  {
    return false;
  }

  // assumes one run per day, keeping the min age to at least a month
  // for now we don't use timestamps, if it turns out its problematic we can still change this, counter is 64 bits so time can be used
  // instead at any moment
  // with time based multiply by
  constexpr uint32_t min_runs_since_first = 30;
  auto isCurrentEvaluator = [currentRun = ungrouped_pipelines_run_index](const auto &pair) {
    auto &[hash, info] = pair;
    // keep it when:
    // - it was just tapped this run
    // - its first seen run is not old enough
    // - the time the entry was used is larger than the time it was not used since
    return (info.lastRun == currentRun) || ((currentRun - info.firstRun) < min_runs_since_first) ||
           ((info.lastRun - info.firstRun) > (currentRun - info.lastRun));
  };

  auto isCurrentCount = eastl::count_if(write_access->constantState.vertexShaderSet.begin(),
    write_access->constantState.vertexShaderSet.end(), isCurrentEvaluator);
  isCurrentCount += eastl::count_if(write_access->constantState.pixelShaderSet.begin(),
    write_access->constantState.pixelShaderSet.end(), isCurrentEvaluator);
  isCurrentCount += eastl::count_if(write_access->constantState.computeShaderSet.begin(),
    write_access->constantState.computeShaderSet.end(), isCurrentEvaluator);
  // values stored in dynamic are per convention always current
  isCurrentCount += write_access->dynamicState.vertexShaderSet.size();
  isCurrentCount += write_access->dynamicState.pixelShaderSet.size();
  isCurrentCount += write_access->dynamicState.computeShaderSet.size();

  auto totalCount = write_access->constantState.vertexShaderSet.size() + write_access->constantState.pixelShaderSet.size() +
                    write_access->constantState.computeShaderSet.size() + write_access->dynamicState.vertexShaderSet.size() +
                    write_access->dynamicState.pixelShaderSet.size() + write_access->dynamicState.computeShaderSet.size();

  constexpr uint32_t lib_replacement_threshold_rate = 75;

  if (totalCount && (isCurrentCount * 100 / totalCount) < lib_replacement_threshold_rate)
  {
    ComPtr<ID3D12PipelineLibrary1> replacementLib;
    auto errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&replacementLib)));
    if (FAILED(errorCode))
    {
      logwarn("DX12: Failed to create new pipeline library for ungrouped pipeline library replacement, continuing with old");
      write_access->dynamicState.vertexShaderSet.clear();
      write_access->dynamicState.pixelShaderSet.clear();
      write_access->dynamicState.computeShaderSet.clear();
    }
    if (replacementLib)
    {
      logdbg("DX12: Using new pipeline library for ungrouped library replacement, because %u%% of shaders are considered too old",
        100 - isCurrentCount * 100 / totalCount);
      write_access->dynamicState.library = replacementLib;
    }
  }

  return true;
}

bool load_ungrouped_pipeline_library_file(const auto &write_access, uint64_t &ungrouped_pipelines_run_index, const char *dump_name)
{
  FullFileLoadCB cacheFile{dump_name, DF_IGNORE_MISSING | DF_READ};
  if (!cacheFile.fileHandle)
  {
    logwarn("DX12: Failed to open <%s> to load ungrouped pipelines cache", dump_name);
    return false;
  }

  ReadWrapper reader{cacheFile};
  auto headerReadResult = reader.readValue<UngroupedPipelineLibraryFileHeader>();
  if (!headerReadResult)
  {
    logwarn("DX12: Failed to read the file header from <%s>", dump_name);
    return false;
  }

  if ((UngroupedPipelineLibraryFileHeader::magic_value != headerReadResult->magic) ||
      (UngroupedPipelineLibraryFileHeader::version_value != headerReadResult->version))
  {
    logwarn("DX12: Invalid ungrouped file header, magic or version mismatch <%s>", dump_name);
    return false;
  }

  auto headerHashReadResult = reader.readValue<ShaderHashValue>();
  if (!headerHashReadResult)
  {
    logwarn("DX12: Failed to read header hash value <%s>", dump_name);
    return false;
  }

  if (ShaderHashValue::calculate(&headerReadResult.value(), 1) != headerHashReadResult.value())
  {
    logwarn("DX12: Invalid ungrouped file header, hash mismatch <%s>", dump_name);
    return false;
  }

  auto vertexShaderSetArrayReadResult =
    reader.readContainer<DynamicArray<eastl::decay_t<decltype(*write_access->constantState.vertexShaderSet.begin())>>>(
      headerReadResult->vertexShaderArraySize);
  if (!vertexShaderSetArrayReadResult)
  {
    logwarn("DX12: Failed to read vertex shader set array <%s>", dump_name);
    return false;
  }

  if (ShaderHashValue::calculate(vertexShaderSetArrayReadResult->data(), vertexShaderSetArrayReadResult->size()) !=
      headerReadResult->vertexShaderArrayHash)
  {
    logwarn("DX12: Invalid ungrouped file, vertex shader set array hash mismatch <%s>", dump_name);
    return false;
  }

  auto pixelShaderSetArrayReadResult =
    reader.readContainer<DynamicArray<eastl::decay_t<decltype(*write_access->constantState.pixelShaderSet.begin())>>>(
      headerReadResult->pixelShaderArraySize);
  if (!pixelShaderSetArrayReadResult)
  {
    logwarn("DX12: Failed to read pixel shader set array <%s>", dump_name);
    return false;
  }

  if (ShaderHashValue::calculate(pixelShaderSetArrayReadResult->data(), pixelShaderSetArrayReadResult->size()) !=
      headerReadResult->pixelShaderArrayHash)
  {
    logwarn("DX12: Invalid ungrouped file, pixel shader set array hash mismatch <%s>", dump_name);
    return false;
  }

  auto computeShaderSetArrayReadResult =
    reader.readContainer<DynamicArray<eastl::decay_t<decltype(*write_access->constantState.computeShaderSet.begin())>>>(
      headerReadResult->computeShaderArraySize);
  if (!computeShaderSetArrayReadResult)
  {
    logwarn("DX12: Failed to read compute shader set array <%s>", dump_name);
    return false;
  }

  if (ShaderHashValue::calculate(computeShaderSetArrayReadResult->data(), computeShaderSetArrayReadResult->size()) !=
      headerReadResult->computeShaderArrayHash)
  {
    logwarn("DX12: Invalid ungrouped file, compute shader set array hash mismatch <%s>", dump_name);
    return false;
  }

  auto libraryBlobReadResult = reader.readContainer<DynamicArray<uint8_t>>(headerReadResult->librarySize);
  if (!libraryBlobReadResult)
  {
    logwarn("DX12: Failed to read pipeline library blob <%s>", dump_name);
    return false;
  }

  auto transferArray = [](auto &target, auto &source) {
    target.reserve(source.size());
    // each hash and lifetime array may has duplicated hash entries, we have to properly merge them
    for (auto &e : source)
    {
      auto result = target.insert(e);
      if (!result.second)
      {
        result.first->second.firstRun = eastl::min(result.first->second.firstRun, e.second.firstRun);
        result.first->second.lastRun = eastl::max(result.first->second.lastRun, e.second.lastRun);
      }
    }
  };

  write_access->reset();
  transferArray(write_access->constantState.vertexShaderSet, *vertexShaderSetArrayReadResult);
  transferArray(write_access->constantState.pixelShaderSet, *pixelShaderSetArrayReadResult);
  transferArray(write_access->constantState.computeShaderSet, *computeShaderSetArrayReadResult);

  write_access->constantState.libraryBlob = eastl::move(libraryBlobReadResult.value());

  ungrouped_pipelines_run_index = 1 + headerReadResult->runIndex;

  return true;
}
} // namespace
#endif

void PipelineCache::init([[maybe_unused]] const SetupParameters &params)
{
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

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  rootSignaturesUsesCBVDescriptorRanges = params.rootSignaturesUsesCBVDescriptorRanges;
#endif

  setupBuildInPipelineLibrary(params, deviceFeatures);
  setupUngroupedPipelineLibrary(params);
#endif
}

void PipelineCache::shutdown([[maybe_unused]] const ShutdownParameters &params)
{
#if _TARGET_PC_WIN
  storeBuildInPipelineLibraryFile(params);
  shutdownBuildInPipelineLibrary();

  storeUngroupedPipelineLibraryFile(params);
  shutdownUngroupedPipelineLibrary();

  waitForWriterThreads();
#endif
}

void PipelineCache::preRecovery()
{
#if _TARGET_PC_WIN
  preRecoveryBuildInPipelineLibrary();
  preRecoveryUngroupedPipelineLibrary();

  preRecoverActiveGroup = 0;
  for (auto &group : shaderDumpsInfos)
  {
    auto result = group.access()->preRecovery();
    // +1 because group 0 is special and all others start from 1
    auto groupId = eastl::distance(&shaderDumpsInfos[0], &group) + 1;
    switch (result)
    {
      case GroupPreRecoveryResult::NoLibs:
        // most common case, when the group is not used, then there is no lib and nothing to do
        break;
      case GroupPreRecoveryResult::NoChanges:
        logdbg("DX12: No rescue for pipeline library blob for group <%d> pipelines attempted", groupId);
        preRecoverActiveGroup |= 1u << groupId;
        break;
      case GroupPreRecoveryResult::NoRescue:
        logdbg("DX12: Attempt to rescue pipeline library blob for group <%d> pipelines failed", groupId);
        preRecoverActiveGroup |= 1u << groupId;
        break;
      case GroupPreRecoveryResult::Rescue:
        logdbg("DX12: Rescued pipeline library blob for group <%d> pipelines", groupId);
        preRecoverActiveGroup |= 1u << groupId;
        break;
    }
  }
#endif
}

void PipelineCache::recover([[maybe_unused]] D3DDevice *device, [[maybe_unused]] D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes)
{
#if _TARGET_PC_WIN
  D3D12_FEATURE_DATA_SHADER_CACHE cacheFlags = {};
  if (DX12_CHECK_OK(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &cacheFlags, sizeof(cacheFlags))))
  {
    deviceFeatures = cacheFlags.SupportFlags & allowed_modes;
  }

  logdbg("DX12: PipelineCache features:");
  logdbg("DX12: Per PSO cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO) != 0);
  logdbg("DX12: Pipeline cache library: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_LIBRARY) != 0);
  logdbg("DX12: Automatic OS in memory cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE) != 0);
  logdbg("DX12: Automatic OS on disk cache: %u", (deviceFeatures & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE) != 0);

  recoverBuildInPipelineLibrary(device, deviceFeatures);
  recover_ungrouped_pipeline_library(device, deviceFeatures, ungroupedPipelinesInfo.access(), "ungrouped", ungroupedPipelinesRunIndex);
  for (auto &group : shaderDumpsInfos)
  {
    auto groupId = eastl::distance(&shaderDumpsInfos[0], &group) + 1;
    if (0 == (preRecoverActiveGroup & (1u << groupId)))
    {
      continue;
    }
    recover_for_store(device, deviceFeatures, group.access(), "shaderDumpGroup");
  }
#endif
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

bool PipelineCache::loadCacheForShaderGroup(D3DDevice *device, uint32_t group_index, eastl::string_view dump_name,
  eastl::span<const dxil::HashValue> all_shader_hashes)
{
  if (dump_name.empty())
  {
    logwarn("DX12: Can not de-serialize shader dump pipeline cache, as no name was provided");
    return false;
  }

  auto group = getGroup(group_index);
  if (!group)
  {
    logwarn("DX12: Can not de-serialize shader dump pipeline cache, group was %u", group_index);
    return false;
  }

  // writer may still access the file we want to access (after dump reload)
  waitForWriterThreads();

  auto writeAccess = group->access();
  // first reset everything
  writeAccess->reset();

  auto ok = load_shader_cache_for_group(writeAccess, dump_name);
  ok = ok && setup_for_group(device, deviceFeatures, writeAccess, dump_name, all_shader_hashes);
  if (!create_default_libraries(device, deviceFeatures, writeAccess))
  {
    ok = false;
  }

  return ok;
}

void PipelineCache::storeAndUnloadCacheForShaderGroup(uint32_t group_index, eastl::string_view dump_name)
{
  if (dump_name.empty())
  {
    logwarn("DX12: Can not serialize shader dump pipeline cache, as no name was provided");
    return;
  }

  auto group = getGroup(group_index);
  if (!group)
  {
    logwarn("DX12: Can not serialize shader dump pipeline cache, group was %u", group_index);
    return;
  }

  // prevent any alterations to the group so data is consistent for the whole serialization process
  auto writeAccess = group->access();

  store_cache_for_pipeline_group(writeAccess, dump_name, issuedCacheWrites, pendingCacheWrites);

  writeAccess->reset();
}

dag::Expected<void, HRESULT> PipelineCache::addGraphicsPipelineVariant(uint32_t group_index, ID3D12PipelineState *pipeline,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident)
{
  auto group = getGroup(group_index);
  return group ? add_graphics_pipeline_vairiant_to_store(*group, pipeline, desc, ident,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges})
               : add_graphics_pipeline_vairiant_to_store(ungroupedPipelinesInfo, pipeline, desc, ident,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges}, ungroupedPipelinesRunIndex);
}

dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> PipelineCache::loadGraphicsPipelineVariant(uint32_t group_index,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident)
{
  auto group = getGroup(group_index);
  return group ? load_graphics_pipeline_variant_from_store(*group, desc, ident,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges})
               : load_graphics_pipeline_variant_from_store(ungroupedPipelinesInfo, desc, ident,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges}, ungroupedPipelinesRunIndex);
}

dag::Expected<void, HRESULT> PipelineCache::addComputePipeline(uint32_t group_index, ID3D12PipelineState *pipeline,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier)
{
  auto group = getGroup(group_index);
  return group ? add_compute_to_store(*group, pipeline, desc, shader_identifier,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges})
               : add_compute_to_store(ungroupedPipelinesInfo, pipeline, desc, shader_identifier,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges}, ungroupedPipelinesRunIndex);
}

dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> PipelineCache::loadComputePipeline(uint32_t group_index,
  const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier)
{
  auto group = getGroup(group_index);
  return group ? load_compute_pipeline_from_store(*group, desc, shader_identifier,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges})
               : load_compute_pipeline_from_store(ungroupedPipelinesInfo, desc, shader_identifier,
                   {.usesConstBufferRootDescriptors = rootSignaturesUsesCBVDescriptorRanges}, ungroupedPipelinesRunIndex);
}

bool PipelineCache::hasBasePipeline(uint32_t group_index, const BasePipelineIdentifier &ident)
{
  auto group = getGroup(group_index);
  return group ? has_base_pipeline_in_store(*group, ident) : has_base_pipeline_in_store(ungroupedPipelinesInfo, ident);
}

bool PipelineCache::hasComputePipeline(uint32_t group_index, const dxil::HashValue &shader_identifier)
{
  auto group = getGroup(group_index);
  return group ? has_compute_pipeline_in_store(*group, shader_identifier)
               : has_compute_pipeline_in_store(ungroupedPipelinesInfo, shader_identifier);
}
#endif

#if _TARGET_PC_WIN
void PipelineCache::setupBuildInPipelineLibrary(const SetupParameters &params, const D3D12_SHADER_CACHE_SUPPORT_FLAGS cache_support)
{
  buildInShouldRecord = params.recordBuildInCache;

  loadBuildInPipelineLibraryFile(params);

  recoverBuildInPipelineLibrary(params.device, cache_support);
}

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

bool PipelineCache::setupUngroupedPipelineLibrary(const SetupParameters &params)
{
  if (!params.fileName || '\0' == params.fileName[0])
  {
    logwarn("DX12: Can not de-serialize ungrouped pipeline cache, as no name was provided");
    return false;
  }

  auto writeAccess = ungroupedPipelinesInfo.access();
  if (load_ungrouped_pipeline_library_file(writeAccess, ungroupedPipelinesRunIndex, params.fileName))
  {
    recover_ungrouped_pipeline_library(params.device, deviceFeatures, writeAccess, params.fileName, ungroupedPipelinesRunIndex);
  }
  return create_default_libraries(params.device, deviceFeatures, writeAccess);
}

void PipelineCache::storeUngroupedPipelineLibraryFile(const ShutdownParameters &params)
{
  auto writeAccess = ungroupedPipelinesInfo.access();

  if (!writeAccess->hasLibrary())
  {
    return;
  }

  if (!writeAccess->hasAnyData())
  {
    return;
  }

  // have to always store the ungrouped cache as the run index changes each time, so no has changed checks

  auto libSerializeResult = writeAccess->serializeLibrary();
  if (!libSerializeResult)
  {
    logwarn("DX12: Failed to obtain pipeline library blob for ungrouped: %s", dxgi_error_code_to_string(libSerializeResult.error()));
    return;
  }

  auto &libraryBlob = libSerializeResult.value();
  auto vertexShaderArray = writeAccess->serializeVertexShaderInfos();
  auto pixelShaderArray = writeAccess->serializePixelShaderInfo();
  auto computeShaderArray = writeAccess->serializeComputeShaderInfo();

  const UngroupedPipelineLibraryFileHeader header{
    .magic = UngroupedPipelineLibraryFileHeader::magic_value,
    .version = UngroupedPipelineLibraryFileHeader::version_value,
    .vertexShaderArrayHash = ShaderHashValue::calculate(vertexShaderArray.data(), vertexShaderArray.size()),
    .pixelShaderArrayHash = ShaderHashValue::calculate(pixelShaderArray.data(), pixelShaderArray.size()),
    .computeShaderArrayHash = ShaderHashValue::calculate(computeShaderArray.data(), computeShaderArray.size()),
    .vertexShaderArraySize = static_cast<uint32_t>(vertexShaderArray.size()),
    .pixelShaderArraySize = static_cast<uint32_t>(pixelShaderArray.size()),
    .computeShaderArraySize = static_cast<uint32_t>(computeShaderArray.size()),
    .librarySize = static_cast<uint32_t>(libraryBlob.size()),
    .runIndex = ungroupedPipelinesRunIndex,
  };

  dd_mkpath(params.fileName);
  FullFileSaveCB cacheFile{params.fileName, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING};
  if (!cacheFile.fileHandle)
  {
    logwarn(
      "DX12: Failed to open <%s> to serialize pipeline cache, missing cache folder or insufficient privileges might be the cause "
      "for this error",
      params.fileName);
    return;
  }

  auto headerHash = ShaderHashValue::calculate(&header, 1);

  WriteWrapper writer{cacheFile};
  writer.write(header);
  writer.write(headerHash);
  writer.writeArray(vertexShaderArray.data(), vertexShaderArray.size());
  writer.writeArray(pixelShaderArray.data(), pixelShaderArray.size());
  writer.writeArray(computeShaderArray.data(), computeShaderArray.size());
  writer.writeArray(libraryBlob.data(), libraryBlob.size());

  if (writer.close())
  {
    logdbg("DX12: Written ungrouped pipelines cache file <%s>", params.fileName);
    logdbg("DX12: vertex shader count: %u", header.vertexShaderArraySize);
    logdbg("DX12: pixel shader count: %u", header.pixelShaderArraySize);
    logdbg("DX12: compute shader count: %u", header.computeShaderArraySize);
    ByteUnits librarySizeInBytes{header.librarySize};
    logdbg("DX12: Pipeline library size: %.4f %s", librarySizeInBytes.units(), librarySizeInBytes.name());
  }
  else
  {
    logwarn("DX12: Error while attempting to write ungrouped pipelines cache file to <%s>", params.fileName);
  }
}

void PipelineCache::preRecoveryUngroupedPipelineLibrary()
{
  auto result = ungroupedPipelinesInfo.access()->preRecovery();
  switch (result)
  {
    case GroupPreRecoveryResult::NoLibs:
    case GroupPreRecoveryResult::NoChanges:
      logdbg("DX12: No rescue for pipeline library blob for ungrouped pipelines attempted");
      break;
    case GroupPreRecoveryResult::NoRescue:
      logdbg("DX12: Attempt to rescue pipeline library blob for ungrouped pipelines failed");
      break;
    case GroupPreRecoveryResult::Rescue: logdbg("DX12: Rescued pipeline library blob for ungrouped pipelines"); break;
  }
}

void PipelineCache::shutdownUngroupedPipelineLibrary() { ungroupedPipelinesInfo.access()->reset(); }

void PipelineCache::waitForWriterThreads()
{
  constexpr int run_wait_ms = 100;
  auto issuedWriteCounter = issuedCacheWrites.load();
  if (0 != issuedWriteCounter)
  {
    logdbg("DX12: Still waiting for %u async cache write threads to launch...", issuedWriteCounter);
    constexpr int total_wait_ms = run_wait_ms * 100;
    auto now = ref_time_ticks();
    for (uint32_t runs = 0; get_time_usec(now) < (1000 * total_wait_ms); ++runs)
    {
      issuedWriteCounter = issuedCacheWrites.load();
      if (0 == issuedWriteCounter)
      {
        break;
      }
      logdbg("DX12: ...waiting for ~%dms, attempt %u...", run_wait_ms, 1 + runs);
      os_wait_on_address(reinterpret_cast<volatile unsigned *>(&issuedCacheWrites), reinterpret_cast<unsigned *>(&issuedWriteCounter),
        run_wait_ms);
    }
    if (0 != issuedWriteCounter)
    {
      logwarn("DX12: ...giving up on waiting for async writer thread launches, %u are still pending to be launched",
        issuedWriteCounter);
    }
  }

  auto pendingWriteCounter = pendingCacheWrites.load();
  if (0 != pendingWriteCounter)
  {
    logdbg("DX12: Still waiting for %u async cache writes to be completed...", pendingWriteCounter);
    // here we do not have a run limit as we know threads are working and we want to let them finish
    for (uint32_t runs = 0;; ++runs)
    {
      pendingWriteCounter = pendingCacheWrites.load();
      if (0 == pendingWriteCounter)
      {
        break;
      }
      logdbg("DX12: ...waiting for %dms, attempt %u...", run_wait_ms, 1 + runs);
      os_wait_on_address(reinterpret_cast<volatile unsigned *>(&pendingCacheWrites),
        reinterpret_cast<unsigned *>(&pendingWriteCounter), run_wait_ms);
    }
  }
}
#endif
