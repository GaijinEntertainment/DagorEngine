// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>

#include "derived_span.h"
#include "pipeline_cache.h"


namespace drv3d_dx12
{
struct GraphicsPipelineVariantSet
{
  const BasePipelineIdentifier &ident;
  DerivedSpan<const GraphicsPipelineVariantState> variants;
};

struct MeshPipelineVariantSet
{
  const BasePipelineIdentifier &ident;
  DerivedSpan<const MeshPipelineVariantState> variants;
};

namespace pipeline
{
template <typename Decoder>
class DataBlockDecodeEnumarator : private Decoder
{
  const DataBlock &block;
  uint32_t blockIndex = 0;

public:
  DataBlockDecodeEnumarator(const DataBlock &blk, uint32_t start) : block{blk}, blockIndex{start} {}
  template <typename... Ts>
  DataBlockDecodeEnumarator(const DataBlock &blk, uint32_t start, Ts &&...ts) :
    block{blk}, blockIndex{start}, Decoder{eastl::forward<Ts>(ts)...}
  {}

  template <typename... Ts>
  bool invoke(Ts &&...ts) const
  {
    if (completed())
    {
      return false;
    }
    auto blk = block.getBlock(blockIndex);
    if (!blk)
    {
      return false;
    }
    return this->Decoder::invoke(*blk, eastl::forward<Ts>(ts)...);
  }
  template <typename... Ts>
  bool decode(Ts &...targets) const
  {
    if (completed())
    {
      return false;
    }
    auto blk = block.getBlock(blockIndex);
    if (!blk)
    {
      return false;
    }
    return this->Decoder::decode(*blk, targets...);
  }

  bool next()
  {
    if (!completed())
    {
      ++blockIndex;
    }
    return completed();
  }

  bool completed() const { return blockIndex >= block.blockCount(); }
  uint32_t index() const { return blockIndex; }
  const char *name() const { return completed() ? nullptr : block.getBlock(blockIndex)->getBlockName(); }
  dxil::HashValue hash() const
  {
    auto n = name();
    if (n)
    {
      return dxil::HashValue::fromString(n);
    }
    return {};
  }
  uint32_t remaining() const { return block.blockCount() - blockIndex; }
};

template <typename Decoder>
class DataBlockParameterDecodeEnumarator : private Decoder
{
  const DataBlock &block;
  uint32_t parameterIndex = 0;

public:
  DataBlockParameterDecodeEnumarator(const DataBlock &blk, uint32_t start) : block{blk}, parameterIndex{start} {}
  template <typename... Ts>
  DataBlockParameterDecodeEnumarator(const DataBlock &blk, uint32_t start, Ts &&...ts) :
    block{blk}, parameterIndex{start}, Decoder{eastl::forward<Ts>(ts)...}
  {}

  template <typename T>
  bool invoke(T &&target) const
  {
    if (completed())
    {
      return false;
    }
    return this->Decoder::invoke(block, parameterIndex, eastl::forward<T>(target));
  }
  template <typename... Ts>
  bool decode(Ts &...targets) const
  {
    if (completed())
    {
      return false;
    }
    return this->Decoder::decode(block, parameterIndex, targets...);
  }

  bool next()
  {
    if (!completed())
    {
      ++parameterIndex;
    }
    return completed();
  }

  bool completed() const { return parameterIndex >= block.paramCount(); }
  uint32_t index() const { return parameterIndex; }
  uint32_t remaining() const { return block.paramCount() - parameterIndex; }
};

template <typename Encoder>
class DataBlockEncodeVisitor : Encoder
{
  DataBlock &block;

public:
  template <typename... Ts>
  DataBlockEncodeVisitor(DataBlock &blk, Ts &&...ts) : block{blk}, Encoder{eastl::forward<Ts>(ts)...}
  {}
  template <typename... Ts>
  bool encode(Ts &&...ts)
  {
    using BlockNameType = typename Encoder::BlockNameType;
    BlockNameType blockName;
    this->Encoder::formatBlockName(blockName, block.blockCount());
    DataBlock *newBlock = block.addNewBlock(blockName);
    if (!newBlock)
    {
      return false;
    }
    if (!this->Encoder::encode(*newBlock, eastl::forward<Ts>(ts)...))
    {
      block.removeBlock(block.blockCount() - 1);
      return true;
    }

    return true;
  }
  template <typename... Ts>
  bool encode2(Ts &&...ts)
  {
    using HashBlockNameType = typename Encoder::HashBlockNameType;
    HashBlockNameType blockName;
    this->Encoder::formatBlockNameHash(blockName, ts...);
    DataBlock *newBlock = block.addNewBlock(blockName);
    if (!newBlock)
    {
      return false;
    }
    if (!this->Encoder::encode(*newBlock, eastl::forward<Ts>(ts)...))
    {
      block.removeBlock(block.blockCount() - 1);
      return true;
    }

    return true;
  }
  uint32_t count() const { return block.blockCount(); }
};

template <typename R, typename D>
class DefaltInvokeDecoder
{
public:
  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    R out;
    if (!static_cast<const D *>(this)->decode(blk, out))
    {
      return false;
    }
    return target(out);
  }
};

template <typename R, typename D>
class DefaultHashNameInvokeDecoder
{
public:
  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    R out;
    if (!static_cast<const D *>(this)->decode(blk, out))
    {
      return false;
    }
    return target(dxil::HashValue::fromString(blk.getBlockName()), out);
  }
};


class FormatedDecoder
{
protected:
  const char *defaultFormat = "dx12";

public:
  FormatedDecoder() = default;
  FormatedDecoder(const char *fmt) : defaultFormat{fmt} {}
  FormatedDecoder(const FormatedDecoder &) = default;
};

template <typename D, size_t N = 64>
struct EncoderBlockNameStore
{
  using BlockNameType = char[N];

  void formatBlockName(BlockNameType &target, int32_t index) { sprintf_s(target, D::blockFormat, index); }
};

template <typename T>
struct EncoderBlockHashNameStore
{
  using HashBlockNameType = char[sizeof(dxil::HashValue) * 2 + 1];

  void formatBlockNameHash(HashBlockNameType &target, const T &v)
  {
    dxil::HashValue::calculate(&v, 1).convertToString(target, sizeof(HashBlockNameType));
  }
};

template <typename T, typename D>
struct ObjectHashDecoder
{
  static bool is_hash_block_name(const char *name)
  {
    // either run until we find the end of the block format or we hit a % which usually starts a placeholder for an index
    for (size_t i = 0; ('\0' != D::blockFormat[i]) && ('%' != D::blockFormat[i]); ++i)
    {
      if (name[i] != D::blockFormat[i])
      {
        return true;
      }
    }
    return false;
  }
  static dxil::HashValue decode_hash(const DataBlock &block, const T &value)
  {
    if (block.paramExists("hash"))
    {
      return dxil::HashValue::fromString(block.getStr("hash"));
    }
    else if (is_hash_block_name(block.getBlockName()))
    {
      return dxil::HashValue::fromString(block.getBlockName());
    }
    else
    {
      return dxil::HashValue::calculate(&value, 1);
    }
  }
};

class InputLayoutDeEncoder : FormatedDecoder,
                             public DefaultHashNameInvokeDecoder<InputLayout, InputLayoutDeEncoder>,
                             public EncoderBlockNameStore<InputLayoutDeEncoder>,
                             public EncoderBlockHashNameStore<InputLayout>,
                             public ObjectHashDecoder<InputLayout, InputLayoutDeEncoder>
{
  bool decodeDX12(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const;
  bool decodeEngine(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const;

public:
  static inline const char *blockFormat = "il_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const;
  bool encode(DataBlock &blk, const InputLayout &source) const;
};

using InputLayoutDecoder = InputLayoutDeEncoder;
using InputLayoutEncoder = InputLayoutDeEncoder;

class RenderStateDeEncoder : FormatedDecoder,
                             public DefaltInvokeDecoder<RenderStateSystem::StaticState, RenderStateDeEncoder>,
                             public EncoderBlockNameStore<RenderStateDeEncoder>,
                             public EncoderBlockHashNameStore<RenderStateSystem::StaticState>,
                             public ObjectHashDecoder<RenderStateSystem::StaticState, RenderStateDeEncoder>
{
  bool decodeDX12(const DataBlock &blk, RenderStateSystem::StaticState &target, dxil::HashValue &hash) const;
  bool decodeEngine(const DataBlock &blk, shaders::RenderState &target) const;

public:
  static inline const char *blockFormat = "rs_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const DataBlock &blk, RenderStateSystem::StaticState &target, dxil::HashValue &hash) const;
  bool encode(DataBlock &blk, const RenderStateSystem::StaticState &source) const;
};

using RenderStateDecoder = RenderStateDeEncoder;
using RenderStateEncoder = RenderStateDeEncoder;

class FramebufferLayoutDeEncoder : FormatedDecoder,
                                   public DefaltInvokeDecoder<FramebufferLayout, FramebufferLayoutDeEncoder>,
                                   public EncoderBlockNameStore<FramebufferLayoutDeEncoder>,
                                   public EncoderBlockHashNameStore<FramebufferLayout>,
                                   public ObjectHashDecoder<FramebufferLayout, FramebufferLayoutDeEncoder>
{
  bool decodeDX12(const DataBlock &blk, FramebufferLayout &target, dxil::HashValue &hash) const;
  bool decodeEngine(const DataBlock &blk, FramebufferLayout &target) const;

public:
  static inline const char *blockFormat = "fb_%u";

  using FormatedDecoder::FormatedDecoder;

  bool decode(const DataBlock &blk, FramebufferLayout &target, dxil::HashValue &hash) const;
  bool encode(DataBlock &blk, const FramebufferLayout &source) const;
};

using FramebufferLayoutDecoder = FramebufferLayoutDeEncoder;
using FramebufferLayoutEncoder = FramebufferLayoutDeEncoder;

class GraphicsPipelineVariantDeEncoder : public EncoderBlockNameStore<GraphicsPipelineVariantDeEncoder>
{
public:
  static inline const char *blockFormat = "v_%u";

  GraphicsPipelineVariantDeEncoder() = default;

  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    GraphicsPipelineVariantState out;
    if (!decode(blk, out))
    {
      return false;
    }
    return target(out);
  }
  bool decode(const DataBlock &blk, GraphicsPipelineVariantState &target) const;
  bool encode(DataBlock &blk, const GraphicsPipelineVariantState &source, int feature_set_id = -1) const;
};
using GraphicsPipelineVariantDecoder = GraphicsPipelineVariantDeEncoder;
using GraphicsPipelineVariantEncoder = GraphicsPipelineVariantDeEncoder;

class GraphicsPipelineDeEncoder : public EncoderBlockNameStore<GraphicsPipelineDeEncoder>
{
public:
  static inline const char *blockFormat = "gp_%u";

  GraphicsPipelineDeEncoder() = default;
  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    BasePipelineIdentifier base;
    DynamicArray<GraphicsPipelineVariantState> vars;
    if (!decode(blk, base, vars))
    {
      return false;
    }
    return target(base, vars);
  }
  template <typename VC>
  bool decode(const DataBlock &blk, BasePipelineIdentifier &base, VC &variants) const
  {
    auto vertexHash = blk.getStr("vertexHash", nullptr);
    auto pixelHash = blk.getStr("pixelHash", nullptr);
    if (!vertexHash || !pixelHash)
    {
      return false;
    }

    base.vs = dxil::HashValue::fromString(vertexHash);
    base.ps = dxil::HashValue::fromString(pixelHash);

    DataBlockDecodeEnumarator<GraphicsPipelineVariantDecoder> enumerator{blk, 0};
    variants.resize(enumerator.remaining());
    uint32_t enumerated = 0;
    for (; !enumerator.completed(); enumerator.next())
    {
      if (enumerator.decode(variants[enumerated]))
      {
        ++enumerated;
      }
    }
    variants.resize(enumerated);
    return enumerated > 0;
  }
  template <typename VC>
  bool encode(DataBlock &blk, const BasePipelineIdentifier &base, const VC &variants, int feature_set_id = -1) const
  {
    BasePipelineIdentifierHashSet hashSet = base;
    blk.setStr("vertexHash", hashSet.vsHash);
    blk.setStr("pixelHash", hashSet.psHash);
    DataBlockEncodeVisitor<GraphicsPipelineVariantEncoder> visitor{blk};
    for (auto &variant : variants)
    {
      visitor.encode(variant, feature_set_id);
    }
    return true;
  }
};

using GraphicsPipelineDecoder = GraphicsPipelineDeEncoder;
using GraphicsPipelineEncoder = GraphicsPipelineDeEncoder;

class MeshPipelineVariantDeEncoder : public DefaltInvokeDecoder<MeshPipelineVariantState, MeshPipelineVariantDeEncoder>,
                                     public EncoderBlockNameStore<GraphicsPipelineDeEncoder>
{
public:
  static inline const char *blockFormat = "v_%u";

  MeshPipelineVariantDeEncoder() = default;
  bool decode(const DataBlock &blk, MeshPipelineVariantState &target) const;
  bool encode(DataBlock &blk, const MeshPipelineVariantState &source, int feature_set_id = -1) const;
};

using MeshPipelineVariantDecoder = MeshPipelineVariantDeEncoder;
using MeshPipelineVariantEncoder = MeshPipelineVariantDeEncoder;

class MeshPipelineDeEncoder : public EncoderBlockNameStore<GraphicsPipelineDeEncoder>
{
public:
  static inline const char *blockFormat = "mp_%u";

  MeshPipelineDeEncoder() = default;

  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    BasePipelineIdentifier base;
    DynamicArray<MeshPipelineVariantState> vars;
    if (!decode(blk, base, vars))
    {
      return false;
    }
    return target(base, vars);
  }
  template <typename VC>
  bool decode(const DataBlock &blk, BasePipelineIdentifier &base, VC &variants) const
  {
    auto meshHash = blk.getStr("meshHash", nullptr);
    auto pixelHash = blk.getStr("pixelHash", nullptr);
    if (!meshHash || !pixelHash)
    {
      return false;
    }

    base.vs = dxil::HashValue::fromString(meshHash);
    base.ps = dxil::HashValue::fromString(pixelHash);

    DataBlockDecodeEnumarator<MeshPipelineVariantDecoder> enumerator{blk, 0};
    variants.resize(enumerator.remaining());
    uint32_t enumerated = 0;
    for (; !enumerator.completed(); enumerator.next())
    {
      if (enumerator.decode(variants[enumerated]))
      {
        ++enumerated;
      }
    }
    variants.resize(enumerated);
    return enumerated > 0;
  }
  template <typename VC>
  bool encode(DataBlock &blk, const BasePipelineIdentifier &base, const VC &variants, int feature_set_id = -1) const
  {
    BasePipelineIdentifierHashSet hashSet = base;
    blk.setStr("meshHash", hashSet.vsHash);
    blk.setStr("pixelHash", hashSet.psHash);
    DataBlockEncodeVisitor<MeshPipelineVariantEncoder> visitor{blk};
    for (auto &variant : variants)
    {
      visitor.encode(variant, feature_set_id);
    }
    return true;
  }
};

using MeshPipelineDecoder = MeshPipelineDeEncoder;
using MeshPipelineEncoder = MeshPipelineDeEncoder;

class ComputePipelineDeEncoder : public DefaltInvokeDecoder<ComputePipelineIdentifier, ComputePipelineDeEncoder>,
                                 public EncoderBlockNameStore<ComputePipelineDeEncoder>
{
public:
  static inline const char *blockFormat = "cp_%u";
  ComputePipelineDeEncoder() = default;
  bool decode(const DataBlock &blk, ComputePipelineIdentifier &target) const;
  bool encode(DataBlock &blk, const ComputePipelineIdentifier &target, int feature_set_id = -1) const;
};

using ComputePipelineDecoder = ComputePipelineDeEncoder;
using ComputePipelineEncoder = ComputePipelineDeEncoder;

class SignatueHashDeEncoder : public DefaltInvokeDecoder<cacheBlk::SignatureHash, SignatueHashDeEncoder>
{
public:
  SignatueHashDeEncoder() = default;
  bool decode(const DataBlock &blk, uint32_t param_index, cacheBlk::SignatureHash &target) const;
  bool decode(const DataBlock &blk, cacheBlk::SignatureHash &target) const;
  bool encode(DataBlock &blk, const cacheBlk::SignatureHash &source) const;
  bool encode(DataBlock &blk, const char *name, const char *value, int64_t timestamp) const;
};

using SignatueHashDecoder = SignatueHashDeEncoder;
using SignatueHashEncoder = SignatueHashDeEncoder;

class UseDeEnCoder : public DefaltInvokeDecoder<uint64_t, UseDeEnCoder>
{
public:
  UseDeEnCoder() = default;
  bool decode(const DataBlock &blk, uint32_t param_index, uint32_t &static_code, uint32_t &dynamic_code) const;
  bool decode(const DataBlock &blk, uint32_t param_index, cacheBlk::UseCodes &codes) const
  {
    return decode(blk, param_index, codes.staticCode, codes.dynamicCode);
  }
  bool encode(DataBlock &blk, uint32_t static_code, uint32_t dynamic_code) const;
  bool encode(DataBlock &blk, cacheBlk::UseCodes codes) const { return encode(blk, codes.staticCode, codes.dynamicCode); }
};

using UseEnCoder = UseDeEnCoder;
using UseDeCoder = UseDeEnCoder;

class ShaderClassUseDeEncoder
{
public:
  ShaderClassUseDeEncoder() = default;
  template <typename VC>
  bool decode(const DataBlock &blk, eastl::string &name, VC &uses) const
  {
    name = blk.getBlockName();
    uses.resize(blk.paramCount());
    uint32_t enumerated = 0;
    DataBlockParameterDecodeEnumarator<UseDeCoder> useDecoder{blk, 0};
    for (; !useDecoder.completed(); useDecoder.next())
    {
      if (useDecoder.decode(uses[enumerated]))
      {
        enumerated++;
      }
    }
    uses.resize(enumerated);
    return enumerated > 0;
  }
  template <typename VC>
  bool encode(DataBlock &blk, const char *name, const VC &codes) const
  {
    auto classBlock = blk.addNewBlock(name);
    if (!classBlock)
    {
      return false;
    }
    UseEnCoder useEncoder{};
    for (auto &use : codes)
    {
      useEncoder.encode(*classBlock, use);
    }
    return true;
  }
};

using ShaderClassUseEncoder = ShaderClassUseDeEncoder;
using ShaderClassUseDecoder = ShaderClassUseDeEncoder;

class ComputeVariantDeEncoder : public EncoderBlockNameStore<ComputeVariantDeEncoder>
{
public:
  static inline const char *blockFormat = "variant";

  ComputeVariantDeEncoder() = default;

  template <typename T>
  bool invoke(const DataBlock &blk, T target) const
  {
    cacheBlk::SignatureMask signatureMask = 0;
    auto signatureId = blk.getNameId("signature");
    if (-1 != signatureId)
    {
      auto paramIndex = blk.findParam(signatureId);
      while (-1 != paramIndex)
      {
        signatureMask |= 1u << blk.getInt(paramIndex);
        paramIndex = blk.findParam(signatureId, paramIndex);
      }
    }

    DataBlockDecodeEnumarator<ShaderClassUseDecoder> decoder{blk, 0};
    for (; !decoder.completed(); decoder.next())
    {
      eastl::string name;
      DynamicArray<cacheBlk::UseCodes> codes;
      if (decoder.decode(name, codes))
      {
        target(eastl::move(name), signatureMask, eastl::move(codes));
      }
    }
    return true;
  }
  template <typename T>
  bool encode(DataBlock &blk, const T &classes, cacheBlk::SignatureMask signature_mask = 0)
  {
    if (0 != signature_mask)
    {
      for (auto &&i : LsbVisitor{signature_mask})
      {
        blk.addInt("signature", i);
      }
    }

    pipeline::ShaderClassUseEncoder classUseEncoder{};
    for (auto &c : classes)
    {
      classUseEncoder.encode(blk, c.name, c.uses);
    }
    return true;
  }
};

using ComputeVariantEncoder = ComputeVariantDeEncoder;
using ComputeVariantDecoder = ComputeVariantDeEncoder;

class GraphicsVariantDeEncoder : public EncoderBlockNameStore<GraphicsVariantDeEncoder>
{
public:
  static inline const char *blockFormat = "variant";

  GraphicsVariantDeEncoder() = default;

  template <typename T>
  bool invoke(const DataBlock &blk, T &handler) const
  {
    cacheBlk::SignatureMask signatureMask = 0;
    auto signatureId = blk.getNameId("signature");
    if (-1 != signatureId)
    {
      auto paramIndex = blk.findParam(signatureId);
      while (-1 != paramIndex)
      {
        signatureMask |= 1u << blk.getInt(paramIndex);
        paramIndex = blk.findParam(signatureId, paramIndex);
      }
    }

    bool isWF = blk.getBool("wireFrame", false);
    int topology = blk.getInt("topology", 0);

    int inputLayout = -1;
    if (blk.paramExists("inputLayoutHash"))
    {
      inputLayout = handler.translateInputLayoutHashString(blk.getStr("inputLayoutHash", nullptr));
    }
    else if (blk.paramExists("inputLayout"))
    {
      inputLayout = blk.getInt("inputLayout", -1);
    }

    int staticRenderState = -1;
    if (blk.paramExists("staticRenderStateHash"))
    {
      staticRenderState = handler.translateStaticRenderStateHashString(blk.getStr("staticRenderStateHash", nullptr));
    }
    else
    {
      staticRenderState = blk.getInt("staticRenderState", -1);
    }
    int frameBufferLayout = -1;
    if (blk.paramExists("frameBufferLayoutHash"))
    {
      frameBufferLayout = handler.translateFrambufferLayoutHashString(blk.getStr("frameBufferLayoutHash", nullptr));
    }
    else
    {
      frameBufferLayout = blk.getInt("frameBuferLayout", -1);
    }
    DataBlockDecodeEnumarator<ShaderClassUseDecoder> decoder{blk, 0};
    handler.onVariantBegin(inputLayout, staticRenderState, frameBufferLayout, topology, isWF, signatureMask);
    handler.onMaxClasses(decoder.remaining());
    for (; !decoder.completed(); decoder.next())
    {
      eastl::string name;
      DynamicArray<cacheBlk::UseCodes> codes;
      if (decoder.decode(name, codes))
      {
        handler.onClass(eastl::move(name), eastl::move(codes));
      }
    }
    handler.onVariantEnd();
    return true;
  }
  template <typename T, typename U, typename V, typename W, typename X>
  bool encode(DataBlock &blk, const T &properties, const U &classes, V input_layout_hash_str, W static_render_state_hash_str,
    X framebuffer_layout_hash_str, cacheBlk::SignatureMask signature_mask = 0)
  {
    if (0 != signature_mask)
    {
      for (auto &&i : LsbVisitor{signature_mask})
      {
        blk.addInt("signature", i);
      }
    }

    if (properties.isWF)
    {
      blk.setBool("wireFrame", true);
    }
    if (properties.topology != 0)
    {
      blk.setInt("topology", properties.topology);
    }
    if (properties.inputLayout != decltype(properties.inputLayout)::Null())
    {
      blk.setStr("inputLayoutHash", input_layout_hash_str(properties.inputLayout));
    }
    if (properties.staticRenderState != decltype(properties.staticRenderState)::Null())
    {
      blk.setStr("staticRenderStateHash", static_render_state_hash_str(properties.staticRenderState));
    }
    if (properties.frameBufferLayout != decltype(properties.frameBufferLayout)::Null())
    {
      blk.setStr("frameBufferLayoutHash", framebuffer_layout_hash_str(properties.frameBufferLayout));
    }

    pipeline::ShaderClassUseEncoder classUseEncoder{};
    for (auto &c : classes)
    {
      classUseEncoder.encode(blk, c.name, c.uses);
    }
    return true;
  }
};

using GraphicsVariantEncoder = GraphicsVariantDeEncoder;
using GraphicsVariantDecoder = GraphicsVariantDeEncoder;
} // namespace pipeline
} // namespace drv3d_dx12