// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "device.h"
#include "render_target_mask_util.h"

#include <EASTL/functional.h>
#include <ioSys/dag_memIo.h>


using namespace drv3d_dx12;

#define g_main blit_vertex_shader
#if _TARGET_XBOXONE
#include "shaders/blit.vs.x.h"
#elif _TARGET_SCARLETT
#include "shaders/blit.vs.xs.h"
#else
#include "shaders/blit.vs.h"
#endif
#undef g_main

#define g_main blit_pixel_shader
#if _TARGET_XBOXONE
#include "shaders/blit.ps.x.h"
#elif _TARGET_SCARLETT
#include "shaders/blit.ps.xs.h"
#else
#include "shaders/blit.ps.h"
#endif
#undef g_main

#define g_main clear_vertex_shader
#if _TARGET_XBOXONE
#include "shaders/clear.vs.x.h"
#elif _TARGET_SCARLETT
#include "shaders/clear.vs.xs.h"
#else
#include "shaders/clear.vs.h"
#endif
#undef g_main

#define g_main clear_pixel_shader
#if _TARGET_XBOXONE
#include "shaders/clear.ps.x.h"
#elif _TARGET_SCARLETT
#include "shaders/clear.ps.xs.h"
#else
#include "shaders/clear.ps.h"
#endif

#if _TARGET_SCARLETT
static bool has_acceleration_structure(const dxil::ShaderHeader &header)
{
  for (int i = 0; i < dxil::MAX_T_REGISTERS; ++i)
    if (static_cast<D3D_SHADER_INPUT_TYPE>(header.tRegisterTypes[i] & 0xF) == 12 /*D3D_SIT_RTACCELERATIONSTRUCTURE*/)
      return true;
  return false;
}
static bool has_acceleration_structure(const StageShaderModule &shader) { return has_acceleration_structure(shader.header); }
static bool has_acceleration_structure(const backend::VertexShaderModuleRefStore &ref)
{
  return has_acceleration_structure(ref.header.header);
}
static bool has_acceleration_structure(const backend::PixelShaderModuleRefStore &ref)
{
  return has_acceleration_structure(ref.header.header);
}
#endif

bool PipelineVariant::calculateColorWriteMask(const eastl::string &pipeline_name, const BasePipeline &base,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, uint32_t &color_write_mask) const
{
  // Rules for render target, shader output and color write mask iteraction:
  // - If a render target is not set, the slot is turned off
  // - If a shader does not output a slot at all, the slot is turned off
  // - For all left on slots the color write mask is applied

  // This turns the color write mask implied by the shader oupts into a color write mask
  // this turns either a slot completely on or off. So if the shader writes SV_Target0.xy
  // and SV_Target1.xyzw, the mask of the shader is 0x00000F3 and this derived mask will be
  // 0x000000FF as the target 0 and 1 are enabled.
  uint32_t basicShaderTargetMask =
    spread_color_chanel_mask_to_render_target_color_channel_mask(base.psModule.header.header.inOutSemanticMask);
  // The mask from the framebuffers active slots and thier formats is masked off by the written
  // slots by the shader implicitly.
  uint32_t fbColorWriteMask = fb_layout.getColorWriteMask();
  color_write_mask = fbColorWriteMask & basicShaderTargetMask;
  uint32_t missingShaderOutputMask =
    static_state.calculateMissingShaderOutputMask(color_write_mask, base.psModule.header.header.inOutSemanticMask);
  if (missingShaderOutputMask)
  {
    D3D_ERROR("DX12: Pipeline <%s> does not write all enabled color target components. "
              "Shader output mask %08X, frame buffer color mask implied from shader %08X, static "
              "state color write mask %08X, color write mask implied from frame buffer %08X, "
              "computed color write mask %08X, missing outputs mask %08X",
      pipeline_name, base.psModule.header.header.inOutSemanticMask, basicShaderTargetMask, static_state.colorWriteMask,
      fbColorWriteMask, static_state.adjustColorTargetMask(color_write_mask), missingShaderOutputMask);
    return false;
  }
  return true;
}

bool PipelineVariant::generateOutputMergerDescriptions(const eastl::string &pipeline_name, const BasePipeline &base,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, GraphicsPipelineCreateInfoData &target) const
{
  uint32_t fbFinalColorWriteMask = 0;
  auto isOK = calculateColorWriteMask(pipeline_name, base, static_state, fb_layout, fbFinalColorWriteMask);

  DXGI_SAMPLE_DESC &rtSampleDesc = target.append<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>();
  rtSampleDesc.Count = 1;
  rtSampleDesc.Quality = 0;

  if (0 != fb_layout.hasDepth)
  {
    target.append<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(fb_layout.depthStencilFormat.asDxGiFormat());
    rtSampleDesc.Count = 1 << fb_layout.depthMsaaLevel;
  }

  D3D12_RT_FORMAT_ARRAY &rtfma = target.append<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>();
  uint32_t mask = fb_layout.colorTargetMask;
  if (mask)
  {
    for (uint32_t i = 0; i < 8; ++i, mask >>= 1)
    {
      if (mask & 1)
      {
        rtfma.RTFormats[i] = fb_layout.colorFormats[i].asDxGiFormat();
        rtfma.NumRenderTargets = i + 1;
        rtSampleDesc.Count = 1 << fb_layout.getColorMsaaLevel(i);
      }
      else
      {
        rtfma.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
      }
    }
  }
  else
  {
    rtfma.NumRenderTargets = 0;
  }

  const auto blendDesc = static_state.getBlendDesc(fbFinalColorWriteMask);
  isOK = isOK && validate_blend_desc(blendDesc, fb_layout, fbFinalColorWriteMask);
  target.append<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(blendDesc);

  if (dynamicMask.hasDepthBoundsTest)
  {
    target.append<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>(static_state.getDepthStencilDesc1());
  }
  else
  {
    target.append<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL>(static_state.getDepthStencilDesc());
  }
  return isOK;
}

bool PipelineVariant::generateInputLayoutDescription(const InputLayout &input_layout, GraphicsPipelineCreateInfoData &target,
  uint32_t &input_count) const
{
  input_layout.generateInputElementDescriptions([&input_count, &target](auto desc) //
    { target.inputElementStore[input_count++] = desc; });

  if (input_count > 0)
  {
    target.emplace<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>(target.inputElementStore, input_count);
  }

  return true;
}

bool PipelineVariant::generateRasterDescription(const RenderStateSystem::StaticState &static_state, bool is_wire_frame,
  GraphicsPipelineCreateInfoData &target) const
{
  // TODO: remove polygonLine on non debug builds
  target.append<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(
    static_state.getRasterizerDesc(is_wire_frame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID));

  if (static_state.needsViewInstancing())
  {
    G_ASSERT(d3d::get_driver_desc().caps.hasBasicViewInstancing);
    target.append<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING>(static_state.getViewInstancingDesc());
  }
  return true;
}

bool PipelineVariant::create(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base,
  PipelineCache &pipe_cache, const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name,
  backend::PipelineNameGenerator &name_generator)
{
  bool succeeded = true;
  auto name = name_generator.generateGraphicsPipelineName(base.vsModule, base.psModule, static_state);

#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  profilerMsg.sprintf("PipelineVariant (%s)::create: took %%dus", name.c_str());
  AutoFuncProf funcProfiler(profilerMsg.c_str());
#endif
  TIME_PROFILE_DEV(PipelineVariant_create);
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV(name.c_str());

  fb_layout.checkMsaaLevelsEqual();
#if _TARGET_PC_WIN
  if (!base.signature.signature)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    // turns off reporting of the profile, we don't want to know how long it took to _not_
    // load a pipeline
    funcProfiler.fmt = nullptr;
#endif
    logdbg("DX12: Rootsignature was null, skipping creating graphics pipeline");
    return true;
  }

  GraphicsPipelineCreateInfoData gpci;
  uint32_t inputCount = 0;

  bool inputOK = generateInputLayoutDescription(input_layout, gpci, inputCount);

  auto inputMatchMask = input_layout.vertexAttributeLocationMask ^ base.vsModule.header.header.inOutSemanticMask;
  if (inputMatchMask & base.vsModule.header.header.inOutSemanticMask)
  {
    logdbg("DX12: Detected input layout mismatch (provided=%08X, expected=%08X, result=%08X), ignoring pipeline variant",
      input_layout.vertexAttributeLocationMask, base.vsModule.header.header.inOutSemanticMask, inputMatchMask);
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    // turns off reporting of the profile, we don't want to know how long it took to _not_
    // load a pipeline
    funcProfiler.fmt = nullptr;
#endif
    return false;
  }

  bool outputOK = generateOutputMergerDescriptions(name, base, static_state, fb_layout, gpci);
  bool rasterOK = generateRasterDescription(static_state, is_wire_frame, gpci);

  succeeded = outputOK && inputOK && rasterOK;

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(base.signature.signature.Get());

  auto vsBytecode = shader_bytecodes.getVsBytecode(base.vsModule);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_VS, CD3DX12_SHADER_BYTECODE>(vsBytecode.data(), vsBytecode.size());
  auto psBytecode = shader_bytecodes.getBytecode(base.psModule);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(psBytecode.data(), psBytecode.size());
  if (get_gs(base.vsModule))
  {
    auto gsBytecode = shader_bytecodes.getGsBytecode(base.vsModule);
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_GS, CD3DX12_SHADER_BYTECODE>(gsBytecode.data(), gsBytecode.size());
  }
  if (get_hs(base.vsModule))
  {
    auto hsBytecode = shader_bytecodes.getHsBytecode(base.vsModule);
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_HS, CD3DX12_SHADER_BYTECODE>(hsBytecode.data(), hsBytecode.size());
  }
  if (get_ds(base.vsModule))
  {
    auto dsBytecode = shader_bytecodes.getDsBytecode(base.vsModule);
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_DS, CD3DX12_SHADER_BYTECODE>(dsBytecode.data(), dsBytecode.size());
  }

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>(top);

  D3D12_CACHED_PIPELINE_STATE &cacheTarget = gpci.append<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>();

  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
  pipeline = pipe_cache.loadGraphicsPipelineVariant(base.cacheId, top, input_layout, is_wire_frame, static_state, fb_layout, gpciDesc,
    cacheTarget);

#else
  uint32_t fbFinalColorWriteMask = 0;
  succeeded = calculateColorWriteMask(name, base, static_state, fb_layout, fbFinalColorWriteMask);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {0};
  desc.SampleMask = ~0u;
  if (fb_layout.hasDepth)
  {
    desc.DSVFormat = fb_layout.depthStencilFormat.asDxGiFormat();
    desc.SampleDesc.Count = 1 << fb_layout.depthMsaaLevel;
  }
  else
  {
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
  }

  uint32_t mask = fb_layout.colorTargetMask;
  if (mask)
  {
    for (uint32_t i = 0; i < 8; ++i, mask >>= 1)
    {
      if (mask & 1)
      {
        desc.RTVFormats[i] = fb_layout.colorFormats[i].asDxGiFormat();
        desc.NumRenderTargets = i + 1;
        desc.SampleDesc.Count = 1 << fb_layout.getColorMsaaLevel(i);
      }
      else
        desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
  }
  else
  {
    desc.NumRenderTargets = 0;
  }

  desc.BlendState = static_state.getBlendDesc(fbFinalColorWriteMask);

  uint32_t inputCount = 0;
  D3D12_INPUT_ELEMENT_DESC elemDesc[MAX_VERTEX_ATTRIBUTES];
  input_layout.generateInputElementDescriptions([&inputCount, &elemDesc](auto desc) //
    { elemDesc[inputCount++] = desc; });

  desc.InputLayout.NumElements = inputCount;
  desc.InputLayout.pInputElementDescs = elemDesc;

  desc.RasterizerState = static_state.getRasterizerDesc(is_wire_frame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID);
  desc.DepthStencilState = static_state.getDepthStencilDesc();

  desc.pRootSignature = base.signature.signature.Get();
  desc.PrimitiveTopologyType = top;

  auto vsBytecode = shader_bytecodes.getVsBytecode(base.vsModule);
  desc.VS.pShaderBytecode = vsBytecode.data();
  desc.VS.BytecodeLength = vsBytecode.size();

  auto psBytecode = shader_bytecodes.getBytecode(base.psModule);
  desc.PS.pShaderBytecode = psBytecode.data();
  desc.PS.BytecodeLength = psBytecode.size();

  if (get_gs(base.vsModule))
  {
    auto gsBytecode = shader_bytecodes.getGsBytecode(base.vsModule);
    desc.GS.pShaderBytecode = gsBytecode.data();
    desc.GS.BytecodeLength = gsBytecode.size();
  }
  if (get_hs(base.vsModule))
  {
    auto hsBytecode = shader_bytecodes.getHsBytecode(base.vsModule);
    desc.HS.pShaderBytecode = hsBytecode.data();
    desc.HS.BytecodeLength = hsBytecode.size();
  }
  if (get_ds(base.vsModule))
  {
    auto dsBytecode = shader_bytecodes.getDsBytecode(base.vsModule);
    desc.DS.pShaderBytecode = dsBytecode.data();
    desc.DS.BytecodeLength = dsBytecode.size();
  }

#endif
  if (!pipeline)
  {
#if _TARGET_PC_WIN
    auto result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    if (FAILED(result) && cacheTarget.CachedBlobSizeInBytes > 0)
    {
      logdbg("DX12: CreatePipelineState failed with attached cache blob, retrying without");
      cacheTarget.pCachedBlob = nullptr;
      cacheTarget.CachedBlobSizeInBytes = 0;
      result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    }
#else
    G_UNUSED(pipe_cache);
    auto result = DX12_CHECK_RESULT(xbox_create_graphics_pipeline(device, desc, !static_state.enableDepthBounds, pipeline));
#endif
    // Special handling as on PC we may have to retry if cache blob reload failed
    if (FAILED(DX12_CHECK_RESULTF(result, "CreatePipelineState")))
    {
      if (is_recoverable_error(result))
      {
        String fb_layout_info;
        fb_layout_info.printf(64, "hasDepth %u", fb_layout.hasDepth);
        uint8_t sampleCount = 1;
        if (0 != fb_layout.hasDepth)
        {
          fb_layout_info.aprintf(128, " depthStencilFormat %s", fb_layout.depthStencilFormat.getNameString());
          sampleCount = 1 << fb_layout.depthMsaaLevel;
        }
        fb_layout_info.aprintf(64, " colorTargetMask %u", fb_layout.colorTargetMask);
        if (fb_layout.colorTargetMask)
        {
          uint32_t mask = fb_layout.colorTargetMask;
          fb_layout_info.aprintf(64, " colorFormats were");
          for (uint32_t i = 0; i < 8 && mask; ++i, mask >>= 1)
          {
            if (mask & 1)
            {
              fb_layout_info.aprintf(128, " %s", fb_layout.colorFormats[i].getNameString());
              sampleCount = 1 << fb_layout.getColorMsaaLevel(i);
            }
            else
              fb_layout_info.aprintf(128, " DXGI_FORMAT_UNKNOWN");
          }
        }

        fb_layout_info.aprintf(64, " sampleCount %d", sampleCount);

        String input_layout_info;
        input_layout_info.printf(64, "inputCount %u, mask %08X", inputCount, input_layout.vertexAttributeLocationMask);
        if (inputCount)
          input_layout_info.aprintf(64, " and attributes");
        input_layout.generateInputElementDescriptions([&input_layout_info](auto desc) {
          input_layout_info.aprintf(128, " (SemanticName %s", desc.SemanticName);
          input_layout_info.aprintf(128, " SemanticIndex %u", desc.SemanticIndex);
          input_layout_info.aprintf(128, " Format %u", desc.Format);
          input_layout_info.aprintf(128, " InputSlot %u", desc.InputSlot);
          input_layout_info.aprintf(128, " AlignedByteOffset %u", desc.AlignedByteOffset);
          input_layout_info.aprintf(128, " InputSlotClass %u)", desc.InputSlotClass);
        });
        // generate inputs that are consumed by the vertex shader, this will show if the input layout was under specified.
        String consumedInputLayout;
        auto mask = base.vsModule.header.header.inOutSemanticMask;
        if (mask)
        {
          consumedInputLayout.printf(128, "mask %08X", base.vsModule.header.header.inOutSemanticMask);
          for (uint32_t i = 0; i < MAX_SEMANTIC_INDEX && mask; ++i, mask >>= 1)
          {
            if (mask & 1)
            {
              auto semanticInfo = dxil::getSemanticInfoFromIndex(i);
              G_ASSERT(semanticInfo);
              if (!semanticInfo)
                break;

              consumedInputLayout.aprintf(128, " (SemanticName %s SemanticIndex %u)", semanticInfo->name, semanticInfo->index);
            }
          }
        }
        else
        {
          consumedInputLayout.printf(64, "nothing");
        }
        switch (on_error)
        {
          // Returning succeeded for failed shader factoring because of bad input parameters? (This comment is only for review, to be
          // removed)
          case RecoverablePipelineCompileBehavior::FATAL_ERROR: return true; break;
          case RecoverablePipelineCompileBehavior::ASSERT_FAIL:
          case RecoverablePipelineCompileBehavior::REPORT_ERROR:
            D3D_ERROR("CreatePipelineState failed for <%s>, state was %s, fb_layout was { "
                      "%s }, input_layout was { %s }, vertex inputs were { %s }, il/vi mask xor %08X",
              name, static_state.toString(), fb_layout_info, input_layout_info, consumedInputLayout,
              input_layout.vertexAttributeLocationMask ^ base.vsModule.header.header.inOutSemanticMask);
            break;
        }
      }
      else
      {
        return true;
      }
    }
#if _TARGET_PC_WIN
    // only write back cache if none exists yet (or is invalid)
    else if (succeeded && (0 == cacheTarget.CachedBlobSizeInBytes))
    {
      pipe_cache.addGraphicsPipelineVariant(base.cacheId, top, input_layout, is_wire_frame, static_state, fb_layout, pipeline.Get());
    }
#endif
  }

#if DX12_DOES_SET_DEBUG_NAMES
  if (pipeline && give_name && !name.empty())
  {
    shorten_name_for_pix(name);
    dag::Vector<wchar_t> nameBuf;
    nameBuf.resize(name.length() + 1);
    DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(name.data(), nameBuf.data(), nameBuf.size()));
  }
#else
  G_UNUSED(give_name);
#endif
  return succeeded;
}

bool PipelineVariant::validate_blend_desc(const D3D12_BLEND_DESC &blend_desc, const FramebufferLayout &fb_layout,
  uint32_t color_write_mask)
{
  bool isOk = true;
  G_UNUSED(blend_desc);
  G_UNUSED(fb_layout);
  G_UNUSED(color_write_mask);
#if DAGOR_DBGLEVEL > 0
  auto finalColorTargetMask = color_write_mask & fb_layout.colorTargetMask;
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (i > 0 && !blend_desc.IndependentBlendEnable)
      break;
    if ((finalColorTargetMask & (1 << i)) == 0 || (blend_desc.RenderTarget[i].RenderTargetWriteMask & (1 << i)) == 0)
      continue;
    if (blend_desc.RenderTarget[i].BlendEnable)
    {
      const auto supports = get_device().getFormatFeatures(fb_layout.colorFormats[i]);
      if ((supports.Support1 & D3D12_FORMAT_SUPPORT1_BLENDABLE) == 0)
      {
        isOk = false;
        logerr("DX12: Blend enabled for render target %u with format %s that does not support blending", i,
          fb_layout.colorFormats[i].getNameString());
      }
    }
  }
#endif
  return isOk;
}

#if !_TARGET_XBOXONE
bool PipelineVariant::createMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base,
  PipelineCache &pipe_cache, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, RecoverablePipelineCompileBehavior on_error, bool give_name,
  backend::PipelineNameGenerator &name_generator)
{
  bool succeeded = true;
  auto name = name_generator.generateGraphicsPipelineName(base.vsModule, base.psModule, static_state);

#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  profilerMsg.sprintf("PipelineVariant (%s)::createMesh: took %%dus", name.c_str());
  AutoFuncProf funcProfiler(profilerMsg.c_str());
#endif
  TIME_PROFILE_DEV(PipelineVariant_create);
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV(name.c_str());

  if (!base.signature.signature)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    // turns off reporting of the profile, we don't want to know how long it took to _not_
    // load a pipeline
    funcProfiler.fmt = nullptr;
#endif
    logdbg("DX12: Rootsignature was null, skipping creating graphics pipeline");
    return true;
  }

  GraphicsPipelineCreateInfoData gpci;

  bool outputOK = generateOutputMergerDescriptions(name, base, static_state, fb_layout, gpci);
  bool rasterOK = generateRasterDescription(static_state, is_wire_frame, gpci);

  succeeded = outputOK && rasterOK;

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(base.signature.signature.Get());

  // vsModule stores mesh shader
  auto msBytecode = shader_bytecodes.getMsBytecode(base.vsModule);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_MS, CD3DX12_SHADER_BYTECODE>(msBytecode.data(), msBytecode.size());
  auto psBytecode = shader_bytecodes.getBytecode(base.psModule);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(psBytecode.data(), psBytecode.size());
  // vsModule.geometryShader stores amplification shader
  if (get_as(base.vsModule))
  {
    auto asBytecode = shader_bytecodes.getAsBytecode(base.vsModule);
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_AS, CD3DX12_SHADER_BYTECODE>(asBytecode.data(), asBytecode.size());
  }

#if _TARGET_PC_WIN
  D3D12_CACHED_PIPELINE_STATE &cacheTarget = gpci.append<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>();

  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
  pipeline = pipe_cache.loadGraphicsMeshPipelineVariant(base.cacheId, is_wire_frame, static_state, fb_layout, gpciDesc, cacheTarget);
#else
  G_UNUSED(pipe_cache);
  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
#endif

  if (!pipeline)
  {
    auto result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
#if _TARGET_PC_WIN
    if (FAILED(result) && cacheTarget.CachedBlobSizeInBytes > 0)
    {
      logdbg("DX12: CreatePipelineState failed with attached cache blob, retrying without");
      cacheTarget.pCachedBlob = nullptr;
      cacheTarget.CachedBlobSizeInBytes = 0;
      result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    }
#endif
    // Special handling as on PC we may have to retry if cache blob reload failed
    if (FAILED(DX12_CHECK_RESULTF(result, "CreatePipelineState")))
    {
      if (is_recoverable_error(result))
      {
        String fb_layout_info;
        fb_layout_info.printf(64, "hasDepth %u", fb_layout.hasDepth);
        uint8_t sampleCount = 1;
        if (0 != fb_layout.hasDepth)
        {
          fb_layout_info.aprintf(128, " depthStencilFormat %s", fb_layout.depthStencilFormat.getNameString());
          sampleCount = 1 << fb_layout.depthMsaaLevel;
        }
        fb_layout_info.aprintf(64, " colorTargetMask %u", fb_layout.colorTargetMask);
        if (fb_layout.colorTargetMask)
        {
          uint32_t mask = fb_layout.colorTargetMask;
          fb_layout_info.aprintf(64, " colorFormats were");
          for (uint32_t i = 0; i < 8 && mask; ++i, mask >>= 1)
          {
            if (mask & 1)
            {
              fb_layout_info.aprintf(128, " %s", fb_layout.colorFormats[i].getNameString());
              sampleCount = 1 << fb_layout.getColorMsaaLevel(i);
            }
            else
              fb_layout_info.aprintf(128, " DXGI_FORMAT_UNKNOWN");
          }
        }

        fb_layout_info.aprintf(64, " sampleCount %d", sampleCount);

        switch (on_error)
        {
          // Returning succeeded for failed shader factoring because of bad input parameters? (This comment is only for review, to be
          // removed)
          case RecoverablePipelineCompileBehavior::FATAL_ERROR: return true; break;
          case RecoverablePipelineCompileBehavior::ASSERT_FAIL:
          case RecoverablePipelineCompileBehavior::REPORT_ERROR:
            D3D_ERROR("CreatePipelineState failed for <%s>, state was %s, fb_layout was { %s }", name, static_state.toString(),
              fb_layout_info);
            break;
        }
      }
      else
      {
        return true;
      }
    }
#if _TARGET_PC_WIN
    // only write back cache if none exists yet (or is invalid)
    else if (succeeded && (0 == cacheTarget.CachedBlobSizeInBytes))
    {
      pipe_cache.addGraphicsMeshPipelineVariant(base.cacheId, is_wire_frame, static_state, fb_layout, pipeline.Get());
    }
#endif
  }

#if DX12_DOES_SET_DEBUG_NAMES
  if (pipeline && give_name && !name.empty())
  {
    shorten_name_for_pix(name);
    dag::Vector<wchar_t> nameBuf;
    nameBuf.resize(name.length() + 1);
    DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(name.data(), nameBuf.data(), nameBuf.size()));
  }
#else
  G_UNUSED(give_name);
#endif
  return succeeded;
}
#endif


bool PipelineVariant::load(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base,
  PipelineCache &pipe_cache, const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name,
  backend::PipelineNameGenerator &name_generator)
{
  if (pipeline)
    return true;
  dynamicMask = static_state.getDynamicStateMask();
#if !_TARGET_XBOXONE
  G_ASSERT(!base.isMesh());
#endif
  return create(device, shader_bytecodes, base, pipe_cache, input_layout, is_wire_frame, static_state, fb_layout, top, on_error,
    give_name, name_generator);
}

bool PipelineVariant::loadMesh(ID3D12Device2 *device, backend::ShaderModuleManager &shader_bytecodes, BasePipeline &base,
  PipelineCache &pipe_cache, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, RecoverablePipelineCompileBehavior on_error, bool give_name,
  backend::PipelineNameGenerator &name_generator)
{
  if (pipeline)
    return true;
  dynamicMask = static_state.getDynamicStateMask();
#if !_TARGET_XBOXONE
  G_ASSERT(base.isMesh());
  return createMesh(device, shader_bytecodes, base, pipe_cache, is_wire_frame, static_state, fb_layout, on_error, give_name,
    name_generator);
#else
  G_UNUSED(device);
  G_UNUSED(shader_bytecodes);
  G_UNUSED(base);
  G_UNUSED(pipe_cache);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(on_error);
  G_UNUSED(give_name);
  G_UNUSED(name_generator);
  return false;
#endif
}

void PipelineManager::init(const SetupParameters &params)
{
  ShaderDeviceRequirementChecker::init(params.device);
  backend::InputLayoutManager::init();

  D3D12SerializeRootSignature = params.serializeRootSignature;
  signatureVersion = params.rootSignatureVersion;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  rootSignaturesUsesCBVDescriptorRanges = params.rootSignaturesUsesCBVDescriptorRanges;
#endif

  createBlitSignature(params.device);
  createClearSignature(params.device);

  params.pipelineCache->enumerateInputLayouts([this](auto &layout) { addInternalLayout(layout); });

  params.pipelineCache->enumerateGraphicsSignatures([this, device = params.device](const GraphicsPipelineSignature::Definition &def,
                                                      const dag::Vector<uint8_t> &blob) //
    {
      auto newSig = eastl::make_unique<GraphicsPipelineSignature>();
      newSig->def = def;
      newSig->deriveComboMasks();
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      graphicsSignatures.push_back(eastl::move(newSig));
    });

#if !_TARGET_XBOXONE
  params.pipelineCache->enumerateGraphicsMeshSignatures(
    [this, device = params.device](const GraphicsPipelineSignature::Definition &def, const dag::Vector<uint8_t> &blob) {
      auto newSig = eastl::make_unique<GraphicsPipelineSignature>();
      newSig->def = def;
      newSig->deriveComboMasks();
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      graphicsMeshSignatures.push_back(eastl::move(newSig));
    });
#endif

  params.pipelineCache->enumrateComputeSignatures([this, device = params.device](const ComputePipelineSignature::Definition &def,
                                                    const dag::Vector<uint8_t> &blob) //
    {
      auto newSig = eastl::make_unique<ComputePipelineSignature>();
      newSig->def = def;
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      computeSignatures.push_back(eastl::move(newSig));
    });
}

void PipelineManager::setCompilePipelineSetQueueLength()
{
  uint32_t queueLength = 0;
  if (pipelineSetCompilation)
  {
    const auto graphicsQueue =
      pipelineSetCompilation->graphicsPipelines.size() - pipelineSetCompilation->graphicsPipelineSetCompilationIdx;
    const auto meshQueue = pipelineSetCompilation->meshPipelines.size() - pipelineSetCompilation->meshPipelineSetCompilationIdx;
    const auto computeQueue =
      pipelineSetCompilation->computePipelines.size() - pipelineSetCompilation->computePipelineSetCompilationIdx;
    const auto setQueue = graphicsQueue + meshQueue + computeQueue;
    queueLength += setQueue;
  }
  if (pipelineSetCompilation2)
  {
    const auto groupDone = pipelineSetCompilation2->graphicsPipelineSetCompilationIdx +
                           pipelineSetCompilation2->graphicsPipelinesWithNullOverridesSetCompilationIdx +
                           pipelineSetCompilation2->computePipelineSetCompilationIdx;
    const auto totalGroup = pipelineSetCompilation2->graphicsPipelines.size() +
                            pipelineSetCompilation2->graphicsPipelinesWithNullOverrides.size() +
                            pipelineSetCompilation2->computePipelines.size();
    const auto groupQueue = totalGroup - groupDone;
    const auto leftGroups = (max_scripted_shaders_bin_groups - 1 - pipelineSetCompilation2->groupIndex) * totalGroup;
    const auto set2Queue = groupQueue + leftGroups;
    queueLength += set2Queue;
  }
  compilePipelineSetQueueLength.store(queueLength, std::memory_order_release);
}

void PipelineManager::createBlitSignature(ID3D12Device *device)
{
  // blit signature:
  // [0][offset.x, offset.y, scale.x, scale.y]
  // [1][range to SRV]
  // [x][static sampler]
  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.MipLODBias = 0;
  sampler.MaxAnisotropy = 1;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD = -1000;
  sampler.MaxLOD = 1000;
  sampler.ShaderRegister = 0;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_DESCRIPTOR_RANGE range = {};
  D3D12_ROOT_PARAMETER params[2] = {};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  params[0].Constants.ShaderRegister = 0;
  params[0].Constants.RegisterSpace = 0;
  params[0].Constants.Num32BitValues = 4; // one float4
  params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

  params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  params[1].DescriptorTable.NumDescriptorRanges = 1;
  params[1].DescriptorTable.pDescriptorRanges = &range;
  params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  range.NumDescriptors = 1;
  range.BaseShaderRegister = 0;
  range.RegisterSpace = 0;
  range.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_SIGNATURE_DESC desc = {};
  desc.NumParameters = 2;
  desc.pParameters = params;
  desc.NumStaticSamplers = 1;
  desc.pStaticSamplers = &sampler;
  desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
               D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

  ComPtr<ID3DBlob> rootSignBlob;
  ComPtr<ID3DBlob> errorBlob;
  if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
  {
    DAG_FATAL("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
  }
  DX12_CHECK_RESULT(
    device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&blitSignature)));
}

void PipelineManager::createClearSignature(ID3D12Device *device)
{
  // clear signature:
  // [0][clear_value]
  D3D12_ROOT_PARAMETER params[1] = {};
  params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  params[0].Constants.ShaderRegister = 0;
  params[0].Constants.RegisterSpace = 0;
  params[0].Constants.Num32BitValues = 4; // one float4
  params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC desc = {};
  desc.NumParameters = 1;
  desc.pParameters = params;
  desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
               D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

  ComPtr<ID3DBlob> rootSignBlob;
  ComPtr<ID3DBlob> errorBlob;
  if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    DAG_FATAL("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));

  DX12_CHECK_RESULT(
    device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&clearSignature)));
}

template <typename VertexShaderT, typename PixelShaderT>
static auto createSimplePipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, ComPtr<ID3D12PipelineState> &pipeline,
  ID3D12RootSignature *signature, const VertexShaderT &vs, const PixelShaderT &ps)
{
  G_ASSERT(signature);
#if _TARGET_PC_WIN
  GraphicsPipelineCreateInfoData gpci;

  D3D12_RT_FORMAT_ARRAY &rtfma = gpci.append<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>();
  rtfma.RTFormats[0] = out_fmt;
  rtfma.NumRenderTargets = 1;

  CD3DX12_BLEND_DESC &blendDesc = gpci.append<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>();
  blendDesc.AlphaToCoverageEnable = FALSE;
  blendDesc.IndependentBlendEnable = FALSE;

  auto &target = blendDesc.RenderTarget[0];
  target.BlendEnable = FALSE;
  target.LogicOpEnable = FALSE;
  target.SrcBlend = D3D12_BLEND_ONE;
  target.DestBlend = D3D12_BLEND_ZERO;
  target.BlendOp = D3D12_BLEND_OP_ADD;
  target.SrcBlendAlpha = D3D12_BLEND_ONE;
  target.DestBlendAlpha = D3D12_BLEND_ZERO;
  target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  target.LogicOp = D3D12_LOGIC_OP_NOOP;
  target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, FALSE, 0, 0.f, 1.f, TRUE, FALSE,
    FALSE, 0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL>(FALSE, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, FALSE, 0,
    0, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS);

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(signature);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_VS, CD3DX12_SHADER_BYTECODE>(vs, sizeof(vs));
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(ps, sizeof(ps));
  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
  return DX12_CHECK_RESULT(device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline)));

#else

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {0};
  desc.SampleMask = 0x1;
  desc.pRootSignature = signature;
  desc.VS.pShaderBytecode = vs;
  desc.VS.BytecodeLength = sizeof(vs);
  desc.PS.pShaderBytecode = ps;
  desc.PS.BytecodeLength = sizeof(ps);
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.NumRenderTargets = 1;
  desc.RTVFormats[0] = out_fmt;
  desc.BlendState.AlphaToCoverageEnable = FALSE;
  desc.BlendState.IndependentBlendEnable = FALSE;
  desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
  desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
  desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
  desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
  desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
  desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
  desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
  desc.RasterizerState.FrontCounterClockwise = FALSE;
  desc.RasterizerState.DepthBias = 0;
  desc.RasterizerState.DepthBiasClamp = 0.f;
  desc.RasterizerState.SlopeScaledDepthBias = 1.f;
  desc.RasterizerState.DepthClipEnable = TRUE;
  desc.RasterizerState.MultisampleEnable = FALSE;
  desc.RasterizerState.AntialiasedLineEnable = FALSE;
  desc.RasterizerState.ForcedSampleCount = 0;
  desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  desc.DepthStencilState.DepthEnable = FALSE;
  desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  desc.DepthStencilState.StencilEnable = FALSE;
  desc.DepthStencilState.StencilReadMask = 0;
  desc.DepthStencilState.StencilWriteMask = 0;
  desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
  desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

  return DX12_CHECK_RESULT(xbox_create_graphics_pipeline(device, desc, true, pipeline));
#endif
}

ID3D12PipelineState *PipelineManager::createBlitPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name)
{
#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  profilerMsg.sprintf("PipelineManager::createBlitPipeline(out fmt: %s): took %%dus", dxgi_format_name(out_fmt));
  AutoFuncProf funcProfiler(profilerMsg.c_str());
#endif

#if _TARGET_PC_WIN
  if (!blitSignature)
    return nullptr;
#endif

  FormatPipeline newBlitPipe{nullptr, out_fmt};
  auto result =
    createSimplePipeline(device, out_fmt, newBlitPipe.pipeline, blitSignature.Get(), blit_vertex_shader, blit_pixel_shader);

  if (FAILED(result))
  {
    DAG_FATAL("Failed to create blit pipeline");
    return nullptr;
  }

  if (give_name)
    DX12_SET_DEBUG_OBJ_NAME(newBlitPipe.pipeline, L"BlitPipeline");

  return blitPipelines.emplace_back(eastl::move(newBlitPipe)).pipeline.Get();
}

ID3D12PipelineState *PipelineManager::createClearPipeline(ID3D12Device2 *device, DXGI_FORMAT out_fmt, bool give_name)
{
#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  profilerMsg.sprintf("PipelineManager::createClearPipeline(out fmt: %s): took %%dus", dxgi_format_name(out_fmt));
  AutoFuncProf funcProfiler(profilerMsg.c_str());
#endif

#if _TARGET_PC_WIN
  if (!clearSignature)
    return nullptr;
#endif

  FormatPipeline newClearPipe{nullptr, out_fmt};
  auto result =
    createSimplePipeline(device, out_fmt, newClearPipe.pipeline, clearSignature.Get(), clear_vertex_shader, clear_pixel_shader);

  if (FAILED(result))
  {
    DAG_FATAL("Failed to create clear pipeline");
    return nullptr;
  }

  if (give_name)
    DX12_SET_DEBUG_OBJ_NAME(newClearPipe.pipeline, L"ClearPipeline");

  return clearPipelines.emplace_back(eastl::move(newClearPipe)).pipeline.Get();
}

namespace
{
template <size_t StageCount, typename D>
struct BasicGraphicsRootSignatureGenerator
{
  static constexpr uint32_t per_stage_resource_limits =
    dxil::MAX_T_REGISTERS + dxil::MAX_S_REGISTERS + dxil::MAX_U_REGISTERS + dxil::MAX_B_REGISTERS;
  // cbuffer each one param, then srv, uav and sampler each one
  static constexpr uint32_t per_stage_resource_group_limits = dxil::MAX_B_REGISTERS + 1 + 1 + 1;
  static constexpr uint32_t bindles_stage_descriptors = 2;
  GraphicsPipelineSignature *signature = nullptr;
  D3D12_DESCRIPTOR_RANGE ranges[per_stage_resource_limits * StageCount + bindles_stage_descriptors] = {};
  D3D12_ROOT_PARAMETER params[per_stage_resource_group_limits * StageCount + bindles_stage_descriptors] = {};
  D3D12_ROOT_SIGNATURE_DESC desc = {};
  D3D12_DESCRIPTOR_RANGE *rangePosition = &ranges[0];
  D3D12_ROOT_PARAMETER *unboundedSamplersRootParam = nullptr;
  D3D12_ROOT_PARAMETER *bindlessSRVRootParam = nullptr;
  uint32_t rangeSize = 0;
  uint32_t signatureCost = 0;
  D3D12_SHADER_VISIBILITY currentVisibility = D3D12_SHADER_VISIBILITY_ALL;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  bool useConstantBufferRootDescriptors = false;
  bool shouldUseConstantBufferRootDescriptors() const { return useConstantBufferRootDescriptors; }
#else
  constexpr bool shouldUseConstantBufferRootDescriptors() const { return true; }
#endif
  void begin()
  {
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.pParameters = params;
  }
  void hasAccelerationStructure()
  {
#if _TARGET_SCARLETT
    desc.Flags |= ROOT_SIGNATURE_FLAG_RAYTRACING;
#endif
  }
  void noPixelShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; }
  void setVisibilityPixelShader() { currentVisibility = D3D12_SHADER_VISIBILITY_PIXEL; }
  void addRootParameterConstantExplicit(uint32_t space, uint32_t index, uint32_t dwords, D3D12_SHADER_VISIBILITY vis)
  {
    G_ASSERT(desc.NumParameters < countof(params));

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    target.ShaderVisibility = vis;
    target.Constants.ShaderRegister = index;
    target.Constants.RegisterSpace = space;
    target.Constants.Num32BitValues = dwords;

    signatureCost += dwords;
  }
  void addRootParameterConstant(uint32_t space, uint32_t index, uint32_t dwords)
  {
    addRootParameterConstantExplicit(space, index, dwords, currentVisibility);
  }
  void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
  {
    static_cast<D *>(this)->getStageInfo().rootConstantsParamIndex = desc.NumParameters;
    addRootParameterConstant(space, index, dwords);
  }
  void specialConstants(uint32_t space, uint32_t index)
  {
    addRootParameterConstantExplicit(space, index, 1, D3D12_SHADER_VISIBILITY_ALL);
  }
  void beginConstantBuffers()
  {
    rangeSize = 0;
    static_cast<D *>(this)->getStageInfo().setConstBufferDescriptorIndex(desc.NumParameters, shouldUseConstantBufferRootDescriptors());
  }
  void endConstantBuffers()
  {
    if (!shouldUseConstantBufferRootDescriptors())
    {
      G_ASSERT(desc.NumParameters < countof(params));
      G_ASSERT(rangeSize > 0);

      auto &target = params[desc.NumParameters++];
      target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      target.ShaderVisibility = currentVisibility;
      target.DescriptorTable.NumDescriptorRanges = rangeSize;
      target.DescriptorTable.pDescriptorRanges = rangePosition;

      rangePosition += rangeSize;
      rangeSize = 0;
      G_ASSERT(rangePosition <= eastl::end(ranges));

      signatureCost += 1; // offset into active descriptor heap
    }
  }
  void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
  {
    if (shouldUseConstantBufferRootDescriptors())
    {
      G_UNUSED(linear_index);
      G_ASSERT(desc.NumParameters < countof(params));

      auto &target = params[desc.NumParameters++];
      target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
      target.ShaderVisibility = currentVisibility;
      target.Descriptor.ShaderRegister = slot;
      target.Descriptor.RegisterSpace = space;

      signatureCost += 2; // cbuffer is a 64bit gpu address
      ++rangeSize;
    }
    else
    {
      auto &target = rangePosition[rangeSize++];
      G_ASSERT(&target < eastl::end(ranges));
      target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      target.NumDescriptors = 1;
      target.BaseShaderRegister = slot;
      target.RegisterSpace = space;
      target.OffsetInDescriptorsFromTableStart = linear_index;
    }
  }
  void beginSamplers()
  {
    rangeSize = 0;
    static_cast<D *>(this)->getStageInfo().samplersParamIndex = desc.NumParameters;
  }
  void endSamplers()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = currentVisibility;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
  {
    auto &target = rangePosition[rangeSize++];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    target.NumDescriptors = 1;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = linear_index;
  }
  void beginBindlessSamplers()
  {
    if (unboundedSamplersRootParam == nullptr)
    {
      G_ASSERT(desc.NumParameters < countof(params));
      signature->def.layout.bindlessSamplersParamIndex = desc.NumParameters++;
      unboundedSamplersRootParam = &params[signature->def.layout.bindlessSamplersParamIndex];
      unboundedSamplersRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      unboundedSamplersRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
      unboundedSamplersRootParam->ShaderVisibility = currentVisibility;
      signatureCost += 1; // 1 root param for all unbounded sampler array ranges
    }
    // Note: - We cant "OR" ShaderVisibilty flags together, so if we need this for more than one stage, just use
    //         D3D12_SHADER_VISIBILITY_ALL
    else
    {
      G_ASSERTF(unboundedSamplersRootParam->ShaderVisibility != currentVisibility,
        "beginBindlessSamplers() shouldn't be called with the same visibility more than once");
      unboundedSamplersRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
  }
  void endBindlessSamplers() { G_ASSERT(unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges != 0); }
  void bindlessSamplers(uint32_t space, uint32_t slot)
  {
    G_ASSERT(space < dxil::MAX_UNBOUNDED_REGISTER_SPACES);

    // Deduplicate ranges
    for (int i = 0; i < unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges; i++)
    {
      auto &range = unboundedSamplersRootParam->DescriptorTable.pDescriptorRanges[i];
      if (slot == range.BaseShaderRegister && space == range.RegisterSpace)
        return;
    }

    auto &smpRange = rangePosition[0];
    G_ASSERT(&smpRange < eastl::end(ranges));
    smpRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    smpRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
    smpRange.BaseShaderRegister = slot;
    smpRange.RegisterSpace = space;
    smpRange.OffsetInDescriptorsFromTableStart = 0;

    unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges++;
    rangePosition++;
    G_ASSERT(rangePosition <= eastl::end(ranges));
  }
  void beginShaderResourceViews()
  {
    rangeSize = 0;
    static_cast<D *>(this)->getStageInfo().shaderResourceViewParamIndex = desc.NumParameters;
  }
  void endShaderResourceViews()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = currentVisibility;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
  {
    auto &target = rangePosition[rangeSize++];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    target.NumDescriptors = descriptor_count;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = linear_index;
  }
  void beginBindlessShaderResourceViews()
  {
    if (bindlessSRVRootParam == nullptr)
    {
      G_ASSERT(desc.NumParameters < countof(params));
      signature->def.layout.bindlessShaderResourceViewParamIndex = desc.NumParameters++;
      bindlessSRVRootParam = &params[signature->def.layout.bindlessShaderResourceViewParamIndex];
      bindlessSRVRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      bindlessSRVRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
      bindlessSRVRootParam->ShaderVisibility = currentVisibility;
      signatureCost += 1; // 1 root param for all unbounded sampler array ranges
    }
    // Note: - We cant "OR" ShaderVisibilty flags together, so if we need this for more than one stage, just use
    //         D3D12_SHADER_VISIBILITY_ALL
    else
    {
      G_ASSERTF(bindlessSRVRootParam->ShaderVisibility != currentVisibility,
        "beginBindlessShaderResourceViews() shouldn't be called with the same visibility more than once");
      bindlessSRVRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
  }
  void endBindlessShaderResourceViews() {}
  void bindlessShaderResourceViews(uint32_t space, uint32_t slot)
  {
    // Deduplicate ranges
    for (int i = 0; i < bindlessSRVRootParam->DescriptorTable.NumDescriptorRanges; i++)
    {
      auto &range = bindlessSRVRootParam->DescriptorTable.pDescriptorRanges[i];
      if (slot == range.BaseShaderRegister && space == range.RegisterSpace)
        return;
    }

    auto &registerRange = rangePosition[0];
    G_ASSERT(&registerRange < eastl::end(ranges));
    registerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    registerRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
    registerRange.BaseShaderRegister = slot;
    registerRange.RegisterSpace = space;
    registerRange.OffsetInDescriptorsFromTableStart = 0;

    bindlessSRVRootParam->DescriptorTable.NumDescriptorRanges++;
    rangePosition++;
    G_ASSERT(rangePosition <= eastl::end(ranges));
  }
  void beginUnorderedAccessViews()
  {
    rangeSize = 0;
    static_cast<D *>(this)->getStageInfo().unorderedAccessViewParamIndex = desc.NumParameters;
  }
  void endUnorderedAccessViews()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = currentVisibility;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
  {
    auto &target = rangePosition[rangeSize++];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    target.NumDescriptors = descriptor_count;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = linear_index;
  }
};
struct GraphicsRootSignatureGenerator : BasicGraphicsRootSignatureGenerator<5, GraphicsRootSignatureGenerator>
{
  using BaseType = BasicGraphicsRootSignatureGenerator<5, GraphicsRootSignatureGenerator>;
  RootSignatureStageLayout &getStageInfo()
  {
    switch (currentVisibility)
    {
      default:
      case D3D12_SHADER_VISIBILITY_ALL:
        DAG_FATAL("DX12: Trying to update pipeline data for unexpected shader visibility!");
        [[fallthrough]];
      case D3D12_SHADER_VISIBILITY_VERTEX: return signature->def.vsLayout;
      case D3D12_SHADER_VISIBILITY_PIXEL: return signature->def.psLayout;
      case D3D12_SHADER_VISIBILITY_HULL: return signature->def.hsLayout;
      case D3D12_SHADER_VISIBILITY_DOMAIN: return signature->def.dsLayout;
      case D3D12_SHADER_VISIBILITY_GEOMETRY: return signature->def.gsLayout;
    }
  }
  void begin()
  {
    BaseType::begin();
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
  }
  void end() {}
  void beginFlags() {}
  void endFlags() {}
  void hasVertexInputs() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; }
  void noVertexShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS; }
  void noHullShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; }
  void noDomainShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; }
  void noGeometryShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; }
  void setVisibilityVertexShader() { currentVisibility = D3D12_SHADER_VISIBILITY_VERTEX; }
  void setVisibilityHullShader() { currentVisibility = D3D12_SHADER_VISIBILITY_HULL; }
  void setVisibilityDomainShader() { currentVisibility = D3D12_SHADER_VISIBILITY_DOMAIN; }
  void setVisibilityGeometryShader() { currentVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY; }
};

#if !_TARGET_XBOXONE
struct GraphicsMeshRootSignatureGenerator : BasicGraphicsRootSignatureGenerator<3, GraphicsMeshRootSignatureGenerator>
{
  using BaseType = BasicGraphicsRootSignatureGenerator<3, GraphicsMeshRootSignatureGenerator>;
  RootSignatureStageLayout &getStageInfo()
  {
    switch (currentVisibility)
    {
      default:
      case D3D12_SHADER_VISIBILITY_ALL:
        DAG_FATAL("DX12: Trying to update pipeline data for unexpected shader visibility!");
        [[fallthrough]];
      case D3D12_SHADER_VISIBILITY_MESH: return signature->def.vsLayout;
      case D3D12_SHADER_VISIBILITY_PIXEL: return signature->def.psLayout;
      case D3D12_SHADER_VISIBILITY_AMPLIFICATION: return signature->def.gsLayout;
    }
  }
  void begin()
  {
    BaseType::begin();
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
  }
  void hasAmplificationStage()
  {
#if _TARGET_SCARLETT
    desc.Flags |= ROOT_SIGNATURE_FLAG_FORCE_MEMORY_BASED_ABI;
#endif
  }
  void end() {}
  void beginFlags() {}
  void endFlags() {}
  void noMeshShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS; }
  void noAmplificationShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS; }
  void setVisibilityMeshShader() { currentVisibility = D3D12_SHADER_VISIBILITY_MESH; }
  void setVisibilityAmplificationShader() { currentVisibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION; }
};
#endif
} // namespace

// TODO: add signature 1.1 support
// changes are very minor right now
// - in place cbv could be data static (needs verification that buffers never change after submit)
// - cbv range - descriptor static (we never change descriptors after submit) data volatile (never
// know when we have a buffer that has changed)
// - uav range - descriptor static (we never change descriptors after submit) data volatile (never
// know when a buffer / image change, need metadata from shader for this)
// - sampler - descriptor static
GraphicsPipelineSignature *PipelineManager::getGraphicsPipelineSignature(ID3D12Device *device, PipelineCache &cache,
  bool has_vertex_input, bool has_acceleration_structure, const dxil::ShaderResourceUsageTable &vs_header,
  const dxil::ShaderResourceUsageTable &ps_header, const dxil::ShaderResourceUsageTable *gs_header,
  const dxil::ShaderResourceUsageTable *hs_header, const dxil::ShaderResourceUsageTable *ds_header)
{
  dxil::ShaderResourceUsageTable defaultHeader = {};
  if (!gs_header)
    gs_header = &defaultHeader;
  if (!hs_header)
    hs_header = &defaultHeader;
  if (!ds_header)
    ds_header = &defaultHeader;

  auto ref = eastl::find_if(begin(graphicsSignatures), end(graphicsSignatures),
    [=, &vs_header, &ps_header](const eastl::unique_ptr<GraphicsPipelineSignature> &gps) //
    {
      return (gps->def.vertexShaderRegisters == vs_header) && (gps->def.pixelShaderRegisters == ps_header) &&
             (gps->def.geometryShaderRegisters == *gs_header) && (gps->def.hullShaderRegisters == *hs_header) &&
             (gps->def.domainShaderRegisters == *ds_header) && (gps->def.hasVertexInputs == has_vertex_input)
#if _TARGET_SCARLETT
             && (gps->def.hasAccelerationStructure == has_acceleration_structure)
#endif
        ;
    });

  if (ref == end(graphicsSignatures) || (*ref)->signature.Get() == nullptr)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    AutoFuncProf funcProfiler("PipelineManager::getGraphicsPipelineSignature->compile: took %dus");
#endif
    auto newSign = eastl::make_unique<GraphicsPipelineSignature>();
    newSign->def.hasVertexInputs = has_vertex_input;
#if _TARGET_SCARLETT
    newSign->def.hasAccelerationStructure = has_acceleration_structure;
#endif
    newSign->def.vertexShaderRegisters = vs_header;
    newSign->def.pixelShaderRegisters = ps_header;
    newSign->def.geometryShaderRegisters = *gs_header;
    newSign->def.hullShaderRegisters = *hs_header;
    newSign->def.domainShaderRegisters = *ds_header;
    newSign->deriveComboMasks();

    GraphicsRootSignatureGenerator generator;
    generator.signature = newSign.get();
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    generator.useConstantBufferRootDescriptors = !rootSignaturesUsesCBVDescriptorRanges;
#endif
    decode_graphics_root_signature(has_vertex_input, has_acceleration_structure, vs_header, ps_header, *hs_header, *ds_header,
      *gs_header, generator);

    G_ASSERTF(generator.signatureCost < 64, "Too many root signature parameters, the limit is 64 but this one needs %u",
      generator.signatureCost);
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    logdbg("DX12: Generating graphics signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;
    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      DAG_FATAL("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
      return nullptr;
    }
    auto result = DX12_CHECK_RESULT(
      device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&newSign->signature)));
    if (E_INVALIDARG == result)
    {
      return nullptr;
    }
    cache.addGraphicsSignature(newSign->def, rootSignBlob.Get());
    if (ref == end(graphicsSignatures))
      ref = graphicsSignatures.insert(ref, eastl::move(newSign));
    else
      (*ref)->signature.Swap(newSign->signature);
  }

  return ref->get();
}

#if !_TARGET_XBOXONE
GraphicsPipelineSignature *PipelineManager::getGraphicsMeshPipelineSignature(ID3D12Device *device, PipelineCache &cache,
  bool has_acceleration_structure, const dxil::ShaderResourceUsageTable &ms_header, const dxil::ShaderResourceUsageTable &ps_header,
  const dxil::ShaderResourceUsageTable *as_header)
{
  dxil::ShaderResourceUsageTable defaultHeader = {};
  auto &safeAsHeader = as_header ? *as_header : defaultHeader;

  auto ref = eastl::find_if(begin(graphicsMeshSignatures), end(graphicsMeshSignatures),
    [=, &safeAsHeader, &ms_header, &ps_header](const eastl::unique_ptr<GraphicsPipelineSignature> &gps) //
    {
      return (gps->def.vertexShaderRegisters == ms_header) && (gps->def.pixelShaderRegisters == ps_header) &&
             (gps->def.geometryShaderRegisters == safeAsHeader)
#if _TARGET_SCARLETT
             && (gps->def.hasAccelerationStructure == has_acceleration_structure)
#endif
        ;
    });

  if (ref == end(graphicsMeshSignatures) || (*ref)->signature.Get() == nullptr)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    AutoFuncProf funcProfiler("PipelineManager::getGraphicsMeshPipelineSignature->compile: took %dus");
#endif
    auto newSign = eastl::make_unique<GraphicsPipelineSignature>();
    newSign->def.hasVertexInputs = false;
#if _TARGET_SCARLETT
    newSign->def.hasAccelerationStructure = has_acceleration_structure;
#endif
    newSign->def.vertexShaderRegisters = ms_header;
    newSign->def.pixelShaderRegisters = ps_header;
    newSign->def.geometryShaderRegisters = safeAsHeader;
    newSign->deriveComboMasks();

    GraphicsMeshRootSignatureGenerator generator;
    generator.signature = newSign.get();
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    generator.useConstantBufferRootDescriptors = !rootSignaturesUsesCBVDescriptorRanges;
#endif
    decode_graphics_mesh_root_signature(has_acceleration_structure, ms_header, ps_header, as_header, generator);

    G_ASSERTF(generator.signatureCost < 64, "Too many root signature parameters, the limit is 64 but this one needs %u",
      generator.signatureCost);
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    logdbg("DX12: Generating graphics mesh signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;
    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      DAG_FATAL("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
      return nullptr;
    }
    auto result = DX12_CHECK_RESULT(
      device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&newSign->signature)));
    if (E_INVALIDARG == result)
    {
      return nullptr;
    }
    cache.addGraphicsMeshSignature(newSign->def, rootSignBlob.Get());
    if (ref == end(graphicsMeshSignatures))
      ref = graphicsMeshSignatures.insert(ref, eastl::move(newSign));
    else
      (*ref)->signature.Swap(newSign->signature);
  }

  return ref->get();
}
#endif

// TODO: add signature 1.1 support
ComputePipelineSignature *PipelineManager::getComputePipelineSignature(ID3D12Device *device, PipelineCache &cache,
  bool has_acceleration_structure, const dxil::ShaderResourceUsageTable &cs_header)
{
  auto ref = eastl::find_if(begin(computeSignatures), end(computeSignatures),
    [&cs_header](const eastl::unique_ptr<ComputePipelineSignature> &cps) //
    { return cps->def.registers == cs_header; });

  if (ref == end(computeSignatures) || (*ref)->signature.Get() == nullptr)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    AutoFuncProf funcProfiler("PipelineManager::getComputePipelineSignature->compile: took %dus");
#endif
    auto newSign = eastl::make_unique<ComputePipelineSignature>();
    newSign->def.registers = cs_header;
    struct GeneratorCallback
    {
      ComputePipelineSignature *signature = nullptr;
      D3D12_DESCRIPTOR_RANGE ranges[dxil::MAX_T_REGISTERS + dxil::MAX_S_REGISTERS + dxil::MAX_U_REGISTERS + dxil::MAX_B_REGISTERS] =
        {};
      // cbuffer each one param, then srv, uav and sampler each one and one for each bindless sampler and srv globally
      D3D12_ROOT_PARAMETER params[dxil::MAX_B_REGISTERS + 1 + 1 + 1 + 2] = {};
      D3D12_ROOT_SIGNATURE_DESC desc = {};
      D3D12_DESCRIPTOR_RANGE *rangePosition = &ranges[0];
      D3D12_ROOT_PARAMETER *unboundedSamplersRootParam = nullptr;
      D3D12_ROOT_PARAMETER *bindlessSRVRootParam = nullptr;
      uint32_t rangeSize = 0;
      uint32_t signatureCost = 0;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
      bool useConstantBufferRootDescriptors = false;
      bool shouldUseConstantBufferRootDescriptors() const { return useConstantBufferRootDescriptors; }
#else
      constexpr bool shouldUseConstantBufferRootDescriptors() const { return true; }
#endif
      void begin()
      {
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        desc.pParameters = params;
      }
      void end() {}
      void beginFlags() {}
      void endFlags() {}
      void hasAccelerationStructure()
      {
#if _TARGET_SCARLETT
        desc.Flags |= ROOT_SIGNATURE_FLAG_RAYTRACING;
#endif
      }
      void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
      {
        G_ASSERT(desc.NumParameters < countof(params));

        signature->def.csLayout.rootConstantsParamIndex = desc.NumParameters;

        auto &target = params[desc.NumParameters++];
        target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        target.Constants.ShaderRegister = index;
        target.Constants.RegisterSpace = space;
        target.Constants.Num32BitValues = dwords;

        signatureCost += dwords;
      }
      void beginConstantBuffers()
      {
        signature->def.csLayout.setConstBufferDescriptorIndex(desc.NumParameters, shouldUseConstantBufferRootDescriptors());
      }
      void endConstantBuffers()
      {
        if (!shouldUseConstantBufferRootDescriptors())
        {
          G_ASSERT(desc.NumParameters < countof(params));
          G_ASSERT(rangeSize > 0);

          auto &target = params[desc.NumParameters++];
          target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
          target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
          target.DescriptorTable.NumDescriptorRanges = rangeSize;
          target.DescriptorTable.pDescriptorRanges = rangePosition;

          rangePosition += rangeSize;
          rangeSize = 0;
          G_ASSERT(rangePosition <= eastl::end(ranges));

          signatureCost += 1; // offset into active descriptor heap
        }
      }
      void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
      {
        if (shouldUseConstantBufferRootDescriptors())
        {
          G_UNUSED(linear_index);
          G_ASSERT(desc.NumParameters < countof(params));

          auto &target = params[desc.NumParameters++];
          target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
          target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
          target.Descriptor.ShaderRegister = slot;
          target.Descriptor.RegisterSpace = space;

          signatureCost += 2; // cbuffer is a 64bit gpu address
        }
        else
        {
          auto &target = rangePosition[rangeSize++];
          G_ASSERT(&target < eastl::end(ranges));
          target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
          target.NumDescriptors = 1;
          target.BaseShaderRegister = slot;
          target.RegisterSpace = space;
          target.OffsetInDescriptorsFromTableStart = linear_index;
        }
      }
      void beginSamplers() { signature->def.csLayout.samplersParamIndex = desc.NumParameters; }
      void endSamplers()
      {
        G_ASSERT(desc.NumParameters < countof(params));
        G_ASSERT(rangeSize > 0);

        auto &target = params[desc.NumParameters++];
        target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        target.DescriptorTable.NumDescriptorRanges = rangeSize;
        target.DescriptorTable.pDescriptorRanges = rangePosition;

        rangePosition += rangeSize;
        rangeSize = 0;
        G_ASSERT(rangePosition <= eastl::end(ranges));

        signatureCost += 1; // offset into active descriptor heap
      }
      void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
      {
        G_UNUSED(linear_index); // rangeSize will be the same as linear_index
        auto &target = rangePosition[rangeSize];
        G_ASSERT(&target < eastl::end(ranges));
        target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        target.NumDescriptors = 1;
        target.BaseShaderRegister = slot;
        target.RegisterSpace = space;
        target.OffsetInDescriptorsFromTableStart = rangeSize++;
      }
      void beginBindlessSamplers()
      {
        // compute has only one stage, all unbounded samplers should be added within a single begin-end block
        G_ASSERT(unboundedSamplersRootParam == nullptr);
        G_ASSERT(desc.NumParameters < countof(params));
        signature->def.layout.bindlessSamplersParamIndex = desc.NumParameters++;
        unboundedSamplersRootParam = &params[signature->def.layout.bindlessSamplersParamIndex];
        unboundedSamplersRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        unboundedSamplersRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
        unboundedSamplersRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        signatureCost += 1; // 1 root param for all unbounded sampler array ranges
      }
      void endBindlessSamplers() { G_ASSERT(unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges != 0); }
      void bindlessSamplers(uint32_t space, uint32_t slot)
      {
        G_ASSERT(space < dxil::MAX_UNBOUNDED_REGISTER_SPACES);

        auto &smpRange = rangePosition[0];
        G_ASSERT(&smpRange < eastl::end(ranges));
        smpRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        smpRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
        smpRange.BaseShaderRegister = slot;
        smpRange.RegisterSpace = space;
        smpRange.OffsetInDescriptorsFromTableStart = 0;

        unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges++;
        rangePosition++;
        G_ASSERT(rangePosition <= eastl::end(ranges));
      }
      void beginShaderResourceViews() { signature->def.csLayout.shaderResourceViewParamIndex = desc.NumParameters; }
      void endShaderResourceViews()
      {
        G_ASSERT(desc.NumParameters < countof(params));
        G_ASSERT(rangeSize > 0);

        auto &target = params[desc.NumParameters++];
        target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        target.DescriptorTable.NumDescriptorRanges = rangeSize;
        target.DescriptorTable.pDescriptorRanges = rangePosition;

        rangePosition += rangeSize;
        rangeSize = 0;
        G_ASSERT(rangePosition <= eastl::end(ranges));

        signatureCost += 1; // offset into active descriptor heap
      }
      void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
      {
        auto &target = rangePosition[rangeSize++];
        G_ASSERT(&target < eastl::end(ranges));
        target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        target.NumDescriptors = descriptor_count;
        target.BaseShaderRegister = slot;
        target.RegisterSpace = space;
        target.OffsetInDescriptorsFromTableStart = linear_index;
      }
      void beginBindlessShaderResourceViews()
      {
        G_ASSERT(desc.NumParameters < countof(params));
        signature->def.layout.bindlessShaderResourceViewParamIndex = desc.NumParameters++;
        bindlessSRVRootParam = &params[signature->def.layout.bindlessShaderResourceViewParamIndex];
        bindlessSRVRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        bindlessSRVRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
        bindlessSRVRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        signatureCost += 1; // 1 root param for all unbounded sampler array ranges
      }
      void endBindlessShaderResourceViews() {}
      void bindlessShaderResourceViews(uint32_t space, uint32_t slot)
      {
        G_ASSERT(space < dxil::MAX_UNBOUNDED_REGISTER_SPACES);

        auto &registerRange = rangePosition[0];
        G_ASSERT(&registerRange < eastl::end(ranges));
        registerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        registerRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
        registerRange.BaseShaderRegister = slot;
        registerRange.RegisterSpace = space;
        registerRange.OffsetInDescriptorsFromTableStart = 0;

        bindlessSRVRootParam->DescriptorTable.NumDescriptorRanges++;
        rangePosition++;
        G_ASSERT(rangePosition <= eastl::end(ranges));
      }
      void beginUnorderedAccessViews() { signature->def.csLayout.unorderedAccessViewParamIndex = desc.NumParameters; }
      void endUnorderedAccessViews()
      {
        G_ASSERT(desc.NumParameters < countof(params));
        G_ASSERT(rangeSize > 0);

        auto &target = params[desc.NumParameters++];
        target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        target.DescriptorTable.NumDescriptorRanges = rangeSize;
        target.DescriptorTable.pDescriptorRanges = rangePosition;

        rangePosition += rangeSize;
        rangeSize = 0;
        G_ASSERT(rangePosition <= eastl::end(ranges));

        signatureCost += 1; // offset into active descriptor heap
      }
      void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
      {
        auto &target = rangePosition[rangeSize++];
        G_ASSERT(&target < eastl::end(ranges));
        target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        target.NumDescriptors = descriptor_count;
        target.BaseShaderRegister = slot;
        target.RegisterSpace = space;
        target.OffsetInDescriptorsFromTableStart = linear_index;
      }
    };
    GeneratorCallback generator;
    generator.signature = newSign.get();
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    generator.useConstantBufferRootDescriptors = !rootSignaturesUsesCBVDescriptorRanges;
#endif
    decode_compute_root_signature(has_acceleration_structure, cs_header, generator);
    G_ASSERTF(generator.signatureCost < 64, "Too many root signature parameters, the limit is 64 but this one needs %u",
      generator.signatureCost);
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    logdbg("DX12: Generating compute pipeline signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;

    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      DAG_FATAL("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
      return nullptr;
    }

    auto result = DX12_CHECK_RESULT(
      device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&newSign->signature)));
    if (E_INVALIDARG == result)
    {
      return nullptr;
    }

    cache.addComputeSignature(newSign->def, rootSignBlob.Get());

    if (ref == end(computeSignatures))
      ref = computeSignatures.insert(ref, eastl::move(newSign));
    else
      (*ref)->signature.Swap(newSign->signature);
  }

  return ref->get();
}

ComputePipeline *PipelineManager::getCompute(ProgramID program)
{
  G_ASSERTF(program.isCompute(), "getCompute called for a non compute program!");
  uint32_t index = program.getIndex();
  auto &pipelineGroup = computePipelines[program.getGroup()];
  G_ASSERTF(pipelineGroup[index] != nullptr, "getCompute called for uninitialized compute pipeline! index was %u", index);
  return pipelineGroup[index].get();
}

BasePipeline *PipelineManager::getGraphics(GraphicsProgramID program)
{
  uint32_t index = program.getIndex();
  auto &pipelineGroup = graphicsPipelines[program.getGroup()];
  G_ASSERTF(pipelineGroup[index] != nullptr, "getGraphics called for uninitialized graphics pipeline! index was %u", index);
  return pipelineGroup[index].get();
}

void PipelineManager::addCompute(ID3D12Device2 *device, PipelineCache &cache, ProgramID id, ComputeShaderModule shader,
  RecoverablePipelineCompileBehavior on_error, bool give_name, CSPreloaded preloaded)
{
  G_ASSERTF(id.isCompute(), "addCompute called for a non compute program!");
  uint32_t index = id.getIndex();
  auto &pipelineGroup = computePipelines[id.getGroup()];
  while (pipelineGroup.size() <= index)
    pipelineGroup.emplace_back(nullptr);

  if (pipelineGroup[index])
    return;

  if (!isCompatible(shader))
  {
    logdbg("DX12: ...a compute shader requires features that the device can not provide, ignoring...");
    return;
  }
#if _TARGET_SCARLETT
  bool hasAccelerationStructure = has_acceleration_structure(shader);
#else
  bool hasAccelerationStructure = false;
#endif
  if (auto signature = getComputePipelineSignature(device, cache, hasAccelerationStructure, shader.header.resourceUsageTable))
  {
    if (preloaded == CSPreloaded::Yes && hasFeatureSetInCache)
      needToUpdateCache = true;
    pipelineGroup[index] =
      eastl::make_unique<ComputePipeline>(*signature, eastl::move(shader), device, cache, on_error, give_name, *this);
  }
}

void PipelineManager::addGraphics(ID3D12Device2 *device, PipelineCache &cache, FramebufferLayoutManager &fbs,
  GraphicsProgramID program, ShaderID vs, ShaderID ps, RecoverablePipelineCompileBehavior on_error, bool give_name)
{
  uint32_t index = program.getIndex();
  auto &pipelineGroup = graphicsPipelines[program.getGroup()];
  while (pipelineGroup.size() <= index)
    pipelineGroup.emplace_back(nullptr);

  auto &target = pipelineGroup[index];

  if (target)
  {
    return;
  }

  auto preloadedPipelineIt = eastl::find_if(preloadedGraphicsPipelines.begin(), preloadedGraphicsPipelines.end(),
    [&vs, &ps](const auto &pp) { return (pp.vsID == vs) && (pp.psID == ps); });

  if (preloadedPipelineIt != preloadedGraphicsPipelines.end())
  {
    target = eastl::move(preloadedPipelineIt->pipeline);
    *preloadedPipelineIt = eastl::move(preloadedGraphicsPipelines.back());
    preloadedGraphicsPipelines.pop_back();
  }
  else
  {
    auto vertexShader = getVertexShader(vs);
    auto pixelShader = getPixelShader(ps);
    if (!isCompatible(vertexShader) || !isCompatible(pixelShader))
    {
      logdbg("DX12: ...a shader (%u) requires features that the device can not provide, ignoring...",
        (isCompatible(vertexShader) ? 1 : 0) | (isCompatible(pixelShader) ? 2 : 0));
      return;
    }

    target = createGraphics(device, cache, fbs, vertexShader, pixelShader, on_error, give_name);
    if (target && hasFeatureSetInCache)
      needToUpdateCache = true;
  }
  if (target)
  {
    target->setProgramId(program);
  }
}

eastl::unique_ptr<BasePipeline> PipelineManager::createGraphics(ID3D12Device2 *device, PipelineCache &cache,
  FramebufferLayoutManager &fbs, backend::VertexShaderModuleRefStore vertexShader, backend::PixelShaderModuleRefStore pixelShader,
  RecoverablePipelineCompileBehavior on_error, bool give_name)
{
#if _TARGET_SCARLETT
  bool hasAccelerationStructure = has_acceleration_structure(vertexShader);
  if (!hasAccelerationStructure)
    hasAccelerationStructure = has_acceleration_structure(pixelShader);
  if (!hasAccelerationStructure && get_gs(vertexShader))
    hasAccelerationStructure = has_acceleration_structure(*get_gs(vertexShader));
  if (!hasAccelerationStructure && get_hs(vertexShader))
    hasAccelerationStructure = has_acceleration_structure(*get_hs(vertexShader));
  if (!hasAccelerationStructure && get_ds(vertexShader))
    hasAccelerationStructure = has_acceleration_structure(*get_ds(vertexShader));
#else
  bool hasAccelerationStructure = false;
#endif

  auto &psHeader = pixelShader.header.header.resourceUsageTable;

  GraphicsPipelineSignature *signature = nullptr;
#if !_TARGET_XBOXONE
  if (is_mesh(vertexShader))
  {
    auto &msHeader = vertexShader.header.header.resourceUsageTable;
    auto as = get_as(vertexShader);
    auto asHeader = as ? &as->resourceUsageTable : nullptr;
    signature = getGraphicsMeshPipelineSignature(device, cache, hasAccelerationStructure, msHeader, psHeader, asHeader);
  }
  else
#endif
  {
    bool hasVertexInputs = 0 != vertexShader.header.header.inOutSemanticMask;
    auto &vsHeader = vertexShader.header.header.resourceUsageTable;
    auto gs = get_gs(vertexShader);
    auto gsHeader = gs ? &gs->resourceUsageTable : nullptr;
    auto hs = get_hs(vertexShader);
    auto hsHeader = hs ? &hs->resourceUsageTable : nullptr;
    auto ds = get_ds(vertexShader);
    auto dsHeader = ds ? &ds->resourceUsageTable : nullptr;
    signature = getGraphicsPipelineSignature(device, cache, hasVertexInputs, hasAccelerationStructure, vsHeader, psHeader, gsHeader,
      hsHeader, dsHeader);
  }
  if (!signature)
  {
    return nullptr;
  }
  return eastl::make_unique<BasePipeline>(device, *signature, cache, static_cast<backend::ShaderModuleManager &>(*this),
    static_cast<backend::InputLayoutManager &>(*this), static_cast<backend::StaticRenderStateManager &>(*this), fbs, vertexShader,
    pixelShader, on_error, give_name, static_cast<backend::PipelineNameGenerator &>(*this));
}

void PipelineManager::unloadAll()
{
  blitPipelines.clear();
  clearPipelines.clear();
  blitSignature.Reset();
  clearSignature.Reset();

  for (auto &group : graphicsPipelines)
  {
    for (auto &pipe : group)
    {
      if (pipe)
      {
        pipe->unloadAll();
      }
    }
    group.clear();
  }

  graphicsSignatures.clear();

  for (auto &group : computePipelines)
    group.clear();
  computeSignatures.clear();

#if D3D_HAS_RAY_TRACING
  raytracePipelines.clear();
  raytraceSignatures.clear();
#endif
}

void PipelineManager::removeProgram(ProgramID program)
{
  auto ref = eastl::find_if(begin(prepedForDeletion), end(prepedForDeletion),
    [=](const PrepedForDelete &pfd) { return pfd.program == program; });
  if (program.isCompute())
    ; // no-op
#if D3D_HAS_RAY_TRACING
  else if (program.isRaytrace())
    ; // no-op
#endif
  else
  {
    G_ASSERTF(false, "Invalid program type");
  }
  *ref = eastl::move(prepedForDeletion.back());
  prepedForDeletion.pop_back();
}

void PipelineManager::removeProgram(GraphicsProgramID program)
{
  auto ref = eastl::find_if(begin(prepedForDeletion), end(prepedForDeletion),
    [=](const PrepedForDelete &pfd) { return pfd.graphicsProgram == program; });
  G_ASSERT(ref != end(prepedForDeletion));
  if (ref->graphics.get())
    ref->graphics->unloadAll();
  *ref = eastl::move(prepedForDeletion.back());
  prepedForDeletion.pop_back();
}

void PipelineManager::prepareForRemove(ProgramID program)
{
  PrepedForDelete info;
  info.program = program;
  info.graphicsProgram = GraphicsProgramID::Null();

  auto index = program.getIndex();
  auto group = program.getGroup();
  if (program.isCompute())
  {
    info.compute = eastl::move(computePipelines[group][index]);
  }
#if D3D_HAS_RAY_TRACING
  else if (program.isRaytrace())
  {
    info.raytrace = eastl::move(raytracePipelines[index]);
  }
#endif
  else
  {
    G_ASSERTF(false, "Invalid program type");
  }
  prepedForDeletion.push_back(eastl::move(info));
}

void PipelineManager::prepareForRemove(GraphicsProgramID program)
{
  PrepedForDelete info;
  info.program = ProgramID::Null();
  info.graphicsProgram = program;
  info.graphics = eastl::move(graphicsPipelines[program.getGroup()][program.getIndex()]);
  prepedForDeletion.push_back(eastl::move(info));
}

#if D3D_HAS_RAY_TRACING
RaytracePipeline &PipelineManager::getRaytraceProgram(ProgramID program) { return *raytracePipelines[program.getIndex()]; }

void PipelineManager::copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size,
  void *ptr)
{
  uint32_t index = prog.getIndex();
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(ptr);
  G_UNUSED(index);
  // TODO
#if 0
  raytracePipelines[index]->copyRaytraceShaderGroupHandlesToMemory(device, first_group, group_count,
                                                                   size, ptr);
#endif
}

void PipelineManager::addRaytrace(ID3D12Device *device, PipelineCache &cache, ProgramID id, uint32_t max_recursion,
  uint32_t shader_count, const RaytraceShaderModule *shaders, uint32_t group_count, const RaytraceShaderGroup *groups)
{
  // TODO
  G_UNUSED(device);
  G_UNUSED(cache);
  G_UNUSED(id);
  G_UNUSED(max_recursion);
  G_UNUSED(shader_count);
  G_UNUSED(shaders);
  G_UNUSED(group_count);
  G_UNUSED(groups);
#if 0
  WinAutoLock lock(guard);
  uint32_t index = program_to_index(id);
  while (raytracePipelines.size() <= index)
    raytracePipelines.push_back(nullptr);

  if (raytracePipelines[index])
    return;

  uint32_t rayGenIndex = 0;
  for (; rayGenIndex < shader_count; ++rayGenIndex)
  {
    if (shaders[rayGenIndex].header.stage == VK_SHADER_STAGE_RAYGEN_BIT_NV)
      break;
  }

  G_ASSERT(rayGenIndex < shader_count);
  if (rayGenIndex >= shader_count)
  {
    DAG_FATAL("Missing raygen shader for raytrace program");
  }

  // TODO: may do some magic with the whole set of shaders
  const auto &layoutHeader = shaders[rayGenIndex].header;
  RaytracePipelineLayout *layout = nullptr;
  for (auto &&l : raytraceLayouts)
  {
    if (l->matches(layoutHeader))
    {
      layout = l.get();
      break;
    }
  }

  if (!layout)
  {
    layout = new RaytracePipelineLayout(device, layoutHeader);
    raytraceLayouts.emplace_back(layout);
  }

  raytracePipelines[index].reset(new RaytracePipeline(device, cache, layout, id, max_recursion,
                                                      shader_count, shaders, group_count, groups, module_set));
#endif
}
#endif

bool PipelineManager::recover(ID3D12Device2 *device, PipelineCache &cache)
{
#if _TARGET_PC_WIN
  // no need to handle this list, we have to rebuild everything anyway
  prepedForDeletion.clear();
  createBlitSignature(device);
  createClearSignature(device);
  // just reset the list and they will be rebuild on demand
  blitPipelines.clear();
  clearPipelines.clear();

#if D3D_HAS_RAY_TRACING
  // TODO NYI
#endif

  // have to keep signature pointers alive as pipelines reference them
  for (auto &&cs : computeSignatures)
  {
    cs->signature.Reset();
    getComputePipelineSignature(device, cache, false, cs->def.registers);
  }

  // have to keep signature pointers alive as pipelines reference them
  for (auto &&gs : graphicsSignatures)
  {
    gs->signature.Reset();
    getGraphicsPipelineSignature(device, cache, gs->def.hasVertexInputs, false, gs->def.vertexShaderRegisters,
      gs->def.pixelShaderRegisters, &gs->def.geometryShaderRegisters, &gs->def.hullShaderRegisters, &gs->def.domainShaderRegisters);
  }

  for (auto &&gs : graphicsMeshSignatures)
  {
    gs->signature.Reset();
    getGraphicsMeshPipelineSignature(device, cache, false, gs->def.vertexShaderRegisters, gs->def.pixelShaderRegisters,
      &gs->def.geometryShaderRegisters);
  }

  // just drop all variants and lets reload them on next draw call that uses the variants
  for (auto &group : graphicsPipelines)
  {
    for (auto &pipe : group)
    {
      if (pipe)
      {
        pipe->unloadAll();
      }
    }
  }
#else
  G_UNUSED(device);
  G_UNUSED(cache);
#endif
  return true;
}

void PipelineManager::preRecovery()
{
  blitPipelines.clear();
  clearPipelines.clear();
  blitSignature.Reset();
  clearSignature.Reset();
  prepedForDeletion.clear();

#if D3D_HAS_RAY_TRACING
  for (auto &&pipeline : raytracePipelines)
    pipeline->preRecovery();
  for (auto &&signature : raytraceSignatures)
    signature->signature.Reset();
#endif

  for (auto &group : computePipelines)
    for (auto &&pipeline : group)
      if (pipeline != nullptr)
        pipeline->preRecovery();
  for (auto &&signature : computeSignatures)
    signature->signature.Reset();

  for (auto &group : graphicsPipelines)
  {
    for (auto &pipe : group)
    {
      if (pipe)
      {
        pipe->preRecovery();
      }
    }
  }
  for (auto &&signature : graphicsSignatures)
    signature->signature.Reset();

  for (auto &pipe : preloadedGraphicsPipelines)
  {
    if (pipe.pipeline != nullptr)
    {
      pipe.pipeline->preRecovery();
    }
  }
}

namespace
{
// The signature is a set of shader class name and hash. The hash is sha1 over all intervals used
// by the class, iterated in order they appear, static variant intervals first, then for each
// static variant its intervals to select a pass. The hash of each interval is the hash of its
// name and the use totalMul value to ensure the value ranges are still the same.
void generate_scripted_shader_dump_signature(ScriptedShadersBinDumpOwner *dump, eastl::string_view name, DataBlock &block)
{
  auto dumpBlock = block.addNewBlock("dump");
  if (!dumpBlock)
  {
    return;
  }
  auto classBlock = dumpBlock->addNewBlock("classes");
  if (!classBlock)
  {
    return;
  }
  dumpBlock->addStr("name", name.data());
  char hashBuf[sizeof(ShaderHashValue) * 2 + 1]{};
  pipeline::SignatueHashEncoder encoder{};
  auto v1 = dump->getDump();
  for (auto &c : v1->classes)
  {
    calculate_hash(*v1, c).convertToString(hashBuf, sizeof(hashBuf));
    encoder.encode(*classBlock, static_cast<const char *>(c.name), hashBuf, c.getTimestamp());
  }
}

static bool dump_has_zero_timestamps(ScriptedShadersBinDumpOwner *dump)
{
  auto v1 = dump->getDump();
  for (auto &c : v1->classes)
    if (c.getTimestamp() == 0)
      return true;
  return false;
}

struct PipelineInfoCollector
{
  template <typename T>
  struct StateRemapTable
  {
    dag::Vector<T> table;
    uint32_t add(T value)
    {
      auto ref = eastl::find(begin(table), end(table), value);
      if (ref == end(table))
      {
        ref = table.insert(ref, value);
      }
      return eastl::distance(eastl::begin(table), ref);
    }
  };
  struct ShaderClassInfo
  {
    const char *name;
    eastl::vector<cacheBlk::UseCodes> uses;
    void addUse(uint32_t static_code, uint32_t dynamic_code)
    {
      auto ref = eastl::find_if(begin(uses), end(uses),
        [static_code, dynamic_code](auto &e) { return static_code == e.staticCode && dynamic_code == e.dynamicCode; });
      if (ref == end(uses))
      {
        cacheBlk::UseCodes e{static_code, dynamic_code};
        uses.push_back(e);
      }
    }
  };
  struct MeshVariantInfo
  {
    StaticRenderStateID staticRenderState;
    FramebufferLayoutID frameBufferLayout;
    bool isWF;
  };
  struct GraphicsVariantInfo : MeshVariantInfo
  {
    InternalInputLayoutID inputLayout;
    uint32_t topology;
  };
  struct PipelineVariant : GraphicsVariantInfo
  {
    eastl::vector<ShaderClassInfo> classes;

    ShaderClassInfo &getClass(const char *name)
    {
      auto ref = eastl::find_if(begin(classes), end(classes), [name](auto &cls) { return name == cls.name; });
      if (ref == end(classes))
      {
        ShaderClassInfo newClass;
        newClass.name = name;
        ref = classes.insert(ref, newClass);
      }
      return *ref;
    }
  };

  struct PixelPairGroup
  {
    eastl::vector<ShaderClassInfo> classes;
    eastl::vector<ShaderClassInfo> pixelShaderClasses;
  };
  struct PipelineVariantWithPixelShaderOverride : GraphicsVariantInfo
  {
    eastl::vector<PixelPairGroup> pairGroups;
    void beginNewGroup() { pairGroups.emplace_back(); }

    ShaderClassInfo &getClass(const char *name)
    {
      auto &source = pairGroups.back().classes;
      auto ref = eastl::find_if(begin(source), end(source), [name](auto &cls) { return name == cls.name; });
      if (ref == end(source))
      {
        ShaderClassInfo newClass;
        newClass.name = name;
        ref = source.insert(ref, newClass);
      }
      return *ref;
    }

    ShaderClassInfo &getPixelShaderClass(const char *name)
    {
      auto &source = pairGroups.back().pixelShaderClasses;
      auto ref = eastl::find_if(begin(source), end(source), [name](auto &cls) { return name == cls.name; });
      if (ref == end(source))
      {
        ShaderClassInfo newClass;
        newClass.name = name;
        ref = source.insert(ref, newClass);
      }
      return *ref;
    }
  };
  eastl::vector<PipelineVariant> variations;
  eastl::vector<PipelineVariant> nullVariations;
  eastl::vector<PipelineVariantWithPixelShaderOverride> pixelShaderOverrideVariations;
  eastl::vector<ShaderClassInfo> computeClasses;
  GraphicsVariantInfo currentVariant{};
  PipelineVariantWithPixelShaderOverride *lastVertexPixelOverrideTarget = nullptr;
  PipelineVariantWithPixelShaderOverride *lastPixelPixelOverrideTarget = nullptr;

  void beginPipeline(BasePipeline *) {}
  void beginVariant(InternalInputLayoutID input_layout_id, StaticRenderStateID static_render_state_id,
    FramebufferLayoutID frame_buffer_layout_id, uint32_t topology, bool is_wf)
  {
    currentVariant.inputLayout = input_layout_id;
    currentVariant.staticRenderState = static_render_state_id;
    currentVariant.frameBufferLayout = frame_buffer_layout_id;
    currentVariant.topology = topology;
    currentVariant.isWF = is_wf;
  }
  void beginMeshVariant(StaticRenderStateID static_render_state_id, FramebufferLayoutID frame_buffer_layout_id, bool is_wf)
  {
    currentVariant.staticRenderState = static_render_state_id;
    currentVariant.frameBufferLayout = frame_buffer_layout_id;
    currentVariant.isWF = is_wf;
  }
  void onShaderClassGraphicsUse(const char *shader_class, uint32_t static_code, uint32_t dynamic_code, bool has_render_state_override,
    bool use_null_pixel_shader, bool use_with_pixel_shader_override, bool is_pixel_shader_override)
  {
    if (use_with_pixel_shader_override)
    {
      if (is_pixel_shader_override)
      {
        if (!lastPixelPixelOverrideTarget)
        {
          lastPixelPixelOverrideTarget = lastVertexPixelOverrideTarget;
          lastVertexPixelOverrideTarget = nullptr;
        }
        lastPixelPixelOverrideTarget->getPixelShaderClass(shader_class).addUse(static_code, dynamic_code);
      }
      else
      {
        if (!lastVertexPixelOverrideTarget)
        {
          lastVertexPixelOverrideTarget = &findMatchingVariantWithPixelShaderOverride(has_render_state_override);
          lastVertexPixelOverrideTarget->beginNewGroup();
          lastPixelPixelOverrideTarget = nullptr;
        }
        lastVertexPixelOverrideTarget->getClass(shader_class).addUse(static_code, dynamic_code);
      }
    }
    else
    {
      PipelineVariant &variant = findMatchingVariant(has_render_state_override, use_null_pixel_shader);
      auto &cls = variant.getClass(shader_class);
      cls.addUse(static_code, dynamic_code);
    }
  }
  void onShaderClassMeshUse(const char *shader_class, uint32_t static_code, uint32_t dynamic_code, bool has_render_state_override,
    bool use_null_pixel_shader, bool use_with_pixel_shader_override, bool is_pixel_shader_override)
  {
    if (use_with_pixel_shader_override)
    {
      if (is_pixel_shader_override)
      {
        if (!lastPixelPixelOverrideTarget)
        {
          lastPixelPixelOverrideTarget = lastVertexPixelOverrideTarget;
          lastVertexPixelOverrideTarget = nullptr;
        }
        lastPixelPixelOverrideTarget->getPixelShaderClass(shader_class).addUse(static_code, dynamic_code);
      }
      else
      {
        if (!lastVertexPixelOverrideTarget)
        {
          lastVertexPixelOverrideTarget = &findMatchingMeshVariantWithPixelShaderOverride(has_render_state_override);
        }
        lastPixelPixelOverrideTarget->getClass(shader_class).addUse(static_code, dynamic_code);
      }
    }
    else
    {
      PipelineVariant &variant = findMatchingMeshVariant(has_render_state_override, use_null_pixel_shader);
      auto &cls = variant.getClass(shader_class);
      cls.addUse(static_code, dynamic_code);
    }
  }

  ShaderClassInfo &findMatchingComputeClass(const char *name)
  {
    auto ref = eastl::find_if(begin(computeClasses), end(computeClasses), [name](auto &cls) { return name == cls.name; });
    if (ref == end(computeClasses))
    {
      ShaderClassInfo newClass;
      newClass.name = name;
      ref = computeClasses.insert(ref, newClass);
    }
    return *ref;
  }
  void onShaderClassComputeUse(const char *shader_class, uint32_t static_code, uint32_t dynamic_code)
  {
    auto &cls = findMatchingComputeClass(shader_class);
    cls.addUse(static_code, dynamic_code);
  }

  PipelineVariant &findMatchingMeshVariant(bool has_render_state_override, bool use_null_pixel_shader)
  {
    auto &source = use_null_pixel_shader ? nullVariations : variations;
    auto staticRenderStateToCompare = has_render_state_override ? currentVariant.staticRenderState : StaticRenderStateID::Null();
    auto ref = eastl::find_if(begin(source), end(source), [this, staticRenderStateToCompare](auto &compare_to) {
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED == compare_to.topology &&
             InternalInputLayoutID::Null() == compare_to.inputLayout && currentVariant.isWF == compare_to.isWF &&
             currentVariant.frameBufferLayout == compare_to.frameBufferLayout &&
             staticRenderStateToCompare == compare_to.staticRenderState;
    });
    if (ref == end(source))
    {
      PipelineVariant newVariant;
      static_cast<GraphicsVariantInfo &>(newVariant) = currentVariant;
      newVariant.inputLayout = InternalInputLayoutID::Null();
      newVariant.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
      if (!has_render_state_override)
      {
        newVariant.staticRenderState = StaticRenderStateID::Null();
      }
      ref = source.insert(ref, newVariant);
    }
    return *ref;
  }

  PipelineVariant &findMatchingVariant(bool has_render_state_override, bool use_null_pixel_shader)
  {
    auto &source = use_null_pixel_shader ? nullVariations : variations;
    auto staticRenderStateToCompare = has_render_state_override ? currentVariant.staticRenderState : StaticRenderStateID::Null();
    auto ref = eastl::find_if(begin(source), end(source), [this, staticRenderStateToCompare](auto &compare_to) {
      return currentVariant.topology == compare_to.topology && currentVariant.inputLayout == compare_to.inputLayout &&
             currentVariant.isWF == compare_to.isWF && currentVariant.frameBufferLayout == compare_to.frameBufferLayout &&
             staticRenderStateToCompare == compare_to.staticRenderState;
    });
    if (ref == end(source))
    {
      PipelineVariant newVariant;
      static_cast<GraphicsVariantInfo &>(newVariant) = currentVariant;
      if (!has_render_state_override)
      {
        newVariant.staticRenderState = StaticRenderStateID::Null();
      }
      ref = source.insert(ref, newVariant);
    }
    return *ref;
  }

  PipelineVariantWithPixelShaderOverride &findMatchingMeshVariantWithPixelShaderOverride(bool has_render_state_override)
  {
    auto &source = pixelShaderOverrideVariations;
    auto staticRenderStateToCompare = has_render_state_override ? currentVariant.staticRenderState : StaticRenderStateID::Null();
    auto ref = eastl::find_if(begin(source), end(source), [this, staticRenderStateToCompare](auto &compare_to) {
      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED == compare_to.topology &&
             InternalInputLayoutID::Null() == compare_to.inputLayout && currentVariant.isWF == compare_to.isWF &&
             currentVariant.frameBufferLayout == compare_to.frameBufferLayout &&
             staticRenderStateToCompare == compare_to.staticRenderState;
    });
    if (ref == end(source))
    {
      PipelineVariantWithPixelShaderOverride newVariant;
      static_cast<GraphicsVariantInfo &>(newVariant) = currentVariant;
      newVariant.inputLayout = InternalInputLayoutID::Null();
      newVariant.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
      if (!has_render_state_override)
      {
        newVariant.staticRenderState = StaticRenderStateID::Null();
      }
      ref = source.insert(ref, newVariant);
    }
    return *ref;
  }

  PipelineVariantWithPixelShaderOverride &findMatchingVariantWithPixelShaderOverride(bool has_render_state_override)
  {
    auto &source = pixelShaderOverrideVariations;
    auto staticRenderStateToCompare = has_render_state_override ? currentVariant.staticRenderState : StaticRenderStateID::Null();
    auto ref = eastl::find_if(begin(source), end(source), [this, staticRenderStateToCompare](auto &compare_to) {
      return currentVariant.topology == compare_to.topology && currentVariant.inputLayout == compare_to.inputLayout &&
             currentVariant.isWF == compare_to.isWF && currentVariant.frameBufferLayout == compare_to.frameBufferLayout &&
             staticRenderStateToCompare == compare_to.staticRenderState;
    });
    if (ref == end(source))
    {
      PipelineVariantWithPixelShaderOverride newVariant;
      static_cast<GraphicsVariantInfo &>(newVariant) = currentVariant;
      if (!has_render_state_override)
      {
        newVariant.staticRenderState = StaticRenderStateID::Null();
      }
      ref = source.insert(ref, newVariant);
    }
    return *ref;
  }

  template <typename InputLayoutManager, typename StaticRenderStateManager, typename FrameBufferLayoutManager>
  void generateBlk(InputLayoutManager &ilm, StaticRenderStateManager &srsm, FrameBufferLayoutManager &fblm, DataBlock &target)
  {
    if (variations.empty() && computeClasses.empty() && nullVariations.empty() && pixelShaderOverrideVariations.empty())
    {
      return;
    }

    StateRemapTable<StaticRenderStateID> staticRenderStates;
    StateRemapTable<FramebufferLayoutID> frameBufferLayouts;
    StateRemapTable<InternalInputLayoutID> inputLayouts;
    char hashBuffer[sizeof(dxil::HashValue) * 2 + 1];

    auto remapIDs = [&inputLayouts, &staticRenderStates, &frameBufferLayouts](auto &v) {
      if (InternalInputLayoutID::Null() != v.inputLayout)
      {
        v.inputLayout = InternalInputLayoutID::make(inputLayouts.add(v.inputLayout));
      }
      if (StaticRenderStateID::Null() != v.staticRenderState)
      {
        v.staticRenderState = StaticRenderStateID::make(staticRenderStates.add(v.staticRenderState));
      }
      if (FramebufferLayoutID::Null() != v.frameBufferLayout)
      {
        v.frameBufferLayout = FramebufferLayoutID::make(frameBufferLayouts.add(v.frameBufferLayout));
      }
    };

    if (auto computeBlock = target.addNewBlock("compute"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::ComputeVariantEncoder> visitor{*computeBlock};
      visitor.encode(computeClasses, 0);
    }

    if (auto graphicsNullBlock = target.addNewBlock("graphicsNullOverride"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::GraphicsVariantEncoder> visitor{*graphicsNullBlock};
      for (auto &v : nullVariations)
      {
        remapIDs(v);

        visitor.encode(
          v, v.classes,
          [&](auto input_layout_id) {
            dxil::HashValue::calculate(&ilm.getInputLayout(inputLayouts.table[input_layout_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          [&](auto static_render_state_id) {
            dxil::HashValue::calculate(&srsm.getStaticRenderState(staticRenderStates.table[static_render_state_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          [&](auto frambuffer_layout_id) {
            dxil::HashValue::calculate(&fblm.getLayout(frameBufferLayouts.table[frambuffer_layout_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          0);
      }
    }

    if (auto graphicsBlock = target.addNewBlock("graphics"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::GraphicsVariantEncoder> visitor{*graphicsBlock};
      for (auto &v : variations)
      {
        remapIDs(v);

        visitor.encode(
          v, v.classes,
          [&](auto input_layout_id) {
            dxil::HashValue::calculate(&ilm.getInputLayout(inputLayouts.table[input_layout_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          [&](auto static_render_state_id) {
            dxil::HashValue::calculate(&srsm.getStaticRenderState(staticRenderStates.table[static_render_state_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          [&](auto frambuffer_layout_id) {
            dxil::HashValue::calculate(&fblm.getLayout(frameBufferLayouts.table[frambuffer_layout_id.get()]), 1)
              .convertToString(hashBuffer, sizeof(hashBuffer));
            return hashBuffer;
          },
          0);
      }
    }

    if (auto inputLayoutOutBlock = target.addNewBlock("input_layouts"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::InputLayoutEncoder> visitor{*inputLayoutOutBlock};
      for (auto &layout : inputLayouts.table)
      {
        visitor.encode2(ilm.getInputLayout(layout));
      }
    }

    if (auto renderStateOutBlock = target.addNewBlock("render_states"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::RenderStateEncoder> visitor{*renderStateOutBlock};
      for (auto &state : staticRenderStates.table)
      {
        visitor.encode2(srsm.getStaticRenderState(state));
      }
    }

    if (auto framebufferLayoutOutBlock = target.addNewBlock("framebuffer_layouts"))
    {
      pipeline::DataBlockEncodeVisitor<pipeline::FramebufferLayoutEncoder> visitor{*framebufferLayoutOutBlock};
      for (auto &layout : frameBufferLayouts.table)
      {
        visitor.encode2(fblm.getLayout(layout));
      }
    }
  }
};
} // namespace

#if _TARGET_PC_WIN
void PipelineManager::writeBlkCache(FramebufferLayoutManager &fblm, uint32_t group)
{
  auto dump = getDump(group);
  if (!dump || !dump->getDump())
  {
    return;
  }
  if (dump_has_zero_timestamps(dump))
  {
    return;
  }
  DataBlock outBlock;
  if (auto signatureBlock = outBlock.addNewBlock("signature"))
  {
    generate_scripted_shader_dump_signature(dump, getGroupName(group), *signatureBlock);
  }

  PipelineInfoCollector info;
  for (auto &pipeline : graphicsPipelines[group])
  {
    if (!pipeline)
    {
      continue;
    }
    info.beginPipeline(pipeline.get());
    if (!pipeline->isMesh())
    {
      pipeline->visitVariants([this, &info, &pipeline](auto input_layout_id, auto static_render_state_id, auto frame_buffer_layout_id,
                                auto topology, auto wire_frame) {
        auto &vs = pipeline->vertexShaderModule();
        auto &ps = pipeline->pixelShaderModule();
        auto &staticRenderState = this->getStaticRenderState(static_render_state_id);
        info.beginVariant(input_layout_id, static_render_state_id, frame_buffer_layout_id, topology, wire_frame);
        this->visitShaderClassPassesForGraphicsPipeline(vs, ps, staticRenderState,
          [&info](auto &shader_class, auto static_code, auto dynamic_code, bool use_static_state_override, bool use_null_pixel_shader,
            bool use_with_pixel_shader_override, bool is_pixel_shader_override) {
            info.onShaderClassGraphicsUse(static_cast<const char *>(shader_class.name), static_code, dynamic_code,
              use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override, is_pixel_shader_override);
          });
      });
    }
    else
    {
      pipeline->visitVariants(
        [this, &info, &pipeline](auto, auto static_render_state_id, auto frame_buffer_layout_id, auto, auto wire_frame) {
          auto &vs = pipeline->vertexShaderModule();
          auto &ps = pipeline->pixelShaderModule();
          auto &staticRenderState = this->getStaticRenderState(static_render_state_id);
          info.beginMeshVariant(static_render_state_id, frame_buffer_layout_id, wire_frame);
          this->visitShaderClassPassesForGraphicsPipeline(vs, ps, staticRenderState,
            [&info](auto &shader_class, auto static_code, auto dynamic_code, bool use_static_state_override,
              bool use_null_pixel_shader, bool use_with_pixel_shader_override, bool is_pixel_shader_override) {
              info.onShaderClassMeshUse(static_cast<const char *>(shader_class.name), static_code, dynamic_code,
                use_static_state_override, use_null_pixel_shader, use_with_pixel_shader_override, is_pixel_shader_override);
            });
        });
    }
  }

  for (auto &pipeline : computePipelines[group])
  {
    if (!pipeline)
    {
      continue;
    }
    visitShaderClassPassesForComputePipeline(pipeline->getModule(),
      [&info](auto &shader_class, auto static_code, auto dynamic_code, bool, bool, bool, bool) {
        info.onShaderClassComputeUse(static_cast<const char *>(shader_class.name), static_code, dynamic_code);
      });
  }
  info.generateBlk(*this, *this, fblm, outBlock);

  outBlock.saveToTextFile("cache/dx12_cache_new.blk");

  if (get_device().netManager)
    get_device().netManager->sendPsoCacheBlkSync(outBlock);
}
#endif

void PipelineStageStateBase::setSRVTexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds,
  D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  if ((tRegisters[unit].image == image) && (tRegisters[unit].imageView == view_state) && (tRegisters[unit].view == view) &&
      isConstDepthStencil.test(unit) == as_const_ds)
    return;

  isConstDepthStencil.set(unit, as_const_ds);

#if D3D_HAS_RAY_TRACING
  tRegisters[unit].as = nullptr;
#endif
  tRegisters[unit].buffer = BufferReference{};

  tRegisters[unit].image = image;
  tRegisters[unit].imageView = view_state;
  tRegisters[unit].view = view;

  tRegisterValidMask &= ~(1 << unit);
  if (image && image->hasTrackedState())
  {
    tRegisterStateDirtyMask |= 1u << unit;
  }

  if (image)
    image->dbgValidateImageViewStateCompatibility(view_state);
}

void PipelineStageStateBase::setSampler(uint32_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler)
{
  if (sRegisters[unit] == sampler)
    return;

  sRegisterValidMask &= ~(1 << unit);
  sRegisters[unit] = sampler;
}

void PipelineStageStateBase::setUAVTexture(uint32_t unit, Image *image, ImageViewState view_state, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  if ((uRegisters[unit].image == image) && (uRegisters[unit].imageView == view_state) && (uRegisters[unit].view == view))
    return;

  uRegisterValidMask &= ~(1 << unit);
  uRegisterStateDirtyMask |= 1u << unit;

  uRegisters[unit].image = image;
  uRegisters[unit].imageView = view_state;
  uRegisters[unit].view = view;

  uRegisters[unit].buffer = BufferReference{};
}

bool PipelineStageStateBase::setSRVBuffer(uint32_t unit, BufferResourceReferenceAndShaderResourceView buffer)
{
  if (tRegisters[unit].view == buffer)
    return false;

  tRegisterValidMask &= ~(1 << unit);
  tRegisterStateDirtyMask |= 1u << unit;
  // have to ensure that samplers have to be looked up again
  sRegisterValidMask = 0;

#if D3D_HAS_RAY_TRACING
  tRegisters[unit].as = nullptr;
#endif

  tRegisters[unit].buffer = buffer;

  tRegisters[unit].image = nullptr;
  tRegisters[unit].imageView = ImageViewState();
  tRegisters[unit].view = buffer.srv;

  return true;
}

bool PipelineStageStateBase::setUAVBuffer(uint32_t unit, BufferResourceReferenceAndUnorderedResourceView buffer)
{
  if (uRegisters[unit].view == buffer)
    return false;

  uRegisterValidMask &= ~(1 << unit);
  uRegisterStateDirtyMask |= 1u << unit;

  uRegisters[unit].buffer = buffer;

  uRegisters[unit].image = nullptr;
  uRegisters[unit].imageView = ImageViewState();
  uRegisters[unit].view = buffer.uav;

  return true;
}

void PipelineStageStateBase::setConstBuffer(uint32_t unit, const ConstBufferSetupInformation &info)
{
  if (bRegisters[unit] == info.buffer)
    return;

  bRegisterValidMask &= ~(1 << unit);
  bRegisterStateDirtyMask |= 1u << unit;

  bRegisters[unit].BufferLocation = info.buffer.gpuPointer;
  bRegisters[unit].SizeInBytes = info.buffer.size;
  bRegisterBuffers[unit] = info;
}

#if D3D_HAS_RAY_TRACING
void PipelineStageStateBase::setRaytraceAccelerationStructureAtT(uint32_t unit, RaytraceAccelerationStructure *as)
{
  if (tRegisters[unit].as == as)
    return;

  tRegisterValidMask &= ~(1 << unit);
  tRegisterStateDirtyMask |= 1u << unit;
  // have to ensure that samplers have to be looked up again
  sRegisterValidMask = 0;

  tRegisters[unit].as = as;

  tRegisters[unit].buffer = BufferReference{};

  tRegisters[unit].image = nullptr;
  tRegisters[unit].imageView = ImageViewState();
  tRegisters[unit].view.ptr = 0;

  if (as)
  {
    tRegisters[unit].view = as->descriptor;
  }
}
#endif

void PipelineStageStateBase::setSRVNull(uint32_t unit)
{
  auto &slot = tRegisters[unit];
  bool hasChanged = D3D12_CPU_DESCRIPTOR_HANDLE{0} != slot.view;
#if D3D_HAS_RAY_TRACING
  hasChanged = hasChanged || nullptr != slot.as;
#endif
  if (!hasChanged)
    return;

  tRegisterValidMask &= ~(1 << unit);
  tRegisterStateDirtyMask |= 1u << unit;
  sRegisterValidMask = 0;

#if D3D_HAS_RAY_TRACING
  slot.as = nullptr;
#endif

  slot.buffer = BufferReference{};

  slot.image = nullptr;
  slot.imageView = ImageViewState{};

  slot.view = D3D12_CPU_DESCRIPTOR_HANDLE{0};
}

void PipelineStageStateBase::setUAVNull(uint32_t unit)
{
  auto &slot = uRegisters[unit];
  if (D3D12_CPU_DESCRIPTOR_HANDLE{0} == slot.view)
    return;

  uRegisterValidMask &= ~(1 << unit);
  uRegisterStateDirtyMask |= 1u << unit;

  slot.buffer = BufferReference{};

  slot.image = nullptr;
  slot.imageView = ImageViewState{};
  slot.view = D3D12_CPU_DESCRIPTOR_HANDLE{0};
}

namespace
{
template <typename T, typename U, typename V>
uint32_t maching_object_mask(const T &obj, U &container, V extractor)
{
  uint32_t mask = 0;
  for (uint32_t i = 0; i < countof(container); ++i)
  {
    mask |= ((extractor(container[i]) == obj) ? 1u : 0u) << i;
  }
  return mask;
}
} // namespace

void PipelineStageStateBase::dirtyBufferState(BufferGlobalId id)
{
  bRegisterStateDirtyMask |= maching_object_mask(id, bRegisterBuffers, [](auto &ref) { return ref.buffer.resourceId; });
  tRegisterStateDirtyMask |= maching_object_mask(id, tRegisters, [](auto &ref) { return ref.buffer.resourceId; });
  uRegisterStateDirtyMask |= maching_object_mask(id, uRegisters, [](auto &ref) { return ref.buffer.resourceId; });
}

void PipelineStageStateBase::dirtyTextureState(Image *texture)
{
  if (texture && !texture->hasTrackedState())
  {
    return;
  }
  tRegisterStateDirtyMask |= maching_object_mask(texture, tRegisters, [](auto &ref) { return ref.image; });
  uRegisterStateDirtyMask |= maching_object_mask(texture, uRegisters, [](auto &ref) { return ref.image; });
}

void PipelineStageStateBase::flushResourceStates(uint32_t b_register_mask, uint32_t t_register_mask, uint32_t u_register_mask,
  uint32_t stage, ResourceUsageManagerWithHistory &res_usage, BarrierBatcher &barrier_batcher, SplitTransitionTracker &split_tracker)
{
  const auto bUpdateMask = LsbVisitor{bRegisterStateDirtyMask & b_register_mask};
  bRegisterStateDirtyMask &= ~b_register_mask;

  const auto tUpdateMask = LsbVisitor{tRegisterStateDirtyMask & t_register_mask};
  tRegisterStateDirtyMask &= ~t_register_mask;

  const auto uUpdateMask = LsbVisitor{uRegisterStateDirtyMask & u_register_mask};
  uRegisterStateDirtyMask &= ~u_register_mask;

  for (auto i : bUpdateMask)
  {
    res_usage.useBufferAsCBV(barrier_batcher, stage, bRegisterBuffers[i].buffer);
  }

  for (auto i : tUpdateMask)
  {
    if (tRegisters[i].image)
    {
      tRegisters[i].image->dbgValidateImageViewStateCompatibility(tRegisters[i].imageView);
      res_usage.useTextureAsSRV(barrier_batcher, split_tracker, stage, tRegisters[i].image, tRegisters[i].imageView,
        isConstDepthStencil.test(i));
    }
    else if (tRegisters[i].buffer)
    {
      res_usage.useBufferAsSRV(barrier_batcher, stage, tRegisters[i].buffer);
    }
    // RT structures do not need any state tracking
  }

  for (auto i : uUpdateMask)
  {
    if (uRegisters[i].image)
    {
      res_usage.useTextureAsUAV(barrier_batcher, split_tracker, stage, uRegisters[i].image, uRegisters[i].imageView);
    }
    else if (uRegisters[i].buffer)
    {
      res_usage.useBufferAsUAV(barrier_batcher, stage, uRegisters[i].buffer);
    }
  }
}

#if _TARGET_PC_WIN
void ShaderDeviceRequirementChecker::init(ID3D12Device *device)
{
  auto op0 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS>(device);
  auto op1 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS1>(device);
  // auto op2 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS2>(device);
  auto op3 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS3>(device);
  auto op4 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS4>(device);
  auto op5 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS5>(device);
  auto op6 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS6>(device);
  auto op7 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS7>(device);
  // auto op8 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS8>(device);
  auto op9 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS9>(device);
  // auto op10 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS10>(device);
  auto op11 = check_feature_support<D3D12_FEATURE_DATA_D3D12_OPTIONS11>(device);
  auto sm = check_feature_support<D3D12_FEATURE_DATA_SHADER_MODEL>(device, shader_model_to_dx(d3d::smMax));

  supportedRequirement.shaderModel = static_cast<uint8_t>(sm.HighestShaderModel);
  uint64_t combinedFlags = 0;
  combinedFlags |= D3D_SHADER_REQUIRES_DOUBLES;
  combinedFlags |= D3D_SHADER_REQUIRES_EARLY_DEPTH_STENCIL;
  combinedFlags |= D3D_SHADER_REQUIRES_UAVS_AT_EVERY_STAGE;
  combinedFlags |= D3D_SHADER_REQUIRES_64_UAVS;

  if (op0.MinPrecisionSupport)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_MINIMUM_PRECISION;
  }

  if (op0.DoublePrecisionFloatShaderOps)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_11_1_DOUBLE_EXTENSIONS;
  }
  // may be optional, documentation uses DX11 names in DX12 struct that has no equivalent
  combinedFlags |= D3D_SHADER_REQUIRES_11_1_SHADER_EXTENSIONS;
  combinedFlags |= D3D_SHADER_REQUIRES_LEVEL_9_COMPARISON_FILTERING;

  if (D3D12_TILED_RESOURCES_TIER_1 <= op0.TiledResourcesTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_TILED_RESOURCES;
  }

  if (op0.PSSpecifiedStencilRefSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_STENCIL_REF;
  }

  if (D3D12_CONSERVATIVE_RASTERIZATION_TIER_3 <= op0.ConservativeRasterizationTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_INNER_COVERAGE;
  }

  if (op0.TypedUAVLoadAdditionalFormats)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_TYPED_UAV_LOAD_ADDITIONAL_FORMATS;
  }

  if (op0.ROVsSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_ROVS;
  }

  if (op0.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_VIEWPORT_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER_FEEDING_RASTERIZER;
  }

  if (op1.WaveOps)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_WAVE_OPS;
  }

  if (op1.Int64ShaderOps)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_INT64_OPS;
  }

  if (D3D12_VIEW_INSTANCING_TIER_1 <= op3.ViewInstancingTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_VIEW_ID;
  }

  if (op3.BarycentricsSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_BARYCENTRICS;
  }

  if (op4.Native16BitShaderOpsSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_NATIVE_16BIT_OPS;
  }

  if (D3D12_VARIABLE_SHADING_RATE_TIER_2 <= op6.VariableShadingRateTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_SHADING_RATE;
  }

  if (D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_RAYTRACING_TIER_1_1;
  }

  if (D3D12_SAMPLER_FEEDBACK_TIER_0_9 <= op7.SamplerFeedbackTier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_SAMPLER_FEEDBACK;
  }

  if (op9.AtomicInt64OnTypedResourceSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_TYPED_RESOURCE;
  }

  if (op9.AtomicInt64OnGroupSharedSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_GROUP_SHARED;
  }

  if (op9.DerivativesInMeshAndAmplificationShadersSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_DERIVATIVES_IN_MESH_AND_AMPLIFICATION_SHADERS;
  }

  if ((D3D12_RESOURCE_BINDING_TIER_3 <= op0.ResourceBindingTier) && (shader_model_from_dx(sm.HighestShaderModel) >= 6.6_sm))
  {
    combinedFlags |= D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING;
    combinedFlags |= D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING;
  }

  if (D3D12_WAVE_MMA_TIER_1_0 <= op9.WaveMMATier)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_WAVE_MMA;
  }

  if (op11.AtomicInt64OnDescriptorHeapResourceSupported)
  {
    combinedFlags |= D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_DESCRIPTOR_HEAP_RESOURCE;
  }

  supportedRequirement.shaderFeatureFlagsLow = static_cast<uint32_t>(combinedFlags);
  supportedRequirement.shaderFeatureFlagsHigh = static_cast<uint32_t>(combinedFlags >> 32);

  to_debuglog(supportedRequirement);
}
#endif
