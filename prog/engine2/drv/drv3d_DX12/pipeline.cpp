#include "device.h"
#include "EASTL/functional.h"

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

#if _TARGET_SCARLETT
static bool has_acceleration_structure(const StageShaderModule &shader)
{
  for (int i = 0; i < dxil::MAX_T_REGISTERS; ++i)
    if (static_cast<D3D_SHADER_INPUT_TYPE>(shader.header.tRegisterTypes[i] & 0xF) == 12 /*D3D_SIT_RTACCELERATIONSTRUCTURE*/)
      return true;
  return false;
}
#endif

eastl::string PipelineVariant::getVariantName(BasePipeline &base, const InputLayout & /*input_layout*/,
  const RenderStateSystem::StaticState & /*static_state*/, const FramebufferLayout & /*fb_layout*/)
{
  eastl::string name;
  if (!base.vsModule.debugName.empty())
  {
    name.sprintf("vs: \"%s", base.vsModule.debugName.c_str());
    if (!name.empty() && name[name.size() - 1] == '\n')
      name.pop_back();
    name.append("\"");
  }
  if (!base.psModule.debugName.empty())
  {
    if (!name.empty())
      name += ", ";
    name.append_sprintf("ps: \"%s", base.psModule.debugName.c_str());
    if (!name.empty() && name[name.size() - 1] == '\n')
      name.pop_back();
    name.append("\"");
  }
  if (!name.empty())
  {
    // TODO maybe some more properties here: if one or more pipeline properties prove to be needed in the
    // profiling then include their names here
  }

  return name;
}

bool PipelineVariant::isVariantNameEmpty(BasePipeline &base, const InputLayout & /*input_layout*/,
  const RenderStateSystem::StaticState & /*static_state*/, const FramebufferLayout & /*fb_layout*/)
{
  return base.vsModule.debugName.empty() && base.psModule.debugName.empty();
}

eastl::string PipelineVariant::getVariantName(BasePipeline &base, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout)
{
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  eastl::string name;
  if (!base.vsModule.debugName.empty())
  {
    name.sprintf("vs: \"%s", base.vsModule.debugName.c_str());
    if (!name.empty() && name[name.size() - 1] == '\n')
      name.pop_back();
    name.append("\"");
  }
  if (!base.psModule.debugName.empty())
  {
    if (!name.empty())
      name += ", ";
    name.append_sprintf("ps: \"%s", base.psModule.debugName.c_str());
    if (!name.empty() && name[name.size() - 1] == '\n')
      name.pop_back();
    name.append("\"");
  }
  if (!name.empty())
  {
    // TODO maybe some more properties here: if one or more pipeline properties prove to be needed in the
    // profiling then include their names here
  }

  return name;
}

bool PipelineVariant::isVariantNameEmpty(BasePipeline &base, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout)
{
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  return base.vsModule.debugName.empty() && base.psModule.debugName.empty();
}

bool PipelineVariant::calculateColorWriteMask(const BasePipeline &base, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, uint32_t &color_write_mask) const
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
    spread_color_chanel_mask_to_render_target_color_channel_mask(base.psModule.header.inOutSemanticMask);
  // The mask from the framebuffers active slots and thier formats is masked off by the written
  // slots by the shader implicitly.
  uint32_t fbColorWriteMask = fb_layout.getColorWriteMask();
  color_write_mask = fbColorWriteMask & basicShaderTargetMask;
  uint32_t missingShaderOutputMask =
    static_state.calculateMissingShaderOutputMask(color_write_mask, base.psModule.header.inOutSemanticMask);
  if (missingShaderOutputMask)
  {
    logerr("DX12: Pipeline (VS %s PS %s) does not write all enabled color target components. "
           "Shader output mask %08X, frame buffer color mask implied from shader %08X, static "
           "state color write mask %08X, color write mask implied from frame buffer %08X, "
           "computed color write mask %08X, missing outputs mask %08X",
      base.vsModule.debugName, base.psModule.debugName, base.psModule.header.inOutSemanticMask, basicShaderTargetMask,
      static_state.colorWriteMask, fbColorWriteMask, static_state.adjustColorTargetMask(color_write_mask), missingShaderOutputMask);
    return false;
  }
  return true;
}

bool PipelineVariant::generateOutputMergerDescriptions(const BasePipeline &base, const RenderStateSystem::StaticState &static_state,
  const FramebufferLayout &fb_layout, GraphicsPipelineCreateInfoData &target) const
{
  uint32_t fbFinalColorWriteMask = 0;
  auto isOK = calculateColorWriteMask(base, static_state, fb_layout, fbFinalColorWriteMask);

  if (fb_layout.hasDepth)
  {
    target.append<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>(fb_layout.depthStencilFormat.asDxGiFormat());
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

  target.append<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>(static_state.getBlendDesc(fbFinalColorWriteMask));

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
    target.append<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING>(static_state.getViewInstancingDesc());
  }
  return true;
}

bool PipelineVariant::create(ID3D12Device2 *device, BasePipeline &base, PipelineCache &pipe_cache, const InputLayout &input_layout,
  bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
  D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name)
{
  bool succeeded = true;
#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  AutoFuncProf funcProfiler(!isVariantNameEmpty(base, input_layout, static_state, fb_layout)
                              ? (profilerMsg.sprintf("PipelineVariant (%s)::create: took %%dus",
                                   getVariantName(base, input_layout, static_state, fb_layout).c_str()),
                                  profilerMsg.c_str())
                              : "PipelineVariant::create: took %dus");
#endif
  TIME_PROFILE_DEV(PipelineVariant_create);
#if _TARGET_PC_WIN
  if (!base.signature.signature)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    // turns off reporting of the profile, we don't want to know how long it took to _not_
    // load a pipeline
    funcProfiler.fmt = nullptr;
#endif
    debug("DX12: Rootsignature was null, skipping creating graphics pipeline");
    return true;
  }

  GraphicsPipelineCreateInfoData gpci;
  uint32_t inputCount = 0;

  bool outputOK = generateOutputMergerDescriptions(base, static_state, fb_layout, gpci);
  bool inputOK = generateInputLayoutDescription(input_layout, gpci, inputCount);
  bool rasterOK = generateRasterDescription(static_state, is_wire_frame, gpci);

  succeeded = outputOK && inputOK && rasterOK;

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(base.signature.signature.Get());

  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_VS, CD3DX12_SHADER_BYTECODE>(base.vsModule.byteCode.get(), base.vsModule.byteCodeSize);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(base.psModule.byteCode.get(), base.psModule.byteCodeSize);
  if (base.vsModule.geometryShader)
  {
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_GS, CD3DX12_SHADER_BYTECODE>(base.vsModule.geometryShader->byteCode.get(),
      base.vsModule.geometryShader->byteCodeSize);
  }
  if (base.vsModule.hullShader)
  {
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_HS, CD3DX12_SHADER_BYTECODE>(base.vsModule.hullShader->byteCode.get(),
      base.vsModule.hullShader->byteCodeSize);
  }
  if (base.vsModule.domainShader)
  {
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_DS, CD3DX12_SHADER_BYTECODE>(base.vsModule.domainShader->byteCode.get(),
      base.vsModule.domainShader->byteCodeSize);
  }

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>(top);

  D3D12_CACHED_PIPELINE_STATE &cacheTarget = gpci.append<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>();

  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
  pipeline = pipe_cache.loadGraphicsPipelineVariant(base.cacheId, top, input_layout, is_wire_frame, static_state, fb_layout, gpciDesc,
    cacheTarget);

#else
  uint32_t fbFinalColorWriteMask = 0;
  succeeded = calculateColorWriteMask(base, static_state, fb_layout, fbFinalColorWriteMask);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {0};
  desc.SampleMask = 0x1;
  desc.DSVFormat = fb_layout.depthStencilFormat.asDxGiFormat();

  uint32_t mask = fb_layout.colorTargetMask;
  if (mask)
  {
    for (uint32_t i = 0; i < 8; ++i, mask >>= 1)
    {
      if (mask & 1)
      {
        desc.RTVFormats[i] = fb_layout.colorFormats[i].asDxGiFormat();
        desc.NumRenderTargets = i + 1;
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

  desc.VS.pShaderBytecode = base.vsModule.byteCode.get();
  desc.VS.BytecodeLength = base.vsModule.byteCodeSize;
  desc.PS.pShaderBytecode = base.psModule.byteCode.get();
  desc.PS.BytecodeLength = base.psModule.byteCodeSize;

  if (base.vsModule.geometryShader)
  {
    desc.GS.pShaderBytecode = base.vsModule.geometryShader->byteCode.get();
    desc.GS.BytecodeLength = base.vsModule.geometryShader->byteCodeSize;
  }
  if (base.vsModule.hullShader)
  {
    desc.HS.pShaderBytecode = base.vsModule.hullShader->byteCode.get();
    desc.HS.BytecodeLength = base.vsModule.hullShader->byteCodeSize;
  }
  if (base.vsModule.domainShader)
  {
    desc.DS.pShaderBytecode = base.vsModule.domainShader->byteCode.get();
    desc.DS.BytecodeLength = base.vsModule.domainShader->byteCodeSize;
  }

#endif
  if (!pipeline)
  {
#if _TARGET_PC_WIN
    auto result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    if (FAILED(result) && cacheTarget.CachedBlobSizeInBytes > 0)
    {
      debug("DX12: CreatePipelineState failed with attached cache blob, retrying without");
      cacheTarget.pCachedBlob = nullptr;
      cacheTarget.CachedBlobSizeInBytes = 0;
      result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    }
#else
    G_UNUSED(pipe_cache);
    auto result = DX12_CHECK_RESULT(xbox_create_graphics_pipeline(device, desc, !static_state.enableDepthBounds, pipeline));
#endif
    // Special handling as on PC we may have to retry if cache blob reload failed
    if (FAILED(drv3d_dx12::dx12_check_result(result, "CreatePipelineState", __FILE__, __LINE__)))
    {
      if (is_recoverable_error(result))
      {
        String fb_layout_info;
        fb_layout_info.printf(64, "hasDepth %u", fb_layout.hasDepth);
        if (fb_layout.hasDepth)
        {
          fb_layout_info.aprintf(128, " depthStencilFormat %s", fb_layout.depthStencilFormat.getNameString());
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
            }
            else
              fb_layout_info.aprintf(128, " DXGI_FORMAT_UNKNOWN");
          }
        }

        String input_layout_info;
        input_layout_info.printf(64, "inputCount %u, mask %08X", inputCount, input_layout.vertexAttributeSet.locationMask);
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
        auto mask = base.vsModule.header.inOutSemanticMask;
        if (mask)
        {
          consumedInputLayout.printf(128, "mask %08X", base.vsModule.header.inOutSemanticMask);
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
            logerr("CreatePipelineState failed for VS %s and PS %s, state was %s, fb_layout was { "
                   "%s }, input_layout was { %s }, vertex inputs were { %s }, il/vi mask xor %08X",
              base.vsModule.debugName.c_str(), base.psModule.debugName.c_str(), static_state.toString(), fb_layout_info,
              input_layout_info, consumedInputLayout,
              input_layout.vertexAttributeSet.locationMask ^ base.vsModule.header.inOutSemanticMask);
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
  if (pipeline && give_name)
  {
    auto debugName = getVariantName(base, input_layout, static_state, fb_layout);
    if (!debugName.empty())
    {
      eastl::vector<wchar_t> nameBuf;
      nameBuf.resize(debugName.length() + 1);
      DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(debugName.data(), nameBuf.data(), nameBuf.size()));
    }
  }
#else
  G_UNUSED(give_name);
#endif
  return succeeded;
}

#if !_TARGET_XBOXONE
bool PipelineVariant::createMesh(ID3D12Device2 *device, BasePipeline &base, PipelineCache &pipe_cache, bool is_wire_frame,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, RecoverablePipelineCompileBehavior on_error,
  bool give_name)
{
  bool succeeded = true;
#if DX12_REPORT_PIPELINE_CREATE_TIMING
  eastl::string profilerMsg;
  AutoFuncProf funcProfiler(
    !isVariantNameEmpty(base, static_state, fb_layout)
      ? (profilerMsg.sprintf("PipelineVariant (%s)::createMesh: took %%dus", getVariantName(base, static_state, fb_layout).c_str()),
          profilerMsg.c_str())
      : "PipelineVariant::createMesh: took %dus");
#endif
  TIME_PROFILE_DEV(PipelineVariant_create);

  if (!base.signature.signature)
  {
#if DX12_REPORT_PIPELINE_CREATE_TIMING
    // turns off reporting of the profile, we don't want to know how long it took to _not_
    // load a pipeline
    funcProfiler.fmt = nullptr;
#endif
    debug("DX12: Rootsignature was null, skipping creating graphics pipeline");
    return true;
  }

  GraphicsPipelineCreateInfoData gpci;

  bool outputOK = generateOutputMergerDescriptions(base, static_state, fb_layout, gpci);
  bool rasterOK = generateRasterDescription(static_state, is_wire_frame, gpci);

  succeeded = outputOK && rasterOK;

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(base.signature.signature.Get());

  // vsModule stores mesh shader
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_MS, CD3DX12_SHADER_BYTECODE>(base.vsModule.byteCode.get(), base.vsModule.byteCodeSize);
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(base.psModule.byteCode.get(), base.psModule.byteCodeSize);
  // vsModule.geometryShader stores amplification shader
  if (base.vsModule.geometryShader)
  {
    gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_AS, CD3DX12_SHADER_BYTECODE>(base.vsModule.geometryShader->byteCode.get(),
      base.vsModule.geometryShader->byteCodeSize);
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
      debug("DX12: CreatePipelineState failed with attached cache blob, retrying without");
      cacheTarget.pCachedBlob = nullptr;
      cacheTarget.CachedBlobSizeInBytes = 0;
      result = device->CreatePipelineState(&gpciDesc, COM_ARGS(&pipeline));
    }
#endif
    // Special handling as on PC we may have to retry if cache blob reload failed
    if (FAILED(drv3d_dx12::dx12_check_result(result, "CreatePipelineState", __FILE__, __LINE__)))
    {
      if (is_recoverable_error(result))
      {
        String fb_layout_info;
        fb_layout_info.printf(64, "hasDepth %u", fb_layout.hasDepth);
        if (fb_layout.hasDepth)
        {
          fb_layout_info.aprintf(128, " depthStencilFormat %s", fb_layout.depthStencilFormat.getNameString());
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
            }
            else
              fb_layout_info.aprintf(128, " DXGI_FORMAT_UNKNOWN");
          }
        }

        switch (on_error)
        {
          // Returning succeeded for failed shader factoring because of bad input parameters? (This comment is only for review, to be
          // removed)
          case RecoverablePipelineCompileBehavior::FATAL_ERROR: return true; break;
          case RecoverablePipelineCompileBehavior::ASSERT_FAIL:
          case RecoverablePipelineCompileBehavior::REPORT_ERROR:
            logerr("CreatePipelineState failed for MS %s and PS %s, state was %s, fb_layout was { %s }",
              base.vsModule.debugName.c_str(), base.psModule.debugName.c_str(), static_state.toString(), fb_layout_info);
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
  if (pipeline && give_name)
  {
    auto debugName = getVariantName(base, static_state, fb_layout);
    if (!debugName.empty())
    {
      eastl::vector<wchar_t> nameBuf;
      nameBuf.resize(debugName.length() + 1);
      DX12_SET_DEBUG_OBJ_NAME(pipeline, lazyToWchar(debugName.data(), nameBuf.data(), nameBuf.size()));
    }
  }
#else
  G_UNUSED(give_name);
#endif
  return succeeded;
}
#endif


bool PipelineVariant::load(ID3D12Device2 *device, BasePipeline &base, PipelineCache &pipe_cache, const InputLayout &input_layout,
  bool is_wire_frame, const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout,
  D3D12_PRIMITIVE_TOPOLOGY_TYPE top, RecoverablePipelineCompileBehavior on_error, bool give_name)
{
  if (pipeline)
    return true;
  dynamicMask = static_state.getDynamicStateMask();
#if !_TARGET_XBOXONE
  G_ASSERT(!base.isMesh());
#endif
  return create(device, base, pipe_cache, input_layout, is_wire_frame, static_state, fb_layout, top, on_error, give_name);
}

bool PipelineVariant::loadMesh(ID3D12Device2 *device, BasePipeline &base, PipelineCache &pipe_cache, bool is_wire_frame,
  const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, RecoverablePipelineCompileBehavior on_error,
  bool give_name)
{
  if (pipeline)
    return true;
  dynamicMask = static_state.getDynamicStateMask();
#if !_TARGET_XBOXONE
  G_ASSERT(base.isMesh());
  return createMesh(device, base, pipe_cache, is_wire_frame, static_state, fb_layout, on_error, give_name);
#else
  G_UNUSED(device);
  G_UNUSED(base);
  G_UNUSED(pipe_cache);
  G_UNUSED(is_wire_frame);
  G_UNUSED(static_state);
  G_UNUSED(fb_layout);
  G_UNUSED(on_error);
  G_UNUSED(give_name);
  return false;
#endif
}

void PipelineManager::init(const SetupParameters &params)
{
  D3D12SerializeRootSignature = params.serializeRootSignature;
  signatureVersion = params.rootSignatureVersion;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  rootSignaturesUsesCBVDescriptorRanges = params.rootSignaturesUsesCBVDescriptorRanges;
#endif

  createBlitSignature(params.device);

  params.pipelineCache->enumerateInputLayouts([this](auto &layout) { inputLayouts.push_back(layout); });

  params.pipelineCache->enumerateGraphicsSignatures([this, device = params.device](const GraphicsPipelineSignature::Definition &def,
                                                      const eastl::vector<uint8_t> &blob) //
    {
      auto newSig = eastl::make_unique<GraphicsPipelineSignature>();
      newSig->def = def;
      newSig->deriveComboMasks();
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      graphicsSignatures.push_back(eastl::move(newSig));
    });

#if !_TARGET_XBOXONE
  params.pipelineCache->enumerateGraphicsMeshSignatures(
    [this, device = params.device](const GraphicsPipelineSignature::Definition &def, const eastl::vector<uint8_t> &blob) {
      auto newSig = eastl::make_unique<GraphicsPipelineSignature>();
      newSig->def = def;
      newSig->deriveComboMasks();
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      graphicsMeshSignatures.push_back(eastl::move(newSig));
    });
#endif

  params.pipelineCache->enumrateComputeSignatures([this, device = params.device](const ComputePipelineSignature::Definition &def,
                                                    const eastl::vector<uint8_t> &blob) //
    {
      auto newSig = eastl::make_unique<ComputePipelineSignature>();
      newSig->def = def;
      DX12_CHECK_OK(device->CreateRootSignature(0, blob.data(), blob.size(), COM_ARGS(&newSig->signature)));
      computeSignatures.push_back(eastl::move(newSig));
    });
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
    fatal("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
  }
  DX12_CHECK_RESULT(
    device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&blitSignature)));
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
  {
    return nullptr;
  }
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

  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>(blitSignature.Get());
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_VS, CD3DX12_SHADER_BYTECODE>(blit_vertex_shader, sizeof(blit_vertex_shader));
  gpci.emplace<CD3DX12_PIPELINE_STATE_STREAM_PS, CD3DX12_SHADER_BYTECODE>(blit_pixel_shader, sizeof(blit_pixel_shader));
  gpci.append<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

  BlitPipeline newBlitPipe{};
  newBlitPipe.outFormat = out_fmt;

  D3D12_PIPELINE_STATE_STREAM_DESC gpciDesc = {gpci.rootSize(), gpci.root()};
  auto result = DX12_CHECK_RESULT(device->CreatePipelineState(&gpciDesc, COM_ARGS(&newBlitPipe.pipeline)));
  // pipe_cache.addVariant(top, staticState, renderPassClass, pipelines[top - 1].Get());

#else

  BlitPipeline newBlitPipe{};
  newBlitPipe.outFormat = out_fmt;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {0};
  desc.SampleMask = 0x1;
  desc.pRootSignature = blitSignature.Get();
  desc.VS.pShaderBytecode = blit_vertex_shader;
  desc.VS.BytecodeLength = sizeof(blit_vertex_shader);
  desc.PS.pShaderBytecode = blit_pixel_shader;
  desc.PS.BytecodeLength = sizeof(blit_pixel_shader);
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

  // device->CreateGraphicsPipelineState(&desc, COM_ARGS(&newBlitPipe.pipeline));

  auto result = DX12_CHECK_RESULT(xbox_create_graphics_pipeline(device, desc, true, newBlitPipe.pipeline));

#endif

  if (FAILED(result))
  {
    fatal("Failed to create blit pipeline");
    return nullptr;
  }

  if (give_name)
  {
    DX12_SET_DEBUG_OBJ_NAME(newBlitPipe.pipeline, L"BlitPipline");
  }
  return blitPipelines.emplace_back(eastl::move(newBlitPipe)).pipeline.Get();
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
    desc.Flags |= ROOT_SIGNATURE_FLAG_INTERNAL_XDXR;
#endif
  }
  void noPixelShaderResources() { desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; }
  void setVisibilityPixelShader() { currentVisibility = D3D12_SHADER_VISIBILITY_PIXEL; }
  void addRootParameterConstantExplicit(uint32_t space, uint32_t index, uint32_t dwords, D3D12_SHADER_VISIBILITY vis)
  {
    G_ASSERT(desc.NumParameters < array_size(params));

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
      G_ASSERT(desc.NumParameters < array_size(params));
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
      G_ASSERT(desc.NumParameters < array_size(params));

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
    G_ASSERT(desc.NumParameters < array_size(params));
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
      G_ASSERT(desc.NumParameters < array_size(params));
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
    G_ASSERT(desc.NumParameters < array_size(params));
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
      G_ASSERT(desc.NumParameters < array_size(params));
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
    G_ASSERT(desc.NumParameters < array_size(params));
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
        fatal("DX12: Trying to update pipeline data for unexpected shader visibility!");
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
        fatal("DX12: Trying to update pipeline data for unexpected shader visibility!");
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
    debug("DX12: Generating graphics signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;
    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      fatal("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
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
    debug("DX12: Generating graphics mesh signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;
    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      fatal("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
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
      void hasAccelerationStructure()
      {
#if _TARGET_SCARLETT
        desc.Flags |= ROOT_SIGNATURE_FLAG_INTERNAL_XDXR;
#endif
      }
      void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
      {
        G_ASSERT(desc.NumParameters < array_size(params));

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
          G_ASSERT(desc.NumParameters < array_size(params));
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
          G_ASSERT(desc.NumParameters < array_size(params));

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
        G_ASSERT(desc.NumParameters < array_size(params));
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
        G_ASSERT(desc.NumParameters < array_size(params));
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
        G_ASSERT(desc.NumParameters < array_size(params));
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
        G_ASSERT(desc.NumParameters < array_size(params));
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
        G_ASSERT(desc.NumParameters < array_size(params));
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
    debug("DX12: Generating compute pipeline signature with approximately %u dwords used", generator.signatureCost);
#endif
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;

    if (DX12_CHECK_FAIL(D3D12SerializeRootSignature(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      fatal("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
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
  auto &group = computePipelines[program.getGroup()];
  G_ASSERTF(group[index] != nullptr, "getCompute called for uninitialized compute pipeline! index was %u", index);
  return group[index].get();
}

BasePipeline *PipelineManager::getGraphics(GraphicsProgramID program)
{
  uint32_t index = program.getIndex();
  auto &group = graphicsPipelines[program.getGroup()];
  G_ASSERTF(group[index] != nullptr, "getGraphics called for uninitialized graphics pipeline! index was %u", index);
  return group[index].get();
}

void PipelineManager::addCompute(ID3D12Device2 *device, PipelineCache &cache, ProgramID id, ComputeShaderModule shader,
  RecoverablePipelineCompileBehavior on_error, bool give_name)
{
  G_ASSERTF(id.isCompute(), "addCompute called for a non compute program!");
  uint32_t index = id.getIndex();
  auto &group = computePipelines[id.getGroup()];
  while (group.size() <= index)
    group.emplace_back(nullptr);

  if (group[index])
    return;
#if _TARGET_SCARLETT
  bool hasAccelerationStructure = has_acceleration_structure(shader);
#else
  bool hasAccelerationStructure = false;
#endif
  if (auto signature = getComputePipelineSignature(device, cache, hasAccelerationStructure, shader.header.resourceUsageTable))
  {
    group[index] = eastl::make_unique<ComputePipeline>(*signature, eastl::move(shader), device, cache, on_error, give_name);
  }
}

void PipelineManager::addGraphics(ID3D12Device2 *device, PipelineCache &cache, FramebufferLayoutManager &fbs,
  GraphicsProgramID program, ShaderID vs, ShaderID ps, RecoverablePipelineCompileBehavior on_error, bool give_name)
{
  uint32_t index = program.getIndex();
  auto &group = graphicsPipelines[program.getGroup()];
  while (group.size() <= index)
    group.emplace_back(nullptr);

  auto &target = group[index];

  if (target)
  {
    return;
  }

  auto vertexShader = getVertexShader(vs);
  auto pixelShader = getPixelShader(ps);

  if (!vertexShader)
  {
    logerr("DX12: Tried to create graphics pipeline with unknown vertex shader with id %u", vs.exportValue());
  }
  if (!pixelShader)
  {
    logerr("DX12: Tried to create graphics pipeline with unknown pixel shader with id %u", ps.exportValue());
  }
  if (!vertexShader || !pixelShader)
  {
    return;
  }

#if _TARGET_SCARLETT
  bool hasAccelerationStructure = has_acceleration_structure(*vertexShader);
  if (!hasAccelerationStructure)
    hasAccelerationStructure = has_acceleration_structure(*pixelShader);
  if (!hasAccelerationStructure && vertexShader->geometryShader)
    hasAccelerationStructure = has_acceleration_structure(*vertexShader->geometryShader);
  if (!hasAccelerationStructure && vertexShader->hullShader)
    hasAccelerationStructure = has_acceleration_structure(*vertexShader->hullShader);
  if (!hasAccelerationStructure && vertexShader->domainShader)
    hasAccelerationStructure = has_acceleration_structure(*vertexShader->domainShader);
#else
  bool hasAccelerationStructure = false;
#endif

  auto &psHeader = pixelShader->header.resourceUsageTable;

  GraphicsPipelineSignature *signature = nullptr;
#if !_TARGET_XBOXONE
  if (is_mesh(*vertexShader))
  {
    auto &msHeader = vertexShader->header.resourceUsageTable;
    auto asHeader = vertexShader->geometryShader ? &vertexShader->geometryShader->header.resourceUsageTable : nullptr;
    signature = getGraphicsMeshPipelineSignature(device, cache, hasAccelerationStructure, msHeader, psHeader, asHeader);
  }
  else
#endif
  {
    bool hasVertexInputs = 0 != vertexShader->header.inOutSemanticMask;
    auto &vsHeader = vertexShader->header.resourceUsageTable;
    auto gsHeader = vertexShader->geometryShader ? &vertexShader->geometryShader->header.resourceUsageTable : nullptr;
    auto hsHeader = vertexShader->hullShader ? &vertexShader->hullShader->header.resourceUsageTable : nullptr;
    auto dsHeader = vertexShader->domainShader ? &vertexShader->domainShader->header.resourceUsageTable : nullptr;
    signature = getGraphicsPipelineSignature(device, cache, hasVertexInputs, hasAccelerationStructure, vsHeader, psHeader, gsHeader,
      hsHeader, dsHeader);
  }
  if (signature)
  {
    target = eastl::make_unique<BasePipeline>(device, program, *signature, cache, inputLayouts, staticRenderStateTable, fbs,
      *vertexShader, *pixelShader, on_error, give_name);
  }
}

void PipelineManager::unloadAll()
{
  blitPipelines.clear();
  blitSignature.Reset();

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
    fatal("Missing raygen shader for raytrace program");
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
  // just reset the list and they will be rebuild on demand
  blitPipelines.clear();

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
  blitSignature.Reset();
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
}

void PipelineStageStateBase::setSRVTexture(uint32_t unit, Image *image, ImageViewState view_state, bool as_const_ds,
  D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  if ((tRegisters[unit].image != image) || (tRegisters[unit].imageView != view_state) || (tRegisters[unit].view != view) ||
      isConstDepthStencil.test(unit) != as_const_ds)
  {
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
  }
}

void PipelineStageStateBase::setSampler(uint32_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler)
{
  if (sRegisters[unit] != sampler)
  {
    sRegisterValidMask &= ~(1 << unit);
    sRegisters[unit] = sampler;
  }
}

void PipelineStageStateBase::setUAVTexture(uint32_t unit, Image *image, ImageViewState view_state, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  if ((uRegisters[unit].image != image) || (uRegisters[unit].imageView != view_state) || (uRegisters[unit].view != view))
  {
    uRegisterValidMask &= ~(1 << unit);
    uRegisterStateDirtyMask |= 1u << unit;

    uRegisters[unit].image = image;
    uRegisters[unit].imageView = view_state;
    uRegisters[unit].view = view;

    uRegisters[unit].buffer = BufferReference{};
  }
}

bool PipelineStageStateBase::setSRVBuffer(uint32_t unit, BufferResourceReferenceAndShaderResourceView buffer)
{
  if (tRegisters[unit].view != buffer)
  {
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
  return false;
}

bool PipelineStageStateBase::setUAVBuffer(uint32_t unit, BufferResourceReferenceAndUnorderedResourceView buffer)
{
  if (uRegisters[unit].view != buffer)
  {
    uRegisterValidMask &= ~(1 << unit);
    uRegisterStateDirtyMask |= 1u << unit;

    uRegisters[unit].buffer = buffer;

    uRegisters[unit].image = nullptr;
    uRegisters[unit].imageView = ImageViewState();
    uRegisters[unit].view = buffer.uav;

    return true;
  }
  return false;
}

void PipelineStageStateBase::setConstBuffer(uint32_t unit, BufferResourceReferenceAndAddressRange buffer)
{
  if (bRegisters[unit] != buffer)
  {
    bRegisterValidMask &= ~(1 << unit);
    bRegisterStateDirtyMask |= 1u << unit;

    bRegisters[unit].BufferLocation = buffer.gpuPointer;
    bRegisters[unit].SizeInBytes = buffer.size;
    bRegisterBuffers[unit] = buffer;
  }
}

#if D3D_HAS_RAY_TRACING
void PipelineStageStateBase::setRaytraceAccelerationStructureAtT(uint32_t unit, RaytraceAccelerationStructure *as)
{
  if (tRegisters[unit].as != as)
  {
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
      tRegisters[unit].view = as->handle;
    }
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
  if (hasChanged)
  {
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
}

void PipelineStageStateBase::setUAVNull(uint32_t unit)
{
  auto &slot = uRegisters[unit];
  if (D3D12_CPU_DESCRIPTOR_HANDLE{0} != slot.view)
  {
    uRegisterValidMask &= ~(1 << unit);
    uRegisterStateDirtyMask |= 1u << unit;

    slot.buffer = BufferReference{};

    slot.image = nullptr;
    slot.imageView = ImageViewState{};
    slot.view = D3D12_CPU_DESCRIPTOR_HANDLE{0};
  }
}

namespace
{
template <typename T, typename U, typename V>
uint32_t maching_object_mask(const T &obj, U &container, V extractor)
{
  uint32_t mask = 0;
  for (uint32_t i = 0; i < array_size(container); ++i)
  {
    mask |= ((extractor(container[i]) == obj) ? 1u : 0u) << i;
  }
  return mask;
}
} // namespace

void PipelineStageStateBase::dirtyBufferState(BufferGlobalId id)
{
  bRegisterStateDirtyMask |= maching_object_mask(id, bRegisterBuffers, [](auto &ref) { return ref.resourceId; });
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
  for_each_set_bit(bRegisterStateDirtyMask & b_register_mask, [&res_usage, &barrier_batcher, stage, this](uint32_t i) {
    res_usage.useBufferAsCBV(barrier_batcher, stage, bRegisterBuffers[i]);
  });
  bRegisterStateDirtyMask &= ~b_register_mask;

  for_each_set_bit(tRegisterStateDirtyMask & t_register_mask,
    [&res_usage, &barrier_batcher, &split_tracker, stage, this](uint32_t i) //
    {
      if (tRegisters[i].image)
      {
        res_usage.useTextureAsSRV(barrier_batcher, split_tracker, stage, tRegisters[i].image, tRegisters[i].imageView,
          isConstDepthStencil.test(i));
      }
      else if (tRegisters[i].buffer)
      {
        res_usage.useBufferAsSRV(barrier_batcher, stage, tRegisters[i].buffer);
      }
      // RT structures do not need any state tracking
    });
  tRegisterStateDirtyMask &= ~t_register_mask;

  for_each_set_bit(uRegisterStateDirtyMask & u_register_mask,
    [&res_usage, &barrier_batcher, &split_tracker, stage, this](uint32_t i) //
    {
      if (uRegisters[i].image)
      {
        res_usage.useTextureAsUAV(barrier_batcher, split_tracker, stage, uRegisters[i].image, uRegisters[i].imageView);
      }
      else if (uRegisters[i].buffer)
      {
        res_usage.useBufferAsUAV(barrier_batcher, stage, uRegisters[i].buffer);
      }
    });
  uRegisterStateDirtyMask &= ~u_register_mask;
}
