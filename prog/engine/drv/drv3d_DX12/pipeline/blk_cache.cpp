#include "device.h"

using namespace drv3d_dx12;

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decodeDX12(const DataBlock &blk, InputLayout &target) const
{
  if (auto vertexAttributeSetBlk = blk.getBlockByName("vertexAttributeSet"))
  {
    target.vertexAttributeSet.locationMask = vertexAttributeSetBlk->getInt("locationMask", 0);
    target.vertexAttributeSet.locationSourceStream = vertexAttributeSetBlk->getInt64("locationSourceStream", 0);
    auto count = min<uint32_t>(MAX_SEMANTIC_INDEX, vertexAttributeSetBlk->blockCount());
    for (uint32_t locationIndex = 0, locationBlockIndex = 0; locationIndex < MAX_SEMANTIC_INDEX && locationBlockIndex < count;
         ++locationIndex)
    {
      if (0 == (target.vertexAttributeSet.locationMask & (1u << locationIndex)))
      {
        continue;
      }

      auto compactedFormatAndOffset = vertexAttributeSetBlk->getBlock(locationBlockIndex++);
      if (compactedFormatAndOffset->paramExists("compactedFormatAndOffset"))
      {
        target.vertexAttributeSet.compactedFormatAndOffset[locationIndex] =
          compactedFormatAndOffset->getInt("compactedFormatAndOffset", 0);
      }
      else
      {
        if (compactedFormatAndOffset->paramExists("offset"))
        {
          target.vertexAttributeSet.setLocationStreamOffset(locationIndex, compactedFormatAndOffset->getInt("offset", 0));
        }
        if (compactedFormatAndOffset->paramExists("formatIndex"))
        {
          target.vertexAttributeSet.setLocationFormatIndex(locationIndex, compactedFormatAndOffset->getInt("formatIndex", 0));
        }
      }
    }
  }
  if (auto imputStreamSetBlk = blk.getBlockByName("inputStreamSet"))
  {
    target.inputStreamSet.usageMask = imputStreamSetBlk->getInt("usageMask", 0);
    target.inputStreamSet.stepRateMask = imputStreamSetBlk->getInt("stepRateMask", 0);
  }
  return true;
}

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decodeEngine(const DataBlock &blk, InputLayout &target) const
{
  InputLayout::DecodeContext ctx;
  uint32_t pc = blk.paramCount();
  for (uint32_t i = 0; i < pc; ++i)
  {
    if (!target.fromVdecl(ctx, blk.getInt(i)))
    {
      break;
    }
  }
  return true;
}

bool drv3d_dx12::pipeline::InputLayoutDeEncoder::decode(const DataBlock &blk, InputLayout &target) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target);
  }
  return decodeEngine(blk, target);
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
  vertexAttributeSetBlk->setInt("locationMask", source.vertexAttributeSet.locationMask);
  vertexAttributeSetBlk->setInt64("locationSourceStream", source.vertexAttributeSet.locationSourceStream);
  for (uint32_t locationIndex = 0; locationIndex < MAX_SEMANTIC_INDEX; ++locationIndex)
  {
    if (0 == (source.vertexAttributeSet.locationMask & (1u << locationIndex)))
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
      compactedFormatAndOffset->setInt("compactedFormatAndOffset", source.vertexAttributeSet.compactedFormatAndOffset[locationIndex]);
    }
    else
    {
      compactedFormatAndOffset->setInt("offset", source.vertexAttributeSet.getLocationStreamOffset(locationIndex));
      compactedFormatAndOffset->setInt("formatIndex", source.vertexAttributeSet.getLocationFormatIndex(locationIndex));
    }
  }
  imputStreamSetBlk->setInt("usageMask", source.inputStreamSet.usageMask);
  imputStreamSetBlk->setInt("stepRateMask", source.inputStreamSet.stepRateMask);
  return true;
}

bool drv3d_dx12::pipeline::RenderStateDeEncoder::decodeDX12(const DataBlock &blk, RenderStateSystem::StaticState &target) const
{
  target.enableDepthTest = blk.getBool("enableDepthTest", target.enableDepthTest);
  target.enableDepthWrite = blk.getBool("enableDepthWrite", target.enableDepthWrite);
  target.enableDepthClip = blk.getBool("enableDepthClip", target.enableDepthClip);
  target.enableDepthBounds = blk.getBool("enableDepthBounds", target.enableDepthBounds);
  target.enableStencil = blk.getBool("enableStencil", target.enableStencil);
  target.enableIndependentBlend = blk.getBool("enableIndependentBlend", target.enableIndependentBlend);
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
      auto &param = target.blendParams[bbi];

      param.blendFactors.Source = blendBlk->getInt("blendFactorsSource", param.blendFactors.Source);
      param.blendFactors.Destination = blendBlk->getInt("blendFactorsDestination", param.blendFactors.Destination);

      param.blendAlphaFactors.Source = blendBlk->getInt("blendAlphaFactorsSource", param.blendAlphaFactors.Source);
      param.blendAlphaFactors.Destination = blendBlk->getInt("blendAlphaFactorsDestination", param.blendAlphaFactors.Destination);

      param.blendFunction = blendBlk->getInt("blendFunction", param.blendFunction);
      param.blendAlphaFunction = blendBlk->getInt("blendAlphaFunction", param.blendAlphaFunction);
      param.enableBlending = blendBlk->getBool("enableBlending", param.enableBlending);
    }
  }
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

bool drv3d_dx12::pipeline::RenderStateDeEncoder::decode(const DataBlock &blk, RenderStateSystem::StaticState &target) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target);
  }

  shaders::RenderState engineValue;
  if (!decodeEngine(blk, engineValue))
  {
    return false;
  }
  target = RenderStateSystem::StaticState::fromRenderState(engineValue);
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

  auto hasIndipendentBlend = source.enableIndependentBlend;
  auto blendCount = hasIndipendentBlend ? shaders::RenderState::NumIndependentBlendParameters : 1;
  char buf[32];
  for (uint32_t bbi = 0; bbi < blendCount; ++bbi)
  {
    sprintf_s(buf, "bp_%u", bbi);
    auto blendBlk = blendParamsBlk->addNewBlock(buf);
    if (!blendBlk)
    {
      return false;
    }
    auto &param = source.blendParams[bbi];

    blendBlk->setInt("blendFactorsSource", param.blendFactors.Source);
    blendBlk->setInt("blendFactorsDestination", param.blendFactors.Destination);

    blendBlk->setInt("blendAlphaFactorsSource", param.blendAlphaFactors.Source);
    blendBlk->setInt("blendAlphaFactorsDestination", param.blendAlphaFactors.Destination);

    blendBlk->setInt("blendFunction", param.blendFunction);
    blendBlk->setInt("blendAlphaFunction", param.blendAlphaFunction);
    blendBlk->setBool("enableBlending", param.enableBlending);
  }
  return true;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::decodeDX12(const DataBlock &blk, FramebufferLayout &target) const
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
      }

      lastParamIndex = blk.findParam(formatNameId, lastParamIndex);
    }
  }
  if (blk.paramExists("depthStencilFormat"))
  {
    target.hasDepth = 1;
    target.depthStencilFormat = FormatStore(blk.getInt("depthStencilFormat", 0));
  }
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
  }
  if (blk.paramExists("depthStencilFormat"))
  {
    target.hasDepth = 1;
    target.depthStencilFormat = FormatStore::fromCreateFlags(blk.getInt("depthStencilFormat", 0));
  }
  return true;
}

bool drv3d_dx12::pipeline::FramebufferLayoutDeEncoder::decode(const DataBlock &blk, FramebufferLayout &target) const
{
  if (0 == strcmp("dx12", blk.getStr("fmt", defaultFormat)))
  {
    return decodeDX12(blk, target);
  }
  return decodeEngine(blk, target);
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
  }
  if (source.hasDepth)
  {
    blk.setInt("depthStencilFormat", source.depthStencilFormat.wrapper.value);
  }
  return true;
}

bool drv3d_dx12::pipeline::DeviceCapsAndShaderModelDeEncoder::decode(const DataBlock &blk, DeviceCapsAndShaderModel &target) const
{
  target.shaderModel.major = blk.getInt("shaderModelMajor", target.shaderModel.major);
  target.shaderModel.major = blk.getInt("shaderModelMinor", target.shaderModel.minor);
#define DX12_D3D_CAP(name) target.caps.name = blk.getBool(#name, target.caps.name);
  DX12_D3D_CAP_SET
#undef DX12_D3D_CAP
  return true;
}

bool drv3d_dx12::pipeline::DeviceCapsAndShaderModelDeEncoder::encode(DataBlock &blk, const DeviceCapsAndShaderModel &source) const
{
  blk.setInt("shaderModelMajor", source.shaderModel.major);
  blk.setInt("shaderModelMinor", source.shaderModel.minor);
#define DX12_D3D_CAP(name) blk.setBool(#name, source.caps.name);
  DX12_D3D_CAP_SET
#undef DX12_D3D_CAP
  return true;
}

bool drv3d_dx12::pipeline::FeatureSupportResolver::decode(const DataBlock &blk, bool &target) const
{
  DeviceCapsAndShaderModel compare{};
  if (!this->DeviceCapsAndShaderModelDeEncoder::decode(blk, compare))
  {
    return false;
  }
  target = compare.isCompatibleTo(features);
  return true;
}

bool drv3d_dx12::pipeline::GraphicsPipelineVariantDeEncoder::decode(const DataBlock &blk, GraphicsPipelineVariantState &target) const
{
  if (blk.paramExists("featureSet") && featureSet)
  {
    auto fi = blk.getInt("featureSet", 0);
    // when index is out of range, we assume unsupported
    if (fi > featureSetCount)
    {
      return false;
    }
    if (!featureSet[fi])
    {
      return false;
    }
  }
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

bool drv3d_dx12::pipeline::GraphicsPipelineVariantDeEncoder::encode(DataBlock &blk, const GraphicsPipelineVariantState &source) const
{
  blk.setInt("renderState", source.staticRenderStateIndex);
  blk.setInt("outputFormat", source.framebufferLayoutIndex);
  blk.setInt("inputLayout", source.inputLayoutIndex);
  blk.setBool("wireFrame", 0 != source.isWireFrame);
  blk.setInt("primitiveTopology", static_cast<int>(source.topology));
  return true;
}

bool drv3d_dx12::pipeline::MeshPipelineVariantDeEncoder::decode(const DataBlock &blk, MeshPipelineVariantState &target) const
{
  if (blk.paramExists("featureSet") && featureSet)
  {
    auto fi = blk.getInt("featureSet", 0);
    // when index is out of range, we assume unsupported
    if (fi > featureSetCount)
    {
      return false;
    }
    if (!featureSet[fi])
    {
      return false;
    }
  }
  if (!blk.paramExists("renderState") || !blk.paramExists("outputFormat"))
  {
    return false;
  }
  target.staticRenderStateIndex = blk.getInt("renderState", 0);
  target.framebufferLayoutIndex = blk.getInt("outputFormat", 0);
  target.isWireFrame = blk.getBool("wireFrame", false);
  return true;
}

bool drv3d_dx12::pipeline::MeshPipelineVariantDeEncoder::encode(DataBlock &blk, const MeshPipelineVariantState &source) const
{
  blk.setInt("renderState", source.staticRenderStateIndex);
  blk.setInt("outputFormat", source.framebufferLayoutIndex);
  blk.setBool("wireFrame", 0 != source.isWireFrame);
  return true;
}

bool drv3d_dx12::pipeline::ComputePipelineDeEncoder::decode(const DataBlock &blk, ComputePipelineIdentifier &target) const
{
  if (blk.paramExists("featureSet") && featureSet)
  {
    auto fi = blk.getInt("featureSet", 0);
    // when index is out of range, we assume unsupported
    if (fi > featureSetCount)
    {
      return false;
    }
    if (!featureSet[fi])
    {
      return false;
    }
  }
  auto hash = blk.getStr("hash", nullptr);
  if (!hash)
  {
    return false;
  }
  target.hash = dxil::HashValue::fromString(hash);
  return true;
}

bool drv3d_dx12::pipeline::ComputePipelineDeEncoder::encode(DataBlock &blk, const ComputePipelineIdentifier &source) const
{
  ComputePipelineIdentifierHashSet set = source;
  blk.setStr("hash", set.hash);
  return true;
}
