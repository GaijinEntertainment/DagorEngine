// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_cache.h"
#include <device.h>


using namespace drv3d_dx12;

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decodeDX12(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const
{
  if (auto vertexAttributeSetBlk = blk.getBlockByName("vertexAttributeSet"))
  {
    target.vertexAttributeLocationMask = vertexAttributeSetBlk->getInt("locationMask", 0);
    target.vertexAttributeLocationSourceStream = vertexAttributeSetBlk->getInt64("locationSourceStream", 0);
    auto count = min<uint32_t>(MAX_SEMANTIC_INDEX, vertexAttributeSetBlk->blockCount());
    for (uint32_t locationIndex = 0, locationBlockIndex = 0; locationIndex < MAX_SEMANTIC_INDEX && locationBlockIndex < count;
         ++locationIndex)
    {
      if (0 == (target.vertexAttributeLocationMask & (1u << locationIndex)))
      {
        continue;
      }

      auto compactedFormatAndOffset = vertexAttributeSetBlk->getBlock(locationBlockIndex++);
      if (compactedFormatAndOffset->paramExists("compactedFormatAndOffset"))
      {
        target.vertexAttributeCompactedFormatAndOffset[locationIndex] =
          compactedFormatAndOffset->getInt("compactedFormatAndOffset", 0);
      }
      else
      {
        if (compactedFormatAndOffset->paramExists("offset"))
        {
          target.setLocationStreamOffset(locationIndex, compactedFormatAndOffset->getInt("offset", 0));
        }
        if (compactedFormatAndOffset->paramExists("formatIndex"))
        {
          target.setLocationFormatIndex(locationIndex, compactedFormatAndOffset->getInt("formatIndex", 0));
        }
      }
    }
  }
  if (auto imputStreamSetBlk = blk.getBlockByName("inputStreamSet"))
  {
    target.inputStreamUsageMask = imputStreamSetBlk->getInt("usageMask", 0);
    target.inputStreamStepRateMask = imputStreamSetBlk->getInt("stepRateMask", 0);
  }

  hash = decode_hash(blk, target);
  return true;
}

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decodeEngine(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const
{
  G_UNUSED(hash);
  InputLayout::DecodeContext ctx;
  uint32_t pc = blk.paramCount();
  for (uint32_t i = 0; i < pc; ++i)
  {
    if (!target.fromVdecl(ctx, blk.getInt(i)))
    {
      break;
    }
  }

  hash = dxil::HashValue::calculate(&target, 1);
  return true;
}

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decode(const DataBlock &blk, InputLayout &target, dxil::HashValue &hash) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target, hash);
  }
  return decodeEngine(blk, target, hash);
}

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::encode(DataBlock &blk, const InputLayout &source) const
{
  constexpr bool use_combined_format = false;
  auto vertexAttributeSetBlk = blk.addNewBlock("vertexAttributeSet");
  auto imputStreamSetBlk = blk.addNewBlock("inputStreamSet");
  if (!vertexAttributeSetBlk || !imputStreamSetBlk)
  {
    return false;
  }
  vertexAttributeSetBlk->setInt("locationMask", source.vertexAttributeLocationMask);
  vertexAttributeSetBlk->setInt64("locationSourceStream", source.vertexAttributeLocationSourceStream);
  for (uint32_t locationIndex = 0; locationIndex < MAX_SEMANTIC_INDEX; ++locationIndex)
  {
    if (0 == (source.vertexAttributeLocationMask & (1u << locationIndex)))
    {
      continue;
    }

    char blockName[32];
    sprintf_s(blockName, "l_%x", (1u << locationIndex));
    auto compactedFormatAndOffset = vertexAttributeSetBlk->addNewBlock(blockName);
    if (!compactedFormatAndOffset)
    {
      return false;
    }
    if (use_combined_format)
    {
      compactedFormatAndOffset->setInt("compactedFormatAndOffset", source.vertexAttributeCompactedFormatAndOffset[locationIndex]);
    }
    else
    {
      compactedFormatAndOffset->setInt("offset", source.getLocationStreamOffset(locationIndex));
      compactedFormatAndOffset->setInt("formatIndex", source.getLocationFormatIndex(locationIndex));
    }
  }
  imputStreamSetBlk->setInt("usageMask", source.inputStreamUsageMask);
  imputStreamSetBlk->setInt("stepRateMask", source.inputStreamStepRateMask);
  return true;
}

bool drv3d_dx12::pipeline::RenderStateDeEncoder::decodeDX12(const DataBlock &blk, RenderStateSystem::StaticState &target,
  dxil::HashValue &hash) const
{
  target.enableDepthTest = blk.getBool("enableDepthTest", target.enableDepthTest);
  target.enableDepthWrite = blk.getBool("enableDepthWrite", target.enableDepthWrite);
  target.enableDepthClip = blk.getBool("enableDepthClip", target.enableDepthClip);
  target.enableDepthBounds = blk.getBool("enableDepthBounds", target.enableDepthBounds);
  target.enableStencil = blk.getBool("enableStencil", target.enableStencil);
  target.enableIndependentBlend = blk.getBool("enableIndependentBlend", target.enableIndependentBlend);
  target.enableDualSourceBlending = blk.getBool("enableDualSourceBlending", target.enableDualSourceBlending);
  target.enableAlphaToCoverage = blk.getBool("enableAlphaToCoverage", target.enableAlphaToCoverage);
  target.depthFunc = blk.getInt("depthFunc", target.depthFunc);
  target.forcedSampleCountShift = blk.getInt("forcedSampleCountShift", target.forcedSampleCountShift);
  target.enableConservativeRaster = blk.getBool("enableConservativeRaster", target.enableConservativeRaster);
  target.viewInstanceCount = blk.getInt("viewInstanceCount", target.viewInstanceCount);
  target.cullMode = blk.getInt("cullMode", target.cullMode);
  target.stencilReadMask = blk.getInt("stencilReadMask", target.stencilReadMask);
  target.stencilWriteMask = blk.getInt("stencilWriteMask", target.stencilWriteMask);
  target.stencilFunction = blk.getInt("stencilFunction", target.stencilFunction);
  target.stencilOnFail = blk.getInt("stencilOnFail", target.stencilOnFail);
  target.stencilOnDepthFail = blk.getInt("stencilOnDepthFail", target.stencilOnDepthFail);
  target.stencilOnPass = blk.getInt("stencilOnPass", target.stencilOnPass);
  target.colorWriteMask = blk.getInt("colorWriteMask", target.colorWriteMask);
  target.depthBias = blk.getReal("depthBias", target.depthBias);
  target.depthBiasSloped = blk.getReal("depthBiasSloped", target.depthBiasSloped);
  if (auto blendParamsBlk = blk.getBlockByName("blendParams"))
  {
    auto blendParamsBlkCount = min<uint32_t>(blendParamsBlk->blockCount(), shaders::RenderState::NumIndependentBlendParameters);
    for (uint32_t bbi = 0; bbi < blendParamsBlkCount; ++bbi)
    {
      auto blendBlk = blendParamsBlk->getBlock(bbi);
      if (!blendBlk)
      {
        continue;
      }

      auto fillParam = [blendBlk](auto &param) {
        param.blendFactors.Source = blendBlk->getInt("blendFactorsSource", param.blendFactors.Source);
        param.blendFactors.Destination = blendBlk->getInt("blendFactorsDestination", param.blendFactors.Destination);

        param.blendAlphaFactors.Source = blendBlk->getInt("blendAlphaFactorsSource", param.blendAlphaFactors.Source);
        param.blendAlphaFactors.Destination = blendBlk->getInt("blendAlphaFactorsDestination", param.blendAlphaFactors.Destination);

        param.blendFunction = blendBlk->getInt("blendFunction", param.blendFunction);
        param.blendAlphaFunction = blendBlk->getInt("blendAlphaFunction", param.blendAlphaFunction);
        param.enableBlending = blendBlk->getBool("enableBlending", param.enableBlending);
      };

      if (target.enableDualSourceBlending)
        fillParam(target.dualSourceBlend.params);
      else
        fillParam(target.blendParams[bbi]);
    }
  }

  hash = decode_hash(blk, target);
  return true;
}

bool drv3d_dx12::pipeline::RenderStateDeEncoder::decodeEngine(const DataBlock &blk, shaders::RenderState &target) const
{
  target.zwrite = blk.getBool("zwrite", target.zwrite);
  target.ztest = blk.getBool("ztest", target.ztest);
  target.zFunc = blk.getInt("zFunc", target.zFunc);
  target.stencilRef = blk.getInt("stencilRef", target.stencilRef);
  target.cull = blk.getInt("cull", target.cull);
  // optional
  target.depthBoundsEnable = blk.getBool("depthBoundsEnable", target.depthBoundsEnable);
  target.forcedSampleCount = blk.getInt("forcedSampleCount", target.forcedSampleCount);
  // optional
  target.conservativeRaster = blk.getBool("conservativeRaster", target.conservativeRaster);
  target.zClip = blk.getBool("zClip", target.zClip);
  target.scissorEnabled = blk.getBool("scissorEnabled", target.scissorEnabled);
  target.independentBlendEnabled = blk.getBool("independentBlendEnabled", target.independentBlendEnabled);
  target.alphaToCoverage = blk.getBool("alphaToCoverage", target.alphaToCoverage);
  target.viewInstanceCount = blk.getInt("viewInstanceCount", target.viewInstanceCount);
  target.colorWr = blk.getInt("colorWr", target.colorWr);
  target.zBias = blk.getReal("zBias", target.zBias);
  target.slopeZBias = blk.getReal("slopeZBias", target.slopeZBias);
  if (auto stencilBlk = blk.getBlockByName("stencil"))
  {
    target.stencil.func = stencilBlk->getInt("func", target.stencil.func);
    target.stencil.fail = stencilBlk->getInt("fail", target.stencil.fail);
    target.stencil.zFail = stencilBlk->getInt("zFail", target.stencil.zFail);
    target.stencil.pass = stencilBlk->getInt("pass", target.stencil.pass);
    target.stencil.readMask = stencilBlk->getInt("readMask", target.stencil.readMask);
    target.stencil.writeMask = stencilBlk->getInt("writeMask", target.stencil.writeMask);
  }
  if (auto blendParamsBlk = blk.getBlockByName("blendParams"))
  {
    auto blendParamsBlkCount = min<uint32_t>(blendParamsBlk->blockCount(), shaders::RenderState::NumIndependentBlendParameters);
    for (uint32_t bbi = 0; bbi < blendParamsBlkCount; ++bbi)
    {
      auto blendBlk = blendParamsBlk->getBlock(bbi);
      if (!blendBlk)
      {
        continue;
      }
      auto &param = target.blendParams[bbi];

      param.ablendFactors.src = blendBlk->getInt("ablendFactorsSrc", param.ablendFactors.src);
      param.ablendFactors.dst = blendBlk->getInt("ablendFactorsDst", param.ablendFactors.dst);

      param.sepablendFactors.src = blendBlk->getInt("sepablendFactorsSrc", param.sepablendFactors.src);
      param.sepablendFactors.dst = blendBlk->getInt("sepablendFactorsDst", param.sepablendFactors.dst);

      param.blendOp = blendBlk->getInt("blendOp", param.blendOp);
      param.sepablendOp = blendBlk->getInt("sepablendOp", param.sepablendOp);
      param.ablend = blendBlk->getBool("ablend", param.ablend);
      param.sepablend = blendBlk->getBool("sepablend", param.sepablend);
    }
  }
  return true;
}

bool drv3d_dx12::pipeline::RenderStateDeEncoder::decode(const DataBlock &blk, RenderStateSystem::StaticState &target,
  dxil::HashValue &hash) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target, hash);
  }

  shaders::RenderState engineValue;
  if (!decodeEngine(blk, engineValue))
  {
    return false;
  }
  target = RenderStateSystem::StaticState::fromRenderState(engineValue);
  hash = dxil::HashValue::calculate(&target, 1);
  return true;
}

bool drv3d_dx12::pipeline::RenderStateDeEncoder::encode(DataBlock &blk, const RenderStateSystem::StaticState &source) const
{
  auto blendParamsBlk = blk.addNewBlock("blendParams");
  if (!blendParamsBlk)
  {
    return false;
  }
  blk.setBool("enableDepthTest", source.enableDepthTest);
  blk.setBool("enableDepthWrite", source.enableDepthWrite);
  blk.setBool("enableDepthClip", source.enableDepthClip);
  blk.setBool("enableDepthBounds", source.enableDepthBounds);
  blk.setBool("enableStencil", source.enableStencil);
  blk.setBool("enableIndependentBlend", source.enableIndependentBlend);
  blk.setBool("enableDualSourceBlending", source.enableDualSourceBlending);
  blk.setBool("enableAlphaToCoverage", source.enableAlphaToCoverage);
  blk.setInt("depthFunc", source.depthFunc);
  blk.setInt("forcedSampleCountShift", source.forcedSampleCountShift);
  blk.setBool("enableConservativeRaster", source.enableConservativeRaster);
  blk.setInt("viewInstanceCount", source.viewInstanceCount);
  blk.setInt("cullMode", source.cullMode);
  blk.setInt("stencilReadMask", source.stencilReadMask);
  blk.setInt("stencilWriteMask", source.stencilWriteMask);
  blk.setInt("stencilFunction", source.stencilFunction);
  blk.setInt("stencilOnFail", source.stencilOnFail);
  blk.setInt("stencilOnDepthFail", source.stencilOnDepthFail);
  blk.setInt("stencilOnPass", source.stencilOnPass);
  blk.setInt("colorWriteMask", source.colorWriteMask);
  blk.setReal("depthBias", source.depthBias);
  blk.setReal("depthBiasSloped", source.depthBiasSloped);

  char buf[32];
  for (uint32_t bbi = 0; bbi < shaders::RenderState::NumIndependentBlendParameters; ++bbi)
  {
    sprintf_s(buf, "bp_%u", bbi);
    auto blendBlk = blendParamsBlk->addNewBlock(buf);
    if (!blendBlk)
    {
      return false;
    }

    auto writeParamOut = [blendBlk](const auto &param) {
      blendBlk->setInt("blendFactorsSource", param.blendFactors.Source);
      blendBlk->setInt("blendFactorsDestination", param.blendFactors.Destination);

      blendBlk->setInt("blendAlphaFactorsSource", param.blendAlphaFactors.Source);
      blendBlk->setInt("blendAlphaFactorsDestination", param.blendAlphaFactors.Destination);

      blendBlk->setInt("blendFunction", param.blendFunction);
      blendBlk->setInt("blendAlphaFunction", param.blendAlphaFunction);
      blendBlk->setBool("enableBlending", param.enableBlending);
    };

    if (source.enableDualSourceBlending)
      writeParamOut(source.dualSourceBlend.params);
    else
      writeParamOut(source.blendParams[bbi]);
  }
  return true;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::decodeDX12(const DataBlock &blk, FramebufferLayout &target,
  dxil::HashValue &hash) const
{
  auto formatNameId = blk.getNameId("colorFormats");
  if ((-1 != formatNameId) && blk.paramExists(formatNameId))
  {
    uint8_t targetMask = blk.getInt("colorTargetMask", 0xFF);
    int lastParamIndex = blk.findParam(formatNameId);
    while (lastParamIndex != -1)
    {
      FormatStore fmt(blk.getInt(lastParamIndex));
      for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      {
        auto bit = 1u << i;
        if (0 == (bit & targetMask))
        {
          continue;
        }
        target.colorTargetMask |= bit;
        targetMask ^= bit;
        target.colorFormats[i] = fmt;
        break;
      }

      lastParamIndex = blk.findParam(formatNameId, lastParamIndex);
    }
    target.colorMsaaLevels = blk.getInt("colorMsaaLevels", 0);
  }
  if (blk.paramExists("depthStencilFormat"))
    target.setDepthStencilAttachment(blk.getInt("depthMsaaLevel", 0), FormatStore(blk.getInt("depthStencilFormat", 0)));

  hash = decode_hash(blk, target);
  return true;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::decodeEngine(const DataBlock &blk, FramebufferLayout &target) const
{
  auto formatNameId = blk.getNameId("colorFormats");
  if ((-1 != formatNameId) && blk.paramExists(formatNameId))
  {
    uint8_t targetMask = blk.getInt("colorTargetMask", 0xFF);
    int lastParamIndex = blk.findParam(formatNameId);
    while (lastParamIndex != -1)
    {
      auto fmt = FormatStore::fromCreateFlags(blk.getInt(lastParamIndex));
      for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      {
        auto bit = 1u << i;
        if (0 == (bit & targetMask))
        {
          continue;
        }
        target.colorTargetMask |= bit;
        targetMask ^= bit;
        target.colorFormats[i] = fmt;
      }

      lastParamIndex = blk.findParam(formatNameId, lastParamIndex);
    }
    target.colorMsaaLevels = blk.getInt("colorMsaaLevels", 0);
  }
  if (blk.paramExists("depthStencilFormat"))
  {
    const FormatStore depthStencilFormat = FormatStore::fromCreateFlags(blk.getInt("depthStencilFormat", 0));
    target.setDepthStencilAttachment(blk.getInt("depthMsaaLevel", 0), depthStencilFormat);
  }
  return true;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::decode(const DataBlock &blk, FramebufferLayout &target,
  dxil::HashValue &hash) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target, hash);
  }
  if (decodeEngine(blk, target))
  {
    hash = dxil::HashValue::calculate(&target, 1);
    return true;
  }
  return false;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::encode(DataBlock &blk, const FramebufferLayout &source) const
{
  if (source.colorTargetMask)
  {
    // TODO only needed when there are holes in the mask
    blk.setInt("colorTargetMask", source.colorTargetMask);
    for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    {
      if (0 == (source.colorTargetMask & (1u << i)))
      {
        continue;
      }
      blk.addInt("colorFormats", source.colorFormats[i].wrapper.value);
    }
    blk.addInt("colorMsaaLevels", source.colorMsaaLevels);
  }
  if (0 != source.hasDepth)
  {
    blk.setInt("depthStencilFormat", source.depthStencilFormat.wrapper.value);
    blk.setInt("depthMsaaLevel", source.depthMsaaLevel);
  }
  return true;
}

bool drv3d_dx12::pipeline::GraphicsPipelineVariantDeEncoder::decode(const DataBlock &blk, GraphicsPipelineVariantState &target) const
{
  if (!blk.paramExists("renderState") || !blk.paramExists("outputFormat") || !blk.paramExists("inputLayout") ||
      !blk.paramExists("primitiveTopology"))
  {
    return false;
  }
  target.staticRenderStateIndex = blk.getInt("renderState", 0);
  target.framebufferLayoutIndex = blk.getInt("outputFormat", 0);
  target.inputLayoutIndex = blk.getInt("inputLayout", 0);
  target.isWireFrame = blk.getBool("wireFrame", false);
  target.topology = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(blk.getInt("primitiveTopology", 0));
  return true;
}

bool drv3d_dx12::pipeline::GraphicsPipelineVariantDeEncoder::encode(DataBlock &blk, const GraphicsPipelineVariantState &source,
  int feature_set_id) const
{
  if (-1 != feature_set_id)
  {
    blk.addInt("featureSet", feature_set_id);
  }
  blk.setInt("renderState", source.staticRenderStateIndex);
  blk.setInt("outputFormat", source.framebufferLayoutIndex);
  blk.setInt("inputLayout", source.inputLayoutIndex);
  blk.setBool("wireFrame", 0 != source.isWireFrame);
  blk.setInt("primitiveTopology", static_cast<int>(source.topology));
  return true;
}

bool drv3d_dx12::pipeline::MeshPipelineVariantDeEncoder::decode(const DataBlock &blk, MeshPipelineVariantState &target) const
{
  if (!blk.paramExists("renderState") || !blk.paramExists("outputFormat"))
  {
    return false;
  }
  target.staticRenderStateIndex = blk.getInt("renderState", 0);
  target.framebufferLayoutIndex = blk.getInt("outputFormat", 0);
  target.isWireFrame = blk.getBool("wireFrame", false);
  return true;
}

bool drv3d_dx12::pipeline::MeshPipelineVariantDeEncoder::encode(DataBlock &blk, const MeshPipelineVariantState &source,
  int feature_set_id) const
{
  if (-1 != feature_set_id)
  {
    blk.addInt("featureSet", feature_set_id);
  }
  blk.setInt("renderState", source.staticRenderStateIndex);
  blk.setInt("outputFormat", source.framebufferLayoutIndex);
  blk.setBool("wireFrame", 0 != source.isWireFrame);
  return true;
}

bool drv3d_dx12::pipeline::ComputePipelineDeEncoder::decode(const DataBlock &blk, ComputePipelineIdentifier &target) const
{
  auto hash = blk.getStr("hash", nullptr);
  if (!hash)
  {
    return false;
  }
  target.hash = dxil::HashValue::fromString(hash);
  return true;
}

bool drv3d_dx12::pipeline::ComputePipelineDeEncoder::encode(DataBlock &blk, const ComputePipelineIdentifier &source,
  int feature_set_id) const
{
  ComputePipelineIdentifierHashSet set = source;
  if (-1 != feature_set_id)
  {
    blk.addInt("featureSet", feature_set_id);
  }
  blk.setStr("hash", set.hash);
  return true;
}

bool drv3d_dx12::pipeline::SignatueHashDeEncoder::decode(const DataBlock &blk, uint32_t param_index,
  cacheBlk::SignatureHash &target) const
{
  if (blk.paramCount() <= param_index)
  {
    return false;
  }

  if (DataBlock::TYPE_STRING != blk.getParamType(param_index))
  {
    return false;
  }

  target.name = blk.getParamName(param_index);
  target.hash = blk.getStr(param_index);
  target.timestamp = 0;
  return true;
}

bool drv3d_dx12::pipeline::SignatueHashDeEncoder::decode(const DataBlock &blk, cacheBlk::SignatureHash &target) const
{
  auto hashParamIndex = blk.findParam("hash");
  if (DataBlock::TYPE_STRING != blk.getParamType(hashParamIndex))
    return false;
  auto stampParamIndex = blk.findParam("stamp");
  if (DataBlock::TYPE_INT64 != blk.getParamType(stampParamIndex))
    return false;

  target.name = blk.getBlockName();
  target.hash = blk.getStr(hashParamIndex);
  target.timestamp = blk.getInt64(stampParamIndex);
  return true;
}

bool drv3d_dx12::pipeline::SignatueHashDeEncoder::encode(DataBlock &blk, const cacheBlk::SignatureHash &source) const
{
  auto subBlk = blk.addNewBlock(source.name.c_str());
  if (!subBlk)
    return false;
  subBlk->setStr("hash", source.hash.c_str());
  subBlk->setInt64("stamp", source.timestamp);
  return true;
}

bool drv3d_dx12::pipeline::SignatueHashDeEncoder::encode(DataBlock &blk, const char *name, const char *hash, int64_t timestamp) const
{
  auto subBlk = blk.addNewBlock(name);
  if (!subBlk)
    return false;
  subBlk->setStr("hash", hash);
  subBlk->setInt64("stamp", timestamp);
  return true;
}

bool drv3d_dx12::pipeline::UseDeEnCoder::decode(const DataBlock &blk, uint32_t param_index, uint32_t &static_code,
  uint32_t &dynamic_code) const
{
  if (blk.paramCount() <= param_index)
  {
    return false;
  }

  if (DataBlock::TYPE_INT64 != blk.getParamType(param_index))
  {
    return false;
  }

  auto raw = static_cast<uint64_t>(blk.getInt64(param_index));
  dynamic_code = static_cast<uint32_t>(raw >> 32);
  static_code = static_cast<uint32_t>(raw);
  return true;
}

bool drv3d_dx12::pipeline::UseDeEnCoder::encode(DataBlock &blk, uint32_t static_code, uint32_t dynamic_code) const
{
  blk.addInt64("use", (uint64_t{dynamic_code} << 32) | uint64_t{static_code});
  return true;
}
