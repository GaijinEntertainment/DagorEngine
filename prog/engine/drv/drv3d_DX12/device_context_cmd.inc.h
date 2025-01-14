// Copyright (C) Gaijin Games KFT.  All rights reserved.

/* Note: commands are sorted according to actual execution frequency (in order to help CPU's branch target predictor) */

DX12_BEGIN_CONTEXT_COMMAND(true, DrawIndexed)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, indexStart)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(int32_t, vertexBase)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushIndexBuffer();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexed(count, instanceCount, indexStart, vertexBase, firstInstance);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetRootConstants)
  DX12_CONTEXT_COMMAND_PARAM(unsigned, stage)
  DX12_CONTEXT_COMMAND_PARAM(RootConstatInfo, values)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setRootConstants(stage, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(false, SetUniformBuffer, char, name)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(ConstBufferSetupInformationStream, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUniformBuffer(stage, unit, buffer, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetSRVTexture)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(bool, asConstDS)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSRVTexture(stage, unit, image, viewState, asConstDS, viewDescriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetSampler)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, sampler)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSampler(stage, unit, sampler);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetSRVBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndShaderResourceView, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSRVBuffer(stage, unit, buffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setGraphicsPipeline(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetConstRegisterBuffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, update)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setConstRegisterBuffer(stage, update);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, TextureBarrier)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(SubresourceRange, subResRange)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, texFlags)
  DX12_CONTEXT_COMMAND_PARAM(ResourceBarrier, barrier)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)
  DX12_CONTEXT_COMMAND_PARAM(bool, forceBarrier)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.textureBarrier(image, subResRange, texFlags, barrier, queue, forceBarrier);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetStaticRenderState)
  DX12_CONTEXT_COMMAND_PARAM(StaticRenderStateID, ident)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setStaticRenderState(ident);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetFramebuffer)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(Image *, imageList, Driver3dRenderTarget::MAX_SIMRT + 1)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(ImageViewState, viewList, Driver3dRenderTarget::MAX_SIMRT + 1)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptors, Driver3dRenderTarget::MAX_SIMRT + 1)
  DX12_CONTEXT_COMMAND_PARAM(bool, readOnlyDepth)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.invalidateFramebuffer();
  auto nullCT = ctx.getNullColorTarget();
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    auto prevImage = ctx.getFramebufferState().frontendFrameBufferInfo.colorAttachments[i].image;
    if (prevImage)
      ctx.dirtyTextureStateForFramebufferAttachmentUse(prevImage);
    auto image = imageList[i];
    if (image)
    {
      ctx.getFramebufferState().bindColorTarget(i, image, viewList[i], viewDescriptors[i]);
    }
    else
    {
      ctx.getFramebufferState().clearColorTarget(i, nullCT);
    }
  }

  auto prevDepthImage = ctx.getFramebufferState().frontendFrameBufferInfo.depthStencilAttachment.image;
  if (prevDepthImage)
    ctx.dirtyTextureStateForFramebufferAttachmentUse(prevDepthImage);

  if (imageList[Driver3dRenderTarget::MAX_SIMRT])
  {
    ctx.getFramebufferState().bindDepthStencilTarget(imageList[Driver3dRenderTarget::MAX_SIMRT],
      viewList[Driver3dRenderTarget::MAX_SIMRT], viewDescriptors[Driver3dRenderTarget::MAX_SIMRT], readOnlyDepth);
    if (readOnlyDepth && prevDepthImage != imageList[Driver3dRenderTarget::MAX_SIMRT])
    {
      ctx.dirtyTextureStateForFramebufferAttachmentUse(imageList[Driver3dRenderTarget::MAX_SIMRT]);
    }
  }
  else
  {
    ctx.getFramebufferState().clearDepthStencilTarget();
  }
  // changing render targets may result in different target formats, which makes the pipeline
  // incompatible
  ctx.flushRenderTargets();
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BufferBarrier)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceBarrier, barrier)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bufferBarrier(buffer, barrier, queue);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(false, AsBarrier)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.asBarrier(as, queue);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(false, SetVertexBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stream)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVertexBuffer(stream, buffer, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, Draw)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, start)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.draw(count, instanceCount, start, firstInstance);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetInputLayout)
  DX12_CONTEXT_COMMAND_PARAM(InputLayoutID, ident)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setInputLayout(ident);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetUAVBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndUnorderedResourceView, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVBuffer(stage, unit, buffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetUAVTexture)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVTexture(stage, unit, image, viewState, viewDescriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetComputeProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setComputePipeline(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(false, SetViewports, ViewportState, viewports)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateViewports(viewports);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, Dispatch)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, x)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, y)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdDispatch);
  ctx.switchActivePipeline(ActivePipeline::Compute);
  ctx.flushComputeState();
  ctx.dispatch(x, y, z);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetIndexBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(DXGI_FORMAT, type)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setIndexBuffer(buffer, type);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(true, RaytraceBuildTopAccelerationStructure)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, scratchBuffer)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, instanceBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  DX12_CONTEXT_COMMAND_PARAM(bool, update)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, batchSize)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, batchIndex)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdRaytraceBuildTopAccelerationStructure);
  ctx.buildTopAccelerationStructure(batchSize, batchIndex, instanceCount, instanceBuffer, toAccelerationStructureBuildFlags(flags),
    update, as, nullptr, scratchBuffer);
#endif
DX12_END_CONTEXT_COMMAND
#endif

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(true, RaytraceCopyAccelerationStructure)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, dst)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, src)
  DX12_CONTEXT_COMMAND_PARAM(bool, compact)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdRaytraceCopyAccelerationStructure);
  ctx.copyRaytracingAccelerationStructure(dst, src, compact);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(true, MipMapGenSource)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(MipMapIndex, mip)
  DX12_CONTEXT_COMMAND_PARAM(ArrayLayerIndex, array)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.mipMapGenSource(image, mip, array);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, BlitImage)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, srcView)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, dstView)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, srcViewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, dstViewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RECT, srcRect)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RECT, dstRect)
  DX12_CONTEXT_COMMAND_PARAM(bool, disablePedication)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBlitImage);
  ctx.blitImage(src, dst, srcView, dstView, srcViewDescriptor, dstViewDescriptor, srcRect, dstRect, disablePedication);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DispatchIndirect)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdDispatchIndirect);
  ctx.switchActivePipeline(ActivePipeline::Compute);
  ctx.flushComputeState();
  ctx.dispatchIndirect(buffer);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
// Assuming this will be used a lot in the future this may be moved up after profiling
DX12_BEGIN_CONTEXT_COMMAND(true, DispatchMesh)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, x)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, y)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdDispatchMesh);
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsMeshState();
  ctx.dispatchMesh(x, y, z);
#endif
DX12_END_CONTEXT_COMMAND

// Assuming this will be used a lot in the future this may be moved up after profiling
DX12_BEGIN_CONTEXT_COMMAND(true, DispatchMeshIndirect)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, arguments)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, maxCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsMeshState();
  ctx.dispatchMeshIndirect(arguments, stride, count, maxCount);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(true, ClearRenderTargets)
  DX12_CONTEXT_COMMAND_PARAM(ViewportState, viewport)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, clearMask)
  DX12_CONTEXT_COMMAND_PARAM(float, clearDepth)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, clearStencil)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(E3DCOLOR, clearColor, Driver3dRenderTarget::MAX_SIMRT)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearRenderTargets);
  ctx.clearRenderTargets(viewport, clearMask, clearColor, clearDepth, clearStencil);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetSRVNull)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSRVNull(stage, unit);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetUAVNull)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVNull(stage, unit);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearBufferInt)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearBufferInt);
  ctx.clearBufferUint(buffer, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CopyImage)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
  DX12_CONTEXT_COMMAND_PARAM(ImageCopy, region)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdCopyImage);
  ctx.copyImage(src, dst, region);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ResolveMultiSampleImage)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdResolveMultiSampleImage);
  ctx.resolveMultiSampleImage(src, dst);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, EndCPUTextureAccess)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdEndCPUTextureAccess);
  ctx.onEndCPUTextureAccess(texture);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, UpdateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, source)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, dest)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdUpdateBuffer);
  ctx.updateBuffer(source, dest);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DrawIndirect)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndirect(buffer, count, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BeginCPUTextureAccess)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBeginCPUTextureAccess);
  ctx.onBeginCPUTextureAccess(texture);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, RegisterStaticRenderState)
  DX12_CONTEXT_COMMAND_PARAM(StaticRenderStateID, ident)
  DX12_CONTEXT_COMMAND_PARAM(RenderStateSystem::StaticState, state)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdRegisterStaticRenderState);
  ctx.registerStaticRenderState(ident, state);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearDepthStencilTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(ClearDepthStencilValue, value)
  DX12_CONTEXT_COMMAND_PARAM(eastl::optional<D3D12_RECT>, rect)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearDepthStencilTexture);
  ctx.clearDepthStencilImage(image, viewState, viewDescriptor, value, rect);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
DX12_BEGIN_CONTEXT_COMMAND(false, SetVariableRateShading)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE, constantRate)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE_COMBINER, vertexCombiner)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE_COMBINER, pixelCombiner)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVariableRateShading(constantRate, vertexCombiner, pixelCombiner);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(false, BeginConditionalRender)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginConditionalRender(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, BeginSurvey)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginSurvey(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetBlendConstantFactor)
  DX12_CONTEXT_COMMAND_PARAM(E3DCOLOR, constant)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setBlendConstantFactor(constant);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetDepthBoundsRange)
  DX12_CONTEXT_COMMAND_PARAM(float, from)
  DX12_CONTEXT_COMMAND_PARAM(float, to)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setDepthBoundsRange(from, to);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetPolygonLineEnable)
  DX12_CONTEXT_COMMAND_PARAM(bool, enable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setWireFrame(enable);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetStencilRef)
  DX12_CONTEXT_COMMAND_PARAM(uint8_t, ref)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setStencilRef(ref);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, SetScissorEnable)
  DX12_CONTEXT_COMMAND_PARAM(bool, enable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setScissorEnable(enable);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(false, SetScissorRects, D3D12_RECT, rects)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setScissorRects(rects);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DrawIndexedIndirect)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.flushIndexBuffer();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexedIndirect(buffer, count, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DrawUserData)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, userData)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.bindVertexUserData(userData, stride);
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.draw(count, 1, 0, 0);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DrawIndexedUserData)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, vertexStride)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, vertexData)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, indexData)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.switchActivePipeline(ActivePipeline::Graphics);
  ctx.ensureActivePass();
  ctx.bindIndexUser(indexData);
  ctx.bindVertexUserData(vertexData, vertexStride);
  ctx.flushGraphicsStateResourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexed(count, 1, 0, 0, 0);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CopyBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, source)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, dest)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dataSize)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdCopyBuffer);
  ctx.copyBuffer(source, dest, dataSize);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearBufferFloat)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(float, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearBufferFloat);
  ctx.clearBufferFloat(buffer, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, PushEvent, char, text)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.pushEvent(text);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, PopEvent)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.popEvent();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearColorTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(ClearColorValue, value)
  DX12_CONTEXT_COMMAND_PARAM(eastl::optional<D3D12_RECT>, rect)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearColorTexture);
  ctx.clearColorImage(image, viewState, viewDescriptor, value, rect);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearUAVTextureI)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, view)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearUAVTextureI);
  ctx.clearUAVTextureI(image, view, viewDescriptor, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ClearUAVTextureF)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, view)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(float, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdClearUAVTextureF);
  ctx.clearUAVTextureF(image, view, viewDescriptor, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, FlushWithFence)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, progress)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdFlushWithFence);
  ctx.flush(progress);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, EndSurvey)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endSurvey(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, FinishFrame)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, progress)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, latencyFrameId)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, frontFrameId)
  DX12_CONTEXT_COMMAND_PARAM(bool, presentOnSwapchain)
  DX12_CONTEXT_COMMAND_PARAM(Drv3dTimings *, timingData)
  DX12_CONTEXT_COMMAND_PARAM(int64_t, kickoffStamp)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdFinishFrame);
  ctx.finishFrame(progress, timingData, kickoffStamp, latencyFrameId, frontFrameId, presentOnSwapchain);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, InsertTimestampQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeTimestamp(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, EndConditionalRender)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endConditionalRender();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, AddVertexShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, id)
  DX12_CONTEXT_COMMAND_PARAM(VertexShaderModule *, sci)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addVertexShader(id, sci);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, AddPixelShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, id)
  DX12_CONTEXT_COMMAND_PARAM(PixelShaderModule *, sci)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addPixelShader(id, sci);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, AddComputeProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, id)
  DX12_CONTEXT_COMMAND_PARAM(ComputeShaderModule *, csm)
  DX12_CONTEXT_COMMAND_PARAM(CSPreloaded, preloaded)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addComputePipeline(id, csm, preloaded);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, AddGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, vs)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, fs)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addGraphicsPipeline(program, vs, fs);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, PlaceAftermathMarker, char, string)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeToDebug(string);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(false, SetRaytraceAccelerationStructure)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setRaytraceAccelerationStructureAtT(stage, unit, as);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_2(true, RaytraceBuildBottomAccelerationStructure, D3D12_RAYTRACING_GEOMETRY_DESC, geometryDescriptions,
  RaytraceGeometryDescriptionBufferResourceReferenceSet, resourceReferences)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, scratchBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, compactedSize)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  DX12_CONTEXT_COMMAND_PARAM(bool, update)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, batchSize)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, batchIndex)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdRaytraceBuildBottomAccelerationStructure);
  ctx.buildBottomAccelerationStructure(batchSize, batchIndex, geometryDescriptions, resourceReferences,
    toAccelerationStructureBuildFlags(flags), update, as, nullptr, scratchBuffer, compactedSize);
#endif
DX12_END_CONTEXT_COMMAND

#endif

DX12_BEGIN_CONTEXT_COMMAND(false, ChangePresentMode)
  DX12_CONTEXT_COMMAND_PARAM(PresentationMode, presentMode)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.changePresentMode(presentMode);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(false, UpdateVertexShaderName, char, name)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateVertexShaderName(shader, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(false, UpdatePixelShaderName, char, name)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updatePixelShaderName(shader, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, BeginVisibilityQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginVisibilityQuery(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, EndVisibilityQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endVisibilityQuery(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, TextureReadBack, BufferImageCopy, regions)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(DeviceQueueType, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdTextureReadBack);
  ctx.textureReadBack(image, cpuMemory, regions, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, BufferReadBack)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(size_t, offset)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBufferReadBack);
  ctx.bufferReadBack(buffer, cpuMemory, offset);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
DX12_BEGIN_CONTEXT_COMMAND(false, SetVariableRateShadingTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVariableRateShadingTexture(texture);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(true, RegisterInputLayout)
  DX12_CONTEXT_COMMAND_PARAM(InputLayoutID, ident)
  DX12_CONTEXT_COMMAND_PARAM(InputLayout, layout)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerInputLayout(ident, layout);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CreateDlssFeature)
  DX12_CONTEXT_COMMAND_PARAM(bool, stereo_render)
  DX12_CONTEXT_COMMAND_PARAM(int, output_width)
  DX12_CONTEXT_COMMAND_PARAM(int, output_height)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdCreateDlssFeature);
  ctx.createDlssFeature(stereo_render, output_width, output_height);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ReleaseDlssFeature)
  DX12_CONTEXT_COMMAND_PARAM(bool, stereo_render)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdReleaseDlssFeature);
  ctx.releaseDlssFeature(stereo_render);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ExecuteDlss)
  DX12_CONTEXT_COMMAND_PARAM(nv::DlssParams<Image>, dlss_params)
  DX12_CONTEXT_COMMAND_PARAM(int, view_index)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, frame_id)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdExecuteDlss);
  ctx.executeDlss(dlss_params, view_index, frame_id);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ExecuteDlssG)
  DX12_CONTEXT_COMMAND_PARAM(nv::DlssGParams<Image>, dlss_g_params)
  DX12_CONTEXT_COMMAND_PARAM(int, view_index)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdExecuteDlssG);
  ctx.executeDlssG(dlss_g_params, view_index);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ExecuteXess)
  DX12_CONTEXT_COMMAND_PARAM(XessParamsDx12, params)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdExecuteXess);
  ctx.executeXess(params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DispatchFSR2)
  DX12_CONTEXT_COMMAND_PARAM(Fsr2ParamsDx12, params)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdDispatchFSR2);
  ctx.executeFSR2(params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DispatchFSR)
  DX12_CONTEXT_COMMAND_PARAM(amd::FSR *, fsr)
  DX12_CONTEXT_COMMAND_PARAM(FSRUpscalingArgs, params)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdDispatchFSR);
  ctx.executeFSR(fsr, params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, BeginCapture, wchar_t, name)
  DX12_CONTEXT_COMMAND_PARAM(UINT, flags)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCapture(flags, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, EndCapture)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endCapture();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, CaptureNextFrames, wchar_t, name)
  DX12_CONTEXT_COMMAND_PARAM(UINT, flags)
  DX12_CONTEXT_COMMAND_PARAM(int, frame_count)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.captureNextFrames(flags, name, frame_count);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, RemoveVertexShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeVertexShader(shader);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, RemovePixelShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removePixelShader(shader);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DeleteProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteProgram(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DeleteGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteGraphicsProgram(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, DeleteQueries, Query *, queries)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteQueries(queries);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, HostToDeviceMemoryCopy)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, gpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(size_t, cpuOffset)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdHostToDeviceMemoryCopy);
  ctx.hostToDeviceMemoryCopy(gpuMemory, cpuMemory, cpuOffset);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, InitializeTextureState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RESOURCE_STATES, state)
  DX12_CONTEXT_COMMAND_PARAM(ValueRange<ExtendedImageGlobalSubresourceId>, idRange)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.initializeTextureState(state, idRange);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, UploadTexture, BufferImageCopy, regions)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(DeviceQueueType, queue)
  DX12_CONTEXT_COMMAND_PARAM(bool, isDiscard)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdUploadTexture);
  ctx.uploadTexture(texture, regions, cpuMemory, queue, isDiscard);
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_XBOX
DX12_BEGIN_CONTEXT_COMMAND(true, UpdateFrameInterval)
  DX12_CONTEXT_COMMAND_PARAM(int32_t, freqLevel)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdUpdateFrameInterval);
  ctx.updateFrameInterval(freqLevel);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ResummarizeHtile)
  DX12_CONTEXT_COMMAND_PARAM(ID3D12Resource *, depth)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdResummarizeHtile);
  ctx.resummarizeHtile(depth);
#endif

DX12_END_CONTEXT_COMMAND
#endif

// this command only exists to wake up a potentially sleeping worker
DX12_BEGIN_CONTEXT_COMMAND(true, WorkerPing)
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, Terminate)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.terminateWorker();
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_XBOX
DX12_BEGIN_CONTEXT_COMMAND(true, EnterSuspendState)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.enterSuspendState();
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, WriteDebugMessage, char, message)
  DX12_CONTEXT_COMMAND_PARAM(int, severity)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeDebugMessage(message, severity);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BindlessSetResourceDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, descriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBindlessSetResourceDescriptor);
  ctx.bindlessSetResourceDescriptor(slot, descriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BindlessSetTextureDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, view)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBindlessSetTextureDescriptor);
  ctx.bindlessSetResourceDescriptor(slot, view);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BindlessSetSamplerDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, descriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBindlessSetSamplerDescriptor);
  ctx.bindlessSetSamplerDescriptor(slot, descriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, BindlessCopyResourceDescriptors)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, src)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dst)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdBindlessCopyResourceDescriptors);
  ctx.bindlessCopyResourceDescriptors(src, dst, count);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, RegisterFrameCompleteEvent)
  DX12_CONTEXT_COMMAND_PARAM(os_event_t, event)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameCompleteEvent(event);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, RegisterFrameEventsCallback)
  DX12_CONTEXT_COMMAND_PARAM(FrameEvents *, callback)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameEventsCallback(callback);
#endif
DX12_END_CONTEXT_COMMAND

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
DX12_BEGIN_CONTEXT_COMMAND(false, ResetBufferState)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdResetBufferState);
  ctx.resetBufferState(buffer);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(false, AddSwapchainView)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewInfo, view)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdAddSwapchainView);
  ctx.addSwapchainView(image, view);
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_PC_WIN

DX12_BEGIN_CONTEXT_COMMAND(true, OnDeviceError)
  DX12_CONTEXT_COMMAND_PARAM(HRESULT, removerReason)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdOnDeviceError);
  ctx.onDeviceError(removerReason);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, OnSwapchainSwapCompleted)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.onSwapchainSwapCompleted();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CommandFence)
  DX12_CONTEXT_COMMAND_PARAM(std::atomic_bool &, signal)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.commandFence(signal);
#endif
DX12_END_CONTEXT_COMMAND

#endif

#if _TARGET_PC_WIN
DX12_BEGIN_CONTEXT_COMMAND(true, BeginTileMapping)
  DX12_CONTEXT_COMMAND_PARAM(Image *, tex)
  DX12_CONTEXT_COMMAND_PARAM(ID3D12Heap *, heap)
  DX12_CONTEXT_COMMAND_PARAM(size_t, heapBase)
  DX12_CONTEXT_COMMAND_PARAM(size_t, mappingCount)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginTileMapping(tex, heap, heapBase, mappingCount);
#endif

#else
DX12_BEGIN_CONTEXT_COMMAND(true, BeginTileMapping)
  DX12_CONTEXT_COMMAND_PARAM(Image *, tex)
  DX12_CONTEXT_COMMAND_PARAM(uintptr_t, address)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, size)
  DX12_CONTEXT_COMMAND_PARAM(size_t, mappingCount)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginTileMapping(tex, address, size, mappingCount);
#endif
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, AddTileMappings, TileMapping, mapping)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addTileMappings(mapping.data(), mapping.size());
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, EndTileMapping)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endTileMapping();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ActivateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRangeWithClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceMemory, memory)
  DX12_CONTEXT_COMMAND_PARAM(ResourceActivationAction, action)
  DX12_CONTEXT_COMMAND_PARAM(ResourceClearValue, clearValue)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdActivateBuffer);
  ctx.activateBuffer(buffer, memory, action, clearValue, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ActivateTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(ResourceActivationAction, action)
  DX12_CONTEXT_COMMAND_PARAM(ResourceClearValue, clearValue)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, clearViewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, clearViewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdActivateTexture);
  ctx.activateTexture(texture, action, clearValue, clearViewState, clearViewDescriptor, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DeactivateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceMemory, memory)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deactivateBuffer(buffer, memory, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DeactivateTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deactivateTexture(texture, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, AliasFlush)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdAliasFlush);
  ctx.aliasFlush(queue);
#endif
DX12_END_CONTEXT_COMMAND


DX12_BEGIN_CONTEXT_COMMAND(true, TwoPhaseCopyBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, source)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, destinationOffset)
  DX12_CONTEXT_COMMAND_PARAM(ScratchBuffer, scratchMemory)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, size)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdTwoPhaseCopyBuffer);
  ctx.twoPhaseCopyBuffer(source, destinationOffset, scratchMemory, size);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, MoveBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, from)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, to)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdMoveBuffer);
  ctx.moveBuffer(from, to);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, MoveTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, from)
  DX12_CONTEXT_COMMAND_PARAM(Image *, to)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdMoveTexture);
  ctx.moveTexture(from, to);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(false, TransitionBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RESOURCE_STATES, state)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.transitionBuffer(buffer, state);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, ResizeImageMipMapTransfer)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
  DX12_CONTEXT_COMMAND_PARAM(MipMapRange, mipMapRange)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, srcMipMapOffset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dstMipMapOffset)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdResizeImageMipMapTransfer);
  ctx.resizeImageMipMapTransfer(src, dst, mipMapRange, srcMipMapOffset, dstMipMapOffset);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, DebugBreak)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.debugBreak();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, AddDebugBreakString, char, str)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addDebugBreakString(str);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, RemoveDebugBreakString, char, str)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeDebugBreakString(str);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, AddShaderGroup, char, name)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, group)
  DX12_CONTEXT_COMMAND_PARAM(ScriptedShadersBinDumpOwner *, dump)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, nullPixelShader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdAddShaderGroup);
  ctx.addShaderGroup(group, dump, nullPixelShader, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, RemoveShaderGroup)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, group)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdRemoveShaderGroup);
  ctx.removeShaderGroup(group);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, LoadComputeShaderFromDump)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdLoadComputeShaderFromDump);
  ctx.loadComputeShaderFromDump(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CompilePipelineSet)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<InputLayoutIDWithHash>, inputLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<StaticRenderStateIDWithHash>, staticRenderStates)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<FramebufferLayoutWithHash>, framebufferLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<GraphicsPipelinePreloadInfo>, graphicsPipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<MeshPipelinePreloadInfo>, meshPipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<ComputePipelinePreloadInfo>, computePipelines)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdCompilePipelineSet);
  ctx.compilePipelineSet(DynamicArray<InputLayoutIDWithHash>::fromSpan(inputLayouts),
    DynamicArray<StaticRenderStateIDWithHash>::fromSpan(staticRenderStates),
    DynamicArray<FramebufferLayoutWithHash>::fromSpan(framebufferLayouts),
    DynamicArray<GraphicsPipelinePreloadInfo>::fromSpan(graphicsPipelines),
    DynamicArray<MeshPipelinePreloadInfo>::fromSpan(meshPipelines),
    DynamicArray<ComputePipelinePreloadInfo>::fromSpan(computePipelines));
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(true, CompilePipelineSet2)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<InputLayoutIDWithHash>, inputLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<StaticRenderStateIDWithHash>, staticRenderStates)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<FramebufferLayoutWithHash>, framebufferLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<cacheBlk::SignatureEntry>, scriptedShaderDumpSignature)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<cacheBlk::ComputeClassUse>, computePipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<cacheBlk::GraphicsVariantGroup>, graphicsPipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<cacheBlk::GraphicsVariantGroup>, graphicsWithNullOverridePipelines)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, nullPixelShader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdCompilePipelineSet2);
  ctx.compilePipelineSet(DynamicArray<InputLayoutIDWithHash>::fromSpan(inputLayouts),
    DynamicArray<StaticRenderStateIDWithHash>::fromSpan(staticRenderStates),
    DynamicArray<FramebufferLayoutWithHash>::fromSpan(framebufferLayouts),
    DynamicArray<cacheBlk::SignatureEntry>::fromSpan(scriptedShaderDumpSignature),
    DynamicArray<cacheBlk::ComputeClassUse>::fromSpan(computePipelines),
    DynamicArray<cacheBlk::GraphicsVariantGroup>::fromSpan(graphicsPipelines),
    DynamicArray<cacheBlk::GraphicsVariantGroup>::fromSpan(graphicsWithNullOverridePipelines), nullPixelShader);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, DispatchRays, uint32_t, rootConstants)
  DX12_CONTEXT_COMMAND_PARAM(RayDispatchBasicParameters, dispatchParameters)
  DX12_CONTEXT_COMMAND_PARAM(RayDispatchParameters, params)
  DX12_CONTEXT_COMMAND_PARAM(ResourceBindingTable, resourceBindingTable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.dispatchRays(dispatchParameters, resourceBindingTable, rootConstants, params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(true, DispatchRaysIndirect, uint32_t, rootConstants)
  DX12_CONTEXT_COMMAND_PARAM(RayDispatchBasicParameters, dispatchParameters)
  DX12_CONTEXT_COMMAND_PARAM(RayDispatchIndirectParameters, dispatchIndirectParameters)
  DX12_CONTEXT_COMMAND_PARAM(ResourceBindingTable, resourceBindingTable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.dispatchRaysIndirect(dispatchParameters, resourceBindingTable, rootConstants, dispatchIndirectParameters);
#endif
DX12_END_CONTEXT_COMMAND
#endif
