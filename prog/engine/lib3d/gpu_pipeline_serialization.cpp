// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <d3d12.h>
#include <d3dx12.h>

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>
#include <EASTL/algorithm.h>

#include <3d/gpu_pipeline_serialization.h>

template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>
struct ObjectTypeToShaderType;

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::VS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::PS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::GS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::HS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::DS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::CS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::MS;
};

template <>
struct ObjectTypeToShaderType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>
{
  static constexpr DeserializerReceiver::ShaderType shader_type = DeserializerReceiver::ShaderType::AS;
};

enum class DescType
{
  GraphicsPipeline,
  ComputePipeline,
  PipelineState,
  // slim modes do not encode the shader byte codes, instead they encode a size and hash pair
  // the tool later need a shader bin dump paired with the log to extract the shader byte code
  // the dump does not need to 100% match, it just needs to contain the needed byte codes.
  GraphicsPipelineSlim,
  ComputePipelineSlim,
  PipelineStateSlim,
};

class PipelineDeserializerBase
{
  const uint8_t *base = nullptr;
  const uint8_t *at = nullptr;
  const uint8_t *stop = nullptr;
  eastl::vector<eastl::unique_ptr<uint8_t[]>> extraStore;
  DeserializerReceiver &deserializer;

protected:
  bool hadError = false;

  void peekBytes(void *target, size_t count)
  {
    if (stop - at < count)
    {
      hadError = true;
      memset(target, 0, count);
    }
    else
    {
      memcpy(target, at, count);
    }
  }

  template <typename T>
  void peekType(T &target)
  {
    peekBytes(&target, sizeof(T));
  }

  void readBytes(void *target, size_t count)
  {
    if (stop - at < count)
    {
      hadError = true;
      memset(target, 0, count);
    }
    else
    {
      memcpy(target, at, count);
      at += count;
    }
  }

  eastl::span<const uint8_t> getDataSpan() const { return {at, stop}; }
  eastl::span<const uint8_t> getAllDataSpan() const { return {base, stop}; }

  template <typename T>
  void serializeType(T &target)
  {
    readBytes(&target, sizeof(T));
  }

  void serializeString(const char *&str)
  {
    uint64_t ln = 0;
    serializeType(ln);
    str = (const char *)readToExtraStore(ln);
  }

  template <typename T>
  T *allocateExtraStore(size_t count)
  {
    if (count < 1)
    {
      return nullptr;
    }
    auto mem = eastl::make_unique<uint8_t[]>(sizeof(T) * count);
    auto r = mem.get();
    extraStore.push_back(eastl::move(mem));
    return reinterpret_cast<T *>(r);
  }

  void *readToExtraStore(size_t count)
  {
    if (count < 1)
    {
      return nullptr;
    }
    auto r = allocateExtraStore<uint8_t>(count);
    readBytes(r, count);
    return r;
  }

  template <typename T>
  void serializeBytes(const void *&ptr, T &count)
  {
    uint64_t countValue = 0;
    serializeType(countValue);
    ptr = readToExtraStore(countValue);
    count = static_cast<T>(countValue);
  }

  template <typename T, typename U>
  void serializeArray(T *&ary, U &count, auto serialize_element)
  {
    uint64_t countValue = 0;
    serializeType(countValue);
    auto store = allocateExtraStore<eastl::remove_const_t<T>>(countValue);
    for (uint64_t i = 0; i < countValue; ++i)
    {
      serialize_element(store[i]);
    }
    ary = store;
    count = static_cast<U>(countValue);
  }

  // invokes data_handler when count > 0
  template <typename U>
  void serializeSpecial(const void *&mem, U &count, auto &&data_handler)
  {
    uint64_t countValue = 0;
    serializeType(countValue);
    const uint8_t *store = nullptr;
    if (countValue > 0)
    {
      data_handler(store, countValue, [this](auto count) { return allocateExtraStore<uint8_t>(count); });
    }
    mem = store;
    count = static_cast<U>(countValue);
  }

  static constexpr bool is_reader = true;
  static constexpr bool is_writer = false;

  ID3D12RootSignature *onRootSignature(const RootSignatureByteCode &byte_code) { return deserializer.onRootSignature(byte_code); }
  void onGraphicsPipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc) { deserializer.onGraphicsPipeline(desc); }
  void onComputePipeline(const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc) { deserializer.onComputePipeline(desc); }
  void onPipeline(const D3D12_PIPELINE_STATE_STREAM_DESC &desc) { deserializer.onPipeline(desc); }
  const void *onShaderHash(DeserializerReceiver::ShaderType shader_type, size_t shader_binary_size, const ShaderHashValue &hash)
  {
    return deserializer.onShaderHash(shader_type, shader_binary_size, hash);
  }

  bool slimMode = false;

  // translates slim mode DescTypes into no slim counterparts for simpler followup uses
  DescType setSlimMode(DescType type)
  {
    switch (type)
    {
      case DescType::GraphicsPipeline:
      case DescType::ComputePipeline:
      case DescType::PipelineState: slimMode = false; return type;
      case DescType::GraphicsPipelineSlim: slimMode = true; return DescType::GraphicsPipeline;
      case DescType::ComputePipelineSlim: slimMode = true; return DescType::ComputePipeline;
      case DescType::PipelineStateSlim: slimMode = true; return DescType::PipelineState;
    }
    return type;
  }

  bool isSlimMode() const { return slimMode; }

  struct SubObjectSerializationContext
  {
    eastl::vector<uint8_t> blob;

    template <typename T>
    T &type()
    {
      auto ofs = blob.size();
      blob.resize(blob.size() + sizeof(T));
      return *::new (blob.data() + ofs) T;
    }

    template <typename T>
    void advanceBy()
    {}
  };

public:
  PipelineDeserializerBase(dag::Span<const uint8_t> data, DeserializerReceiver &ds) :
    base{data.data()}, at{data.data()}, stop{data.data() + data.size()}, deserializer{ds}
  {}
};

class PipelineSerializerBase
{
  dag::Vector<uint8_t> dataStore;

protected:
  void resetStore() { dataStore.clear(); }
  size_t getWritePos() { return dataStore.size(); }
  template <typename T>
  void patch(size_t ofs, const T &value)
  {
    if (ofs + sizeof(T) > dataStore.size())
    {
      hadError = true;
      return;
    }
    memcpy(dataStore.data() + ofs, &value, sizeof(T));
  }

  template <typename T>
  void serializeType(const T &value)
  {
    memcpy(dataStore.append_default(sizeof(T)), &value, sizeof(T));
  }

  void writeBytes(const void *ptr, size_t bytes) { memcpy(dataStore.append_default(bytes), ptr, bytes); }

  void serializeString(const char *str)
  {
    uint64_t len = strlen(str) + 1;
    serializeType(len);
    writeBytes(str, len);
  }

  void serializeBytes(const void *ptr, uint64_t count)
  {
    serializeType(count);
    writeBytes(ptr, count);
  }

  template <typename T>
  void serializeArray(const T *ary, uint64_t count, auto serialize_element)
  {
    serializeType(count);
    for (uint64_t i = 0; i < count; ++i)
    {
      serialize_element(const_cast<T &>(ary[i]));
    }
  }

  // only invokes data_handler when count > 0
  void serializeSpecial(const void *mem, uint64_t count, auto &&data_handler)
  {
    serializeType(count);
    if (count > 0)
    {
      data_handler(mem, count, [](auto) { return nullptr; });
    }
  }

  static constexpr bool is_reader = false;
  static constexpr bool is_writer = true;

  bool hadError = false;
  bool slimMode = false;

  bool isSlimMode() const { return slimMode; }

  struct SubObjectSerializationContext
  {
    const uint8_t *at = nullptr;
    const uint8_t *stop = nullptr;

    template <typename T>
    void advanceBy()
    {
      at = eastl::min(at + sizeof(T), stop);
    }

    template <typename T>
    T &type() const
    {
      return *const_cast<T *>(reinterpret_cast<const T *>(at));
    }
  };

public:
  PipelineSerializerBase(bool slim_mode) : slimMode{slim_mode} {}

  dag::Vector<uint8_t> &extractData() { return dataStore; }
};

template <typename T>
class PipelineCodec : public T
{
protected:
  using T::serializeArray;
  using T::serializeBytes;
  using T::serializeSpecial;
  using T::serializeString;
  using T::serializeType;
  using SubObjectSerializationContext = T::SubObjectSerializationContext;

public:
  using T::T;

  void serializeData(uint8_t &value) { serializeType(value); }
  void serializeData(uint32_t &value) { serializeType(value); }
  void serializeData(uint64_t &value) { serializeType(value); }
  void serializeData(DXGI_FORMAT &value) { serializeType(value); }
  void serializeData(D3D12_INPUT_CLASSIFICATION &value) { serializeType(value); }
  void serializeData(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE &value) { serializeType(value); }
  void serializeData(D3D12_PRIMITIVE_TOPOLOGY_TYPE &value) { serializeType(value); }
  void serializeData(DXGI_FORMAT (&values)[8]) { serializeType(values); }
  void serializeData(D3D12_PIPELINE_STATE_FLAGS &value) { serializeType(value); }
  void serializeData(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE &value) { serializeType(value); }
  void serializeData(D3D12_VIEW_INSTANCING_FLAGS &value) { serializeType(value); }
  void serializeData(D3D12_BLEND_DESC &desc) { serializeType(desc); }
  void serializeData(D3D12_RASTERIZER_DESC &desc) { serializeType(desc); }
  void serializeData(D3D12_RASTERIZER_DESC1 &desc) { serializeType(desc); }
  void serializeData(D3D12_RASTERIZER_DESC2 &desc) { serializeType(desc); }
  void serializeData(D3D12_DEPTH_STENCIL_DESC &desc) { serializeType(desc); }
  void serializeData(D3D12_DEPTH_STENCIL_DESC1 &desc) { serializeType(desc); }
  void serializeData(D3D12_DEPTH_STENCIL_DESC2 &desc) { serializeType(desc); }
  void serializeData(DXGI_SAMPLE_DESC &desc) { serializeType(desc); }
  void serializeData(D3D12_RT_FORMAT_ARRAY &desc) { serializeType(desc); }
  void serializeData(D3D12_VIEW_INSTANCE_LOCATION &desc) { serializeType(desc); }
  void serializeData(D3D12_SO_DECLARATION_ENTRY &desc)
  {
    serializeData(desc.Stream);
    serializeString(desc.SemanticName);
    serializeData(desc.SemanticIndex);
    serializeData(desc.StartComponent);
    serializeData(desc.ComponentCount);
    serializeData(desc.OutputSlot);
  }
  void serializeData(D3D12_INPUT_ELEMENT_DESC &desc)
  {
    serializeString(desc.SemanticName);
    serializeData(desc.SemanticIndex);
    serializeData(desc.Format);
    serializeData(desc.InputSlot);
    serializeData(desc.AlignedByteOffset);
    serializeData(desc.InputSlotClass);
    serializeData(desc.InstanceDataStepRate);
  }

  void serializeData(D3D12_SHADER_BYTECODE &byte_code, [[maybe_unused]] DeserializerReceiver::ShaderType shader_type)
  {
    serializeSpecial(byte_code.pShaderBytecode, byte_code.BytecodeLength, [&, this](auto *&data, auto count, auto data_alloc) {
      if constexpr (T::is_writer)
      {
        if (T::isSlimMode())
        {
          auto value = ShaderHashValue::calculate(reinterpret_cast<const uint8_t *>(data), count);
          T::writeBytes(&value, sizeof(value));
        }
        else
        {
          T::writeBytes(data, count);
        }
      }
      else
      {
        if (T::isSlimMode())
        {
          ShaderHashValue hashValue;
          T::readBytes(&hashValue, sizeof(hashValue));
          data = reinterpret_cast<const uint8_t *>(T::onShaderHash(shader_type, count, hashValue));
        }
        else
        {
          auto buffer = data_alloc(count);
          T::readBytes(buffer, count);
          data = buffer;
        }
      }
    });
  }

  void serializeData(D3D12_VIEW_INSTANCING_DESC &desc)
  {
    serializeData(desc.Flags);
    serializeArray(desc.pViewInstanceLocations, desc.ViewInstanceCount, [this](auto &target) { serializeData(target); });
  }

  void serializeData(D3D12_CACHED_PIPELINE_STATE &cache_state)
  {
    serializeBytes(cache_state.pCachedBlob, cache_state.CachedBlobSizeInBytes);
  }

  void serializeData(D3D12_INPUT_LAYOUT_DESC &desc)
  {
    serializeArray(desc.pInputElementDescs, desc.NumElements, [this](auto &target) { serializeData(target); });
  }

  void serializeData(D3D12_STREAM_OUTPUT_DESC &desc)
  {
    serializeArray(desc.pSODeclaration, desc.NumEntries, [this](auto &target) { serializeData(target); });
    serializeArray(desc.pBufferStrides, desc.NumStrides, [this](auto &target) { serializeData(target); });
    serializeData(desc.RasterizedStream);
  }

  void serializeData(D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc)
  {
    serializeData(desc.VS, DeserializerReceiver::ShaderType::VS);
    serializeData(desc.PS, DeserializerReceiver::ShaderType::PS);
    serializeData(desc.DS, DeserializerReceiver::ShaderType::DS);
    serializeData(desc.HS, DeserializerReceiver::ShaderType::HS);
    serializeData(desc.GS, DeserializerReceiver::ShaderType::GS);
    serializeData(desc.StreamOutput);
    serializeData(desc.BlendState);
    serializeData(desc.SampleMask);
    serializeData(desc.RasterizerState);
    serializeData(desc.DepthStencilState);
    serializeData(desc.InputLayout);
    serializeData(desc.IBStripCutValue);
    serializeData(desc.PrimitiveTopologyType);
    serializeData(desc.NumRenderTargets);
    serializeData(desc.RTVFormats);
    serializeData(desc.DSVFormat);
    serializeData(desc.SampleDesc);
    serializeData(desc.NodeMask);
    serializeData(desc.CachedPSO);
    serializeData(desc.Flags);
  }

  void serializeData(D3D12_COMPUTE_PIPELINE_STATE_DESC &desc)
  {
    serializeData(desc.CS, DeserializerReceiver::ShaderType::CS);
    serializeData(desc.NodeMask);
    serializeData(desc.CachedPSO);
    serializeData(desc.Flags);
  }

  template <typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename InnerType>
  void serializeData(CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<T, Type, InnerType> &desc)
  {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = Type;
    serializeData(type);
    G_ASSERT(type == Type);
    if constexpr (eastl::is_same<T, D3D12_SHADER_BYTECODE>::value)
    {
      serializeData(static_cast<D3D12_SHADER_BYTECODE &>(desc), ObjectTypeToShaderType<Type>::shader_type);
    }
    else
    {
      serializeData(static_cast<T &>(desc));
    }
  }

  template <typename T>
  void skipData(SubObjectSerializationContext &ctx)
  {
    ctx.template advanceBy<T>();
  }

  template <typename T>
  void serializeData(SubObjectSerializationContext &ctx)
  {
    serializeData(ctx.template type<T>());
    ctx.template advanceBy<T>();
  }

  void serializeData(SubObjectSerializationContext &ctx)
  {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type;
    if constexpr (T::is_writer)
    {
      type = ctx.template type<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();
    }
    else
    {
      T::peekType(type);
    }
    if (T::hadError)
    {
      return;
    }
    switch (type)
    {
      default: break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: skipData<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_VS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_PS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_DS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_HS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_GS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_CS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: serializeData<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND: serializeData<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK: serializeData<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER: serializeData<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL: serializeData<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: serializeData<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC: serializeData<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK: serializeData<CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: serializeData<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_FLAGS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_AS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: serializeData<CD3DX12_PIPELINE_STATE_STREAM_MS>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
        serializeData<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2>(ctx);
        return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1: serializeData<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1>(ctx); return;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2: serializeData<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2>(ctx); return;
    }
    G_ASSERTF(false, "Error while decoding DX12 Pipeline state subobject, unexpected value %u", type);
    T::hadError = true;
  }
};

class PipelineDeserializer : public PipelineCodec<PipelineDeserializerBase>
{
  using BaseType = PipelineCodec<PipelineDeserializerBase>;
  using BaseType::serializeData;

  eastl::span<const uint8_t> parseRootSignatureByteCode() { return getDataSpan(); }

public:
  void deserialize()
  {
    DescType rawMode;
    serializeType(rawMode);
    auto mode = setSlimMode(rawMode);
    if (DescType::GraphicsPipeline == mode)
    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
      serializeData(desc);
      if (hadError)
      {
        return;
      }
      auto rsBc = parseRootSignatureByteCode();
      desc.pRootSignature = onRootSignature(rsBc);
      if (!desc.pRootSignature || hadError)
      {
        return;
      }
      onGraphicsPipeline(desc);
    }
    else if (DescType::ComputePipeline == mode)
    {
      D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
      serializeData(desc);
      if (hadError)
      {
        return;
      }
      auto rsBc = parseRootSignatureByteCode();
      desc.pRootSignature = onRootSignature(rsBc);
      if (!desc.pRootSignature || hadError)
      {
        return;
      }
      onComputePipeline(desc);
    }
    else if (DescType::PipelineState == mode)
    {
      SubObjectSerializationContext desc;

      uint64_t blobEndOfs = 0;
      serializeData(blobEndOfs);

      auto allData = getAllDataSpan();

      // pvs fails to see that serializeData is writing to blobEndOfs and so it most likely is not 0
      if (blobEndOfs > allData.size()) // -V547
      {
        hadError = true;
      }
      else
      {
        auto blobEnd = allData.data() + blobEndOfs;
        while (getDataSpan().data() < blobEnd && !hadError)
        {
          serializeData(desc);
        }
      }

      if (hadError)
      {
        desc.blob.clear();
      }
      auto rsBc = parseRootSignatureByteCode();
      auto rootSign = onRootSignature(rsBc);
      if (!rootSign || hadError)
      {
        return;
      }
      desc.type<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>() = rootSign;
      onPipeline({.SizeInBytes = desc.blob.size(), .pPipelineStateSubobjectStream = desc.blob.data()});
    }
    else
    {
      G_ASSERTF(false, "Unexpected mode %u", static_cast<uint32_t>(rawMode));
    }
  }

  using BaseType::BaseType;
};

void deserialize_pipeline(dag::Span<const uint8_t> data, DeserializerReceiver &receiver)
{
  PipelineDeserializer deserializer{data, receiver};
  deserializer.deserialize();
}

class PipelineSerializer : public PipelineCodec<PipelineSerializerBase>
{
  using BaseType = PipelineCodec<PipelineSerializerBase>;
  using BaseType::serializeData;
  using BaseType::serializeType;

  void setMode(DescType type)
  {
    resetStore();
    serializeType(type);
  }

public:
  using BaseType::BaseType;

  void serialize(const RootSignatureByteCode &byte_code)
  {
    if (!hadError)
    {
      writeBytes(byte_code.data(), byte_code.size());
    }
  }
  void serialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc)
  {
    setMode(slimMode ? DescType::GraphicsPipelineSlim : DescType::GraphicsPipeline);
    serializeData(const_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC &>(desc));
    if (hadError)
    {
      resetStore();
    }
  }
  void serialize(const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc)
  {
    setMode(slimMode ? DescType::ComputePipelineSlim : DescType::ComputePipeline);
    serializeData(const_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC &>(desc));
    if (hadError)
    {
      resetStore();
    }
  }
  void serialize(const D3D12_PIPELINE_STATE_STREAM_DESC &desc)
  {
    setMode(slimMode ? DescType::PipelineStateSlim : DescType::PipelineState);
    uint64_t currentOfs = getWritePos();
    serializeData(currentOfs);
    auto base = static_cast<const uint8_t *>(desc.pPipelineStateSubobjectStream);
    SubObjectSerializationContext subObjectCtx{.at = base, .stop = base + desc.SizeInBytes};
    while (subObjectCtx.at < subObjectCtx.stop && !hadError)
    {
      serializeData(subObjectCtx);
    }
    uint64_t edPos = getWritePos();
    patch(currentOfs, edPos);
    if (hadError)
    {
      resetStore();
    }
  }
};

SerializedGpuPipeline serialize_graphics_pipeline(const SerializationSettings &settings,
  const D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc, const RootSignatureByteCode &root_signature)
{
  PipelineSerializer store{settings.shaderHashes};
  store.serialize(desc);
  store.serialize(root_signature);
  return eastl::move(store.extractData());
}

SerializedGpuPipeline serialize_compute_pipeline(const SerializationSettings &settings, const D3D12_COMPUTE_PIPELINE_STATE_DESC &desc,
  const RootSignatureByteCode &root_signature)
{
  PipelineSerializer store{settings.shaderHashes};
  store.serialize(desc);
  store.serialize(root_signature);
  return eastl::move(store.extractData());
}

SerializedGpuPipeline serialize_pipeline(const SerializationSettings &settings, const D3D12_PIPELINE_STATE_STREAM_DESC &desc,
  const RootSignatureByteCode &root_signature)
{
  PipelineSerializer store{settings.shaderHashes};
  store.serialize(desc);
  store.serialize(root_signature);
  return eastl::move(store.extractData());
}