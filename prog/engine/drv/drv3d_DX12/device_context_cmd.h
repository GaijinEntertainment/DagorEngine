/* Note: commands are sorted according to actual execution frequency (in order to help CPU's branch target predictor) */

DX12_BEGIN_CONTEXT_COMMAND(DrawIndexed)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, indexStart)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(int32_t, vertexBase)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushIndexBuffer();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexed(count, instanceCount, indexStart, vertexBase, firstInstance);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetVertexRootConstant)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, value)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVertexRootConstant(offset, value);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetUniformBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUniformBuffer(stage, unit, buffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetSRVTexture)
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

DX12_BEGIN_CONTEXT_COMMAND(SetSampler)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, sampler)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSampler(stage, unit, sampler);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetSRVBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndShaderResourceView, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSRVBuffer(stage, unit, buffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setGraphicsPipeline(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetConstRegisterBuffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, update)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setConstRegisterBuffer(stage, update);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(TextureBarrier)
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

DX12_BEGIN_CONTEXT_COMMAND(SetPixelRootConstant)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, value)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setPixelRootConstant(offset, value);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetStaticRenderState)
  DX12_CONTEXT_COMMAND_PARAM(StaticRenderStateID, ident)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setStaticRenderState(ident);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetFramebuffer)
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

DX12_BEGIN_CONTEXT_COMMAND(BufferBarrier)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceBarrier, barrier)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bufferBarrier(buffer, barrier, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetVertexBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stream)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVertexBuffer(stream, buffer, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(Draw)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, start)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.draw(count, instanceCount, start, firstInstance);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetInputLayout)
  DX12_CONTEXT_COMMAND_PARAM(InputLayoutID, ident)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setInputLayout(ident);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetUAVBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndUnorderedResourceView, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVBuffer(stage, unit, buffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetUAVTexture)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVTexture(stage, unit, image, viewState, viewDescriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetComputeProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setComputePipeline(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(SetViewports, ViewportState, viewports)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateViewports(viewports);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(Dispatch)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, x)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, y)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.flushComputeState();
  ctx.dispatch(x, y, z);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetIndexBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(DXGI_FORMAT, type)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setIndexBuffer(buffer, type);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(RaytraceBuildTopAccelerationStructure)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, scratchBuffer)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, indexBuffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, indexCount)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  DX12_CONTEXT_COMMAND_PARAM(bool, update)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.buildTopAccelerationStructure(indexCount, indexBuffer, toAccelerationStructureBuildFlags(flags), update, as->getResourceHandle(),
    nullptr, scratchBuffer);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(MipMapGenSource)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(MipMapIndex, mip)
  DX12_CONTEXT_COMMAND_PARAM(ArrayLayerIndex, array)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.mipMapGenSource(image, mip, array);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BlitImage)
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
  ctx.blitImage(src, dst, srcView, dstView, srcViewDescriptor, dstViewDescriptor, srcRect, dstRect, disablePedication);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DispatchIndirect)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.flushComputeState();
  ctx.dispatchIndirect(buffer);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
// Assuming this will be used a lot in the future this may be moved up after profiling
DX12_BEGIN_CONTEXT_COMMAND(DispatchMesh)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, x)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, y)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsMeshState();
  ctx.dispatchMesh(x, y, z);
#endif
DX12_END_CONTEXT_COMMAND

// Assuming this will be used a lot in the future this may be moved up after profiling
DX12_BEGIN_CONTEXT_COMMAND(DispatchMeshIndirect)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, arguments)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, maxCount)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsMeshState();
  ctx.dispatchMeshIndirect(arguments, stride, count, maxCount);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(ClearRenderTargets)
  DX12_CONTEXT_COMMAND_PARAM(ViewportState, viewport)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, clearMask)
  DX12_CONTEXT_COMMAND_PARAM(float, clearDepth)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, clearStencil)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(E3DCOLOR, clearColor, Driver3dRenderTarget::MAX_SIMRT)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearRenderTargets(viewport, clearMask, clearColor, clearDepth, clearStencil);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetSRVNull)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSRVNull(stage, unit);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetUAVNull)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setUAVNull(stage, unit);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ClearBufferInt)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearBufferUint(buffer, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(CopyImage)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
  DX12_CONTEXT_COMMAND_PARAM(ImageCopy, region)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.copyImage(src, dst, region);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ResolveMultiSampleImage)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.resolveMultiSampleImage(src, dst);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndCPUTextureAccess)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.onEndCPUTextureAccess(texture);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(UpdateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, source)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, dest)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateBuffer(source, dest);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DrawIndirect)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndirect(buffer, count, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BeginCPUTextureAccess)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.onBeginCPUTextureAccess(texture);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RegisterStaticRenderState)
  DX12_CONTEXT_COMMAND_PARAM(StaticRenderStateID, ident)
  DX12_CONTEXT_COMMAND_PARAM(RenderStateSystem::StaticState, state)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerStaticRenderState(ident, state);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(SetRaytraceProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setRaytracePipeline(program);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(ClearDepthStencilTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(ClearDepthStencilValue, value)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearDepthStencilImage(image, viewState, viewDescriptor, value);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
DX12_BEGIN_CONTEXT_COMMAND(SetVariableRateShading)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE, constantRate)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE_COMBINER, vertexCombiner)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_SHADING_RATE_COMBINER, pixelCombiner)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVariableRateShading(constantRate, vertexCombiner, pixelCombiner);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(BeginConditionalRender)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginConditionalRender(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BeginSurvey)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginSurvey(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetBlendConstantFactor)
  DX12_CONTEXT_COMMAND_PARAM(E3DCOLOR, constant)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setBlendConstantFactor(constant);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetDepthBoundsRange)
  DX12_CONTEXT_COMMAND_PARAM(float, from)
  DX12_CONTEXT_COMMAND_PARAM(float, to)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setDepthBoundsRange(from, to);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetPolygonLineEnable)
  DX12_CONTEXT_COMMAND_PARAM(bool, enable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setWireFrame(enable);
  ctx.invalidateActiveGraphicsPipeline();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetStencilRef)
  DX12_CONTEXT_COMMAND_PARAM(uint8_t, ref)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setStencilRef(ref);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetScissorEnable)
  DX12_CONTEXT_COMMAND_PARAM(bool, enable)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setScissorEnable(enable);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(SetScissorRects, D3D12_RECT, rects)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setScissorRects(rects);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DrawIndexedIndirect)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, buffer)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushIndexBuffer();
  ctx.flushVertexBuffers();
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexedIndirect(buffer, count, stride);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DrawUserData)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, userData)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.bindVertexUserData(userData, stride);
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.draw(count, 1, 0, 0);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DrawIndexedUserData)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_PRIMITIVE_TOPOLOGY, top)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, vertexStride)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, vertexData)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, indexData)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.bindIndexUser(indexData);
  ctx.bindVertexUserData(vertexData, vertexStride);
  ctx.flushGraphicsStateRessourceBindings();
  ctx.flushGraphicsState(top);
  ctx.drawIndexed(count, 1, 0, 0, 0);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(CopyBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, source)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, dest)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dataSize)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.copyBuffer(source, dest, dataSize);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ClearBufferFloat)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(float, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearBufferFloat(buffer, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(PushEvent, char, text)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.pushEvent(text);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(PopEvent)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.popEvent();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ClearColorTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, viewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(ClearColorValue, value)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearColorImage(image, viewState, viewDescriptor, value);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ClearUAVTextureI)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, view)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearUAVTextureI(image, view, viewDescriptor, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ClearUAVTextureF)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, view)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, viewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM_ARRAY(float, values, 4)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearUAVTextureF(image, view, viewDescriptor, values);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(FlushWithFence)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, progress)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.flush(false, progress, OutputMode::PRESENT);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndSurvey)
  DX12_CONTEXT_COMMAND_PARAM(PredicateInfo, pi)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endSurvey(pi);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(Present)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, progress)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, latencyFrameId)
  DX12_CONTEXT_COMMAND_PARAM(OutputMode, outputMode)
  DX12_CONTEXT_COMMAND_PARAM(Drv3dTimings *, timingData)
  DX12_CONTEXT_COMMAND_PARAM(int64_t, kickoffStamp)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  TIME_PROFILE_DEV(CmdPresent);
  ctx.present(progress, timingData, kickoffStamp, latencyFrameId, outputMode);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(InsertTimestampQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeTimestamp(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndConditionalRender)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endConditionalRender();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddVertexShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, id)
  DX12_CONTEXT_COMMAND_PARAM(VertexShaderModule *, sci)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addVertexShader(id, sci);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddPixelShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, id)
  DX12_CONTEXT_COMMAND_PARAM(PixelShaderModule *, sci)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addPixelShader(id, sci);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddComputeProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, id)
  DX12_CONTEXT_COMMAND_PARAM(ComputeShaderModule *, csm)
  DX12_CONTEXT_COMMAND_PARAM(CSPreloaded, preloaded)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addComputePipeline(id, csm, preloaded);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, vs)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, fs)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addGraphicsPipeline(program, vs, fs);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(PlaceAftermathMarker, char, string)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeToDebug(string);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(TraceRays)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, rayGenTable)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, missTable)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, hitTable)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, callableTable)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, missStride)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, hitStride)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, callableStride)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, width)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, height)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, depth)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.traceRays(rayGenTable, missTable, hitTable, callableTable, missStride, hitStride, callableStride, width, height, depth);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetRaytraceAccelerationStructure)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, stage)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, unit)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setRaytraceAccelerationStructureAtT(stage, unit, as);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_2(RaytraceBuildBottomAccelerationStructure, D3D12_RAYTRACING_GEOMETRY_DESC, geometryDescriptions,
  RaytraceGeometryDescriptionBufferResourceReferenceSet, resourceReferences)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddress, scratchBuffer)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  DX12_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  DX12_CONTEXT_COMMAND_PARAM(bool, update)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.buildBottomAccelerationStructure(geometryDescriptions, resourceReferences, toAccelerationStructureBuildFlags(flags), update,
    as->getResourceHandle(), nullptr, scratchBuffer);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddRaytraceProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)
  DX12_CONTEXT_COMMAND_PARAM(const ShaderID *, shaders)
  DX12_CONTEXT_COMMAND_PARAM(const RaytraceShaderGroup *, shaderGroups)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, shaderCount)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, groupCount)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, maxRecursion)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addRaytracePipeline(program, maxRecursion, shaderCount, shaders, groupCount, shaderGroups);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(CopyRaytraceShaderGroupHandlesToMemory)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, firstGroup)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, groupCount)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, size)
  DX12_CONTEXT_COMMAND_PARAM(void *, ptr)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.copyRaytraceShaderGroupHandlesToMemory(program, firstGroup, groupCount, size, ptr);
#endif
DX12_END_CONTEXT_COMMAND

#endif

DX12_BEGIN_CONTEXT_COMMAND(ChangePresentMode)
  DX12_CONTEXT_COMMAND_PARAM(PresentationMode, presentMode)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.changePresentMode(presentMode);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(UpdateVertexShaderName, char, name)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateVertexShaderName(shader, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(UpdatePixelShaderName, char, name)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updatePixelShaderName(shader, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetGamma)
  DX12_CONTEXT_COMMAND_PARAM(float, power)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setGamma(power);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(SetComputeRootConstant)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, value)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setComputeRootConstant(offset, value);
#endif
DX12_END_CONTEXT_COMMAND

#if D3D_HAS_RAY_TRACING
DX12_BEGIN_CONTEXT_COMMAND(SetRaytraceRootConstant)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, value)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setRaytraceRootConstant(offset, value);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(BeginVisibilityQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginVisibilityQuery(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndVisibilityQuery)
  DX12_CONTEXT_COMMAND_PARAM(Query *, query)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endVisibilityQuery(query);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(TextureReadBack, BufferImageCopy, regions)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(DeviceQueueType, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.textureReadBack(image, cpuMemory, regions, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BufferReadBack)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(size_t, offset)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bufferReadBack(buffer, cpuMemory, offset);
#endif
DX12_END_CONTEXT_COMMAND

#if !_TARGET_XBOXONE
DX12_BEGIN_CONTEXT_COMMAND(SetVariableRateShadingTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setVariableRateShadingTexture(texture);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(RegisterInputLayout)
  DX12_CONTEXT_COMMAND_PARAM(InputLayoutID, ident)
  DX12_CONTEXT_COMMAND_PARAM(InputLayout, layout)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerInputLayout(ident, layout);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(CreateDlssFeature)
  DX12_CONTEXT_COMMAND_PARAM(int, dlssQuality)
  DX12_CONTEXT_COMMAND_PARAM(Extent2D, outputResolution)
  DX12_CONTEXT_COMMAND_PARAM(bool, stereo_render)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.createDlssFeature(dlssQuality, outputResolution, stereo_render);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ReleaseDlssFeature)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.releaseDlssFeature();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ExecuteDlss)
  DX12_CONTEXT_COMMAND_PARAM(DlssParamsDx12, dlss_params)
  DX12_CONTEXT_COMMAND_PARAM(int, view_index)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.executeDlss(dlss_params, view_index);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ExecuteXess)
  DX12_CONTEXT_COMMAND_PARAM(XessParamsDx12, params)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.executeXess(params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DispatchFSR2)
  DX12_CONTEXT_COMMAND_PARAM(Fsr2ParamsDx12, params)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.executeFSR2(params);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(BeginCapture, wchar_t, name)
  DX12_CONTEXT_COMMAND_PARAM(UINT, flags)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCapture(flags, name);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndCapture)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endCapture();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(CaptureNextFrames, wchar_t, name)
  DX12_CONTEXT_COMMAND_PARAM(UINT, flags)
  DX12_CONTEXT_COMMAND_PARAM(int, frame_count)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.captureNextFrames(flags, name, frame_count);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RemoveVertexShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeVertexShader(shader);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RemovePixelShader)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, shader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removePixelShader(shader);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DeleteProgram)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteProgram(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DeleteGraphicsProgram)
  DX12_CONTEXT_COMMAND_PARAM(GraphicsProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteGraphicsProgram(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(DeleteQueries, Query *, queries)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deleteQueries(queries);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(HostToDeviceMemoryCopy)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndRange, gpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(size_t, cpuOffset)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.hostToDeviceMemoryCopy(gpuMemory, cpuMemory, cpuOffset);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(InitializeTextureState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RESOURCE_STATES, state)
  DX12_CONTEXT_COMMAND_PARAM(ValueRange<ExtendedImageGlobalSubresouceId>, idRange)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.initializeTextureState(state, idRange);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(UploadTexture, BufferImageCopy, regions)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(HostDeviceSharedMemoryRegion, cpuMemory)
  DX12_CONTEXT_COMMAND_PARAM(DeviceQueueType, queue)
  DX12_CONTEXT_COMMAND_PARAM(bool, isDiscard)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.uploadTexture(texture, regions, cpuMemory, queue, isDiscard);
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_XBOX
DX12_BEGIN_CONTEXT_COMMAND(UpdateFrameInterval)
  DX12_CONTEXT_COMMAND_PARAM(int32_t, freqLevel)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.updateFrameInterval(freqLevel);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ResummarizeHtile)
  DX12_CONTEXT_COMMAND_PARAM(ID3D12Resource *, depth)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.resummarizeHtile(depth);
#endif

DX12_END_CONTEXT_COMMAND
#endif

// this command only exists to wake up a potentially sleeping worker
DX12_BEGIN_CONTEXT_COMMAND(WorkerPing)
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(Terminate)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.terminateWorker();
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_XBOX
DX12_BEGIN_CONTEXT_COMMAND(EnterSuspendState)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.enterSuspendState();
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(WriteDebugMessage, char, message)
  DX12_CONTEXT_COMMAND_PARAM(int, severity)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeDebugMessage(message, severity);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BindlessSetResourceDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, descriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bindlessSetResourceDescriptor(slot, descriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BindlessSetTextureDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, view)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bindlessSetResourceDescriptor(slot, view);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BindlessSetSamplerDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, slot)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, descriptor)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bindlessSetSamplerDescriptor(slot, descriptor);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(BindlessCopyResourceDescriptors)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, src)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dst)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, count)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bindlessCopyResourceDescriptors(src, dst, count);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RegisterFrameCompleteEvent)
  DX12_CONTEXT_COMMAND_PARAM(os_event_t, event)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameCompleteEvent(event);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RegisterFrameEventsCallback)
  DX12_CONTEXT_COMMAND_PARAM(FrameEvents *, callback)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameEventsCallback(callback);
#endif
DX12_END_CONTEXT_COMMAND

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
DX12_BEGIN_CONTEXT_COMMAND(ResetBufferState)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.resetBufferState(buffer);
#endif
DX12_END_CONTEXT_COMMAND
#endif

DX12_BEGIN_CONTEXT_COMMAND(AddSwapchainView)
  DX12_CONTEXT_COMMAND_PARAM(Image *, image)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewInfo, view)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addSwapchainView(image, view);
#endif
DX12_END_CONTEXT_COMMAND

#if _TARGET_PC_WIN

DX12_BEGIN_CONTEXT_COMMAND(OnDeviceError)
  DX12_CONTEXT_COMMAND_PARAM(HRESULT, removerReason)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.onDeviceError(removerReason);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(OnSwapchainSwapCompleted)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.onSwapchainSwapCompleted();
#endif
DX12_END_CONTEXT_COMMAND

#endif

#if _TARGET_PC_WIN
DX12_BEGIN_CONTEXT_COMMAND(BeginTileMapping)
  DX12_CONTEXT_COMMAND_PARAM(Image *, tex)
  DX12_CONTEXT_COMMAND_PARAM(ID3D12Heap *, heap)
  DX12_CONTEXT_COMMAND_PARAM(size_t, heapBase)
  DX12_CONTEXT_COMMAND_PARAM(size_t, mappingCount)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginTileMapping(tex, heap, heapBase, mappingCount);
#endif

#else
DX12_BEGIN_CONTEXT_COMMAND(BeginTileMapping)
  DX12_CONTEXT_COMMAND_PARAM(Image *, tex)
  DX12_CONTEXT_COMMAND_PARAM(uintptr_t, address)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, size)
  DX12_CONTEXT_COMMAND_PARAM(size_t, mappingCount)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginTileMapping(tex, address, size, mappingCount);
#endif
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(AddTileMappings, TileMapping, mapping)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addTileMappings(mapping.data(), mapping.size());
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(EndTileMapping)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endTileMapping();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ActivateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRangeWithClearView, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceMemory, memory)
  DX12_CONTEXT_COMMAND_PARAM(ResourceActivationAction, action)
  DX12_CONTEXT_COMMAND_PARAM(ResourceClearValue, clearValue)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.activateBuffer(buffer, memory, action, clearValue, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ActivateTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(ResourceActivationAction, action)
  DX12_CONTEXT_COMMAND_PARAM(ResourceClearValue, clearValue)
  DX12_CONTEXT_COMMAND_PARAM(ImageViewState, clearViewState)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_CPU_DESCRIPTOR_HANDLE, clearViewDescriptor)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.activateTexture(texture, action, clearValue, clearViewState, clearViewDescriptor, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DeactivateBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndAddressRange, buffer)
  DX12_CONTEXT_COMMAND_PARAM(ResourceMemory, memory)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deactivateBuffer(buffer, memory, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DeactivateTexture)
  DX12_CONTEXT_COMMAND_PARAM(Image *, texture)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.deactivateTexture(texture, queue);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AliasFlush)
  DX12_CONTEXT_COMMAND_PARAM(GpuPipeline, queue)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.aliasFlush(queue);
#endif
DX12_END_CONTEXT_COMMAND


DX12_BEGIN_CONTEXT_COMMAND(TwoPhaseCopyBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReferenceAndOffset, source)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, destinationOffset)
  DX12_CONTEXT_COMMAND_PARAM(ScratchBuffer, scratchMemory)
  DX12_CONTEXT_COMMAND_PARAM(uint64_t, size)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.twoPhaseCopyBuffer(source, destinationOffset, scratchMemory, size);
#endif
DX12_END_CONTEXT_COMMAND


DX12_BEGIN_CONTEXT_COMMAND(TransitionBuffer)
  DX12_CONTEXT_COMMAND_PARAM(BufferResourceReference, buffer)
  DX12_CONTEXT_COMMAND_PARAM(D3D12_RESOURCE_STATES, state)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.transitionBuffer(buffer, state);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(ResizeImageMipMapTransfer)
  DX12_CONTEXT_COMMAND_PARAM(Image *, src)
  DX12_CONTEXT_COMMAND_PARAM(Image *, dst)
  DX12_CONTEXT_COMMAND_PARAM(MipMapRange, mipMapRange)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, srcMipMapOffset)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, dstMipMapOffset)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.resizeImageMipMapTransfer(src, dst, mipMapRange, srcMipMapOffset, dstMipMapOffset);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(DebugBreak)
#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.debugBreak();
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(AddDebugBreakString, char, str)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addDebugBreakString(str);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND_EXT_1(RemoveDebugBreakString, char, str)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeDebugBreakString(str);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(AddShaderGroup)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, group)
  DX12_CONTEXT_COMMAND_PARAM(ScriptedShadersBinDumpOwner *, dump)
  DX12_CONTEXT_COMMAND_PARAM(ShaderID, nullPixelShader)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addShaderGroup(group, dump, nullPixelShader);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(RemoveShaderGroup)
  DX12_CONTEXT_COMMAND_PARAM(uint32_t, group)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeShaderGroup(group);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(LoadComputeShaderFromDump)
  DX12_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.loadComputeShaderFromDump(program);
#endif
DX12_END_CONTEXT_COMMAND

DX12_BEGIN_CONTEXT_COMMAND(CompilePipelineSet)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<InputLayoutID>, inputLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<StaticRenderStateID>, staticRenderStates)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<FramebufferLayout>, framebufferLayouts)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<GraphicsPipelinePreloadInfo>, graphicsPipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<MeshPipelinePreloadInfo>, meshPipelines)
  DX12_CONTEXT_COMMAND_PARAM(eastl::span<ComputePipelinePreloadInfo>, computePipelines)

#if DX12_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.compilePipelineSet(DynamicArray<InputLayoutID>::fromSpan(inputLayouts),
    DynamicArray<StaticRenderStateID>::fromSpan(staticRenderStates), DynamicArray<FramebufferLayout>::fromSpan(framebufferLayouts),
    DynamicArray<GraphicsPipelinePreloadInfo>::fromSpan(graphicsPipelines),
    DynamicArray<MeshPipelinePreloadInfo>::fromSpan(meshPipelines),
    DynamicArray<ComputePipelinePreloadInfo>::fromSpan(computePipelines));
#endif
DX12_END_CONTEXT_COMMAND
