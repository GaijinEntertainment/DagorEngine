
VULKAN_BEGIN_CONTEXT_COMMAND(SetConstRegisterBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanBufferHandle, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, size)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  auto &resBinds = ctx.back.pipelineState.getStageResourceBinds(stage);
  resBinds.set_raw<StateFieldGlobalConstBuffer, StateFieldGlobalConstBuffer::BufferRangedRef>({buffer, offset, size});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetBlendConstantFactor)
  VULKAN_CONTEXT_COMMAND_PARAM(E3DCOLOR, constant)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsBlendConstantFactor, E3DCOLOR, FrontGraphicsState>(constant);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetDepthBoundsRange)
  VULKAN_CONTEXT_COMMAND_PARAM(float, from)
  VULKAN_CONTEXT_COMMAND_PARAM(float, to)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  using FieldName = StateFieldGraphicsDepthBounds;
  ctx.back.pipelineState.set_raw<FieldName, FieldName::DataType, FrontGraphicsState>({from, to});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetStencilRefOverride)
  VULKAN_CONTEXT_COMMAND_PARAM(uint16_t, ref)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(ref);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetPolygonLineEnable)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, enable)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsPolygonLine, bool, FrontGraphicsState>(enable);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetScissorRect)
  VULKAN_CONTEXT_COMMAND_PARAM(VkRect2D, rect)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsScissorRect, VkRect2D, FrontGraphicsState>(rect);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetInputLayout)
  VULKAN_CONTEXT_COMMAND_PARAM(InputLayoutID, layout)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(layout);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(NotifyBufferDiscard)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, oldBuf)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, newBuf)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, bufFlags)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.queueBufferDiscard(oldBuf, newBuf, bufFlags);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetIndexBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(VkIndexType, type)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(buffer);
  ctx.back.pipelineState.set_raw<StateFieldGraphicsIndexBuffer, VkIndexType, FrontGraphicsState>(type);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetVertexBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stream)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  using Bind = StateFieldGraphicsVertexBufferBind;

  Bind bind{buffer, offset};
  ctx.back.pipelineState.set_raw<StateFieldGraphicsVertexBuffers, Bind::Indexed, FrontGraphicsState>({stream, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetVertexStride)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stream)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  using Bind = StateFieldGraphicsVertexBufferStride;
  ctx.back.pipelineState.set_raw<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({stream, stride});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyBufferToImageOrdered)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanBufferHandle, buf)
  VULKAN_CONTEXT_COMMAND_PARAM(VkBufferImageCopy, region)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("orderedBufferToImageCopy");
  ctx.copyBufferToImageOrdered(image, buf, region);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CompileGraphicsPipeline)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // skip compilation of pipelines with subpass inputs, as we can't supply them using this algorithm
  ProgramID prog = ctx.back.pipelineState.getRO<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>();
  if (prog != ProgramID::Null())
  {
    auto &pipe = get_device().pipeMan.get<VariatedGraphicsPipeline>(prog);
    auto &regs = pipe.getLayout()->registers;
    if (regs.fs().header.inputAttachmentCount)
      return;
  }

  ctx.back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::GRAPHICS);
  ctx.flushGrahpicsState(top);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CompileComputePipeline)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  ctx.flushComputeState();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(Dispatch)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, x)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, y)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  ctx.dispatch(x, y, z);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DispatchIndirect)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  ctx.dispatchIndirect(buffer, offset);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawIndirect)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.drawIndirect(buffer, offset, count, stride);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawIndexedIndirect)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.drawIndexedIndirect(buffer, offset, count, stride);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(Draw)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, start)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.draw(count, instanceCount, start, firstInstance);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawUserData)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferSubAllocation, userData)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  uint32_t oldStride = ctx.beginVertexUserData(stride);
  ctx.flushGrahpicsState(top);
  ctx.bindVertexUserData(userData);
  ctx.draw(count, 1, 0, 0);
  ctx.endVertexUserData(oldStride);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawIndexed)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, indexStart)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(int32_t, vertexBase)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, firstInstance)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.drawIndexed(count, instanceCount, indexStart, vertexBase, firstInstance);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawIndexedUserData)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, vertexStride)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferSubAllocation, vertexData)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferSubAllocation, indexData)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.ensureActivePass();
  uint32_t oldStride = ctx.beginVertexUserData(vertexStride);
  ctx.flushGrahpicsState(top);
  ctx.bindVertexUserData(vertexData);
  ctx.bindIndexUser(indexData);
  ctx.drawIndexed(count, 1, 0, 0, 0);
  ctx.endVertexUserData(oldStride);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetComputeProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldComputeProgram, ProgramID, FrontComputeState>(program);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetGraphicsProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(program);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, source)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, dest)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, sourceOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, destOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, dataSize)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("CopyBuffer");
  ctx.copyBuffer(source.getHandle(), dest.getHandle(), source.dataOffset(sourceOffset), dest.dataOffset(destOffset), dataSize);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearBufferFloat)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(float, values, 4)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  union
  {
    float f;
    unsigned i;
  } fAsI;

  fAsI.f = values[0];
  // TODO: this is incorrect, but it works for now
  ctx.beginCustomStage("ClearBufferFloat");
  ctx.fillBuffer(buffer.getHandle(), buffer.dataOffset(0), buffer.dataSize(), fAsI.i);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearBufferInt)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(int32_t, values, 4)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO: this is incorrect, but it works for now
  ctx.beginCustomStage("ClearBufferInt");
  ctx.fillBuffer(buffer.getHandle(), buffer.dataOffset(0), buffer.dataSize(), values[0]);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(PushEvent)
  VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, index)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.pushEvent(index);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(PopEvent)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.popEvent();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(UpdateDebugUIPipelinesData)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  debug_ui_update_pipelines_data();
#endif
VULKAN_END_CONTEXT_COMMAND


#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT

VULKAN_BEGIN_CONTEXT_COMMAND(SetPipelineUsability)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, id)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, value)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  const bool ret = drv3d_vulkan::get_device().pipeMan.visit(id, [value](auto &pipeline) { pipeline.blockUsage(!value); });
  G_ASSERTF(ret, "Tried to block an invalid pipeline!");
#endif
VULKAN_END_CONTEXT_COMMAND

#endif // VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT


VULKAN_BEGIN_CONTEXT_COMMAND(SetViewport)
  VULKAN_CONTEXT_COMMAND_PARAM(ViewportState, viewport)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>(viewport);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearDepthStencilTexture)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(VkImageSubresourceRange, area)
  VULKAN_CONTEXT_COMMAND_PARAM(VkClearDepthStencilValue, value)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearDepthStencilImage(image, area, value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearColorTexture)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(VkImageSubresourceRange, area)
  VULKAN_CONTEXT_COMMAND_PARAM(VkClearColorValue, value)
  VULKAN_CONTEXT_COMMAND_PARAM(ImageState, followupUsage)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearColorImage(image, area, value, followupUsage);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, srcMip)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, dstMip)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, mipCount)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, regionCount)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, regionIndex)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO flush_clear_state is only needed if either image is a cleared attachment
  ctx.beginCustomStage("CopyImage");
  ctx.copyImage(src, dst, srcMip, dstMip, mipCount, regionCount, regionIndex);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(FlushDraws)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.flushDraws();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(BlitImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(VkImageBlit, region)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO flush_clear_state is only needed if either image is a cleared attachment
  ctx.beginCustomStage("BlitImage");
  ctx.blitImage(src, dst, region);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(BeginSurvey)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanQueryPoolHandle, pool)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("BeginSurvey");
  ctx.resetQuery(pool, index, 1);
  ctx.beginQuery(pool, index, 0);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(EndSurvey)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanQueryPoolHandle, pool)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.endQuery(pool, index);
  ctx.beginCustomStage("EndSurvey");
  ctx.copyQueryResult(pool, index, 1, buffer->getHandle(), buffer->dataOffset(index * sizeof(uint32_t)), sizeof(uint32_t),
    VK_QUERY_RESULT_WAIT_BIT);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RecordFrameTimings)
  VULKAN_CONTEXT_COMMAND_PARAM(Drv3dTimings *, timingData)
  VULKAN_CONTEXT_COMMAND_PARAM(uint64_t, kickoffTime)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.recordFrameTimings(timingData, kickoffTime);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(Present)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.present();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(InsertTimesampQuery)
  VULKAN_CONTEXT_COMMAND_PARAM(TimestampQueryRef, query)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeTimestamp(query);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(StartPreRotate)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, bindingSlot)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.startPreRotate(bindingSlot);
#endif

VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(HoldPreRotate)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.holdPreRotateStateForOneFrame();
#endif

VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(GenerateMipmaps)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO flush_clear_state is only needed if the image is a cleared attachment
  ctx.beginCustomStage("GenerateMipmaps");
  ctx.generateMipmaps(image);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ResolveMultiSampleImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("ResolveMultiSampleImage");
  ctx.resolveMultiSampleImage(src, dst);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(NativeRenderPassChanged)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.nativeRenderPassChanged();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearView)
  VULKAN_CONTEXT_COMMAND_PARAM(int, clearFlags)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearView(clearFlags);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetFramebufferAttachment)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(ImageViewState, view)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, useSwapchain)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  using Bind = StateFieldFramebufferAttachment;
  Bind bind{image, view, useSwapchain};
  ctx.back.pipelineState.set_raw<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
    {index, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetDepthStencilRWState)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetFramebufferSwapchainSrgbWrite)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldFramebufferSwapchainSrgbWrite, bool, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetFramebufferClearColor)
  VULKAN_CONTEXT_COMMAND_PARAM(E3DCOLOR, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldFramebufferClearColor, E3DCOLOR, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetFramebufferClearDepth)
  VULKAN_CONTEXT_COMMAND_PARAM(float, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldFramebufferClearDepth, float, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetFramebufferClearStencil)
  VULKAN_CONTEXT_COMMAND_PARAM(uint8_t, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldFramebufferClearStencil, uint8_t, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CheckAndSetAsyncCompletionState)
  VULKAN_CONTEXT_COMMAND_PARAM(AsyncCompletionState *, sync)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.checkAndSetAsyncCompletionState(sync);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddSyncEvent)
  VULKAN_CONTEXT_COMMAND_PARAM(AsyncCompletionState *, sync)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addSyncEvent(sync);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CaptureScreen)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("CaptureScreen");
  ctx.captureScreen(buffer);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetConditionalRender)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanBufferHandle, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(VkDeviceSize, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
    ConditionalRenderingState{buffer, offset});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddShaderModule)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleBlob *, sci)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addShaderModule(sci, id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RemoveShaderModule)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeShaderModule(id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddComputeProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleBlob *, sci)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleHeader, header)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addComputePipeline(program, sci, header);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddGraphicsProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, vs)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, fs)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, gs)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, tc)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, te)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addGraphicsPipeline(program, vs, fs, gs, tc, te);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(PlaceAftermathMarker)
  VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, stringIndex)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stringLength)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeToDebug(stringIndex);
#endif
VULKAN_END_CONTEXT_COMMAND


#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
VULKAN_BEGIN_CONTEXT_COMMAND(AttachComputeProgramDebugInfo)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderDebugInfo *, dbg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.attachComputePipelineDebugInfo(program, dbg);
#endif
VULKAN_END_CONTEXT_COMMAND

#endif

#if D3D_HAS_RAY_TRACING
VULKAN_BEGIN_CONTEXT_COMMAND(TraceRays)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, rayGenTable)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, missTable)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, hitTable)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, callableTable)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, rayGenOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, missOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, missStride)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, hitOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, hitStride)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, callableOffset)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, callableStride)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, width)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, height)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, depth)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.executionState.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::RAYTRACE);
  ctx.traceRays(rayGenTable, missTable, hitTable, callableTable, rayGenOffset, missOffset, missStride, hitOffset, hitStride,
    callableOffset, callableStride, width, height, depth);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRaytraceProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldRaytraceProgram, ProgramID, FrontRaytraceState>(program);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RaytraceBuildBottomAccelerationStructure)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, scratchBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, descIndex)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, descCount)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, update)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  ctx.beginCustomStage("RaytraceBuildBottomAccelerationStructure");
  ctx.buildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, 0, {}, 0, descCount, descIndex,
    ToVkBuildAccelerationStructureFlagsKHR(flags), update, as->getHandle(), {}, scratchBuffer);
#endif

#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RaytraceBuildTopAccelerationStructure)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, scratchBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, as)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, instanceBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, instanceCount)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceBuildFlags, flags)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, update)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  ctx.beginCustomStage("RaytraceBuildTopAccelerationStructure");
  ctx.buildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, instanceCount, instanceBuffer.buffer,
    instanceBuffer.dataOffset(0), 0, 0, ToVkBuildAccelerationStructureFlagsKHR(flags), update, as->getHandle(), {}, scratchBuffer);
#endif

#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddRaytraceProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(const ShaderModuleUse *, shaders)
  VULKAN_CONTEXT_COMMAND_PARAM(const RaytraceShaderGroup *, shaderGroups)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, shaderCount)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, groupCount)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, maxRecursion)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addRaytracePipeline(program, maxRecursion, shaderCount, shaders, groupCount, shaderGroups);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyRaytraceShaderGroupHandlesToMemory)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, firstGroup)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, groupCount)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, size)
  VULKAN_CONTEXT_COMMAND_PARAM(void *, ptr)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.copyRaytraceShaderGroupHandlesToMemory(program, firstGroup, groupCount, size, ptr);
#endif
VULKAN_END_CONTEXT_COMMAND

#endif

VULKAN_BEGIN_CONTEXT_COMMAND(ChangeSwapchainMode)
  VULKAN_CONTEXT_COMMAND_PARAM(SwapchainMode, newMode)
  VULKAN_CONTEXT_COMMAND_PARAM(ThreadedFence *, fence)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.changeSwapchainMode(newMode, fence);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(FlushAndWait)
  VULKAN_CONTEXT_COMMAND_PARAM(ThreadedFence *, userFence)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.flushAndWait(userFence);
#endif
VULKAN_END_CONTEXT_COMMAND


VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownSwapchain)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.shutdownSwapchain();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddPipelineCache)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanPipelineCacheHandle, cache)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addPipelineCache(cache);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddRenderState)
  VULKAN_CONTEXT_COMMAND_PARAM(shaders::DriverRenderStateId, id)
  VULKAN_CONTEXT_COMMAND_PARAM(shaders::RenderState, data)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.addRenderState(id, data);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRenderState)
  VULKAN_CONTEXT_COMMAND_PARAM(shaders::DriverRenderStateId, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldGraphicsRenderState, shaders::DriverRenderStateId, FrontGraphicsState>(id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RemoveProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.removeProgram(program);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  image->markDead();
  if (ctx.back.pipelineStatePendingCleanups.removeWithReferenceCheck(image, ctx.back.pipelineState))
    ctx.back.contextState.frame.get().cleanups.enqueueFromBackend<Image::CLEANUP_DESTROY>(*image);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyRenderPassResource)
  VULKAN_CONTEXT_COMMAND_PARAM(RenderPassResource *, rp)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  rp->markDead();
  if (ctx.back.pipelineStatePendingCleanups.removeWithReferenceCheck(rp, ctx.back.pipelineState))
    ctx.back.contextState.frame.get().cleanups.enqueueFromBackend<RenderPassResource::CLEANUP_DESTROY>(*rp);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  buffer->markDead();
  if (ctx.back.pipelineStatePendingCleanups.removeWithReferenceCheck(buffer, ctx.back.pipelineState))
    ctx.back.contextState.frame.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*buffer);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CleanupPendingReferences)
  VULKAN_CONTEXT_COMMAND_PARAM(PipelineState, state)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineStatePendingCleanups.cleanupAllNonUsed(state);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownPendingReferences)
  VULKAN_CONTEXT_COMMAND_PARAM(PipelineState, state)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineStatePendingCleanups.shutdown();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownImmediateConstBuffers)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  for (ImmediateConstBuffer &i : ctx.back.immediateConstBuffers)
    i.shutdown(ctx);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(WriteDebugMessage)
  VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, message_index)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, message_length)
  VULKAN_CONTEXT_COMMAND_PARAM(int, severity)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.writeDebugMessage(message_index, severity);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RegisterFrameEventsCallback)
  VULKAN_CONTEXT_COMMAND_PARAM(FrameEvents *, callback)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameEventsCallback(callback);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(TransitURegister)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(URegister, uReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  auto &resBinds = ctx.back.pipelineState.getStageResourceBinds(stage);
  resBinds.set_raw<StateFieldURegisterSet, StateFieldURegister::Indexed>({index, uReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(TransitTRegister)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(TRegister, tReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  auto &resBinds = ctx.back.pipelineState.getStageResourceBinds(stage);
  resBinds.set_raw<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({index, tReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(TransitBRegister)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, bReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  auto &resBinds = ctx.back.pipelineState.getStageResourceBinds(stage);
  resBinds.set_raw<StateFieldBRegisterSet, StateFieldBRegister::Indexed>({index, bReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetImmediateConsts)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, enable)
  VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, data, MAX_IMMEDIATE_CONST_WORDS)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  auto &resBinds = ctx.back.pipelineState.getStageResourceBinds(stage);
  resBinds.set_raw<StateFieldImmediateConst, StateFieldImmediateConst::SrcData>(
    {(uint8_t)(enable ? MAX_IMMEDIATE_CONST_WORDS : 0), data});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ImageBarrier)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, img)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, image_type)
  VULKAN_CONTEXT_COMMAND_PARAM(ResourceBarrier, state)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, res_index)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, res_range)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.imageBarrier(img, ExecutionContext::BarrierImageType(image_type), state, res_index, res_range);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRenderPassTarget)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, mipLevel)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, layer)
  VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, clearValueArr, 4)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  using Bind = StateFieldRenderPassTarget;

  ResourceClearValue rcv;
  rcv.asUint[0] = clearValueArr[0];
  rcv.asUint[1] = clearValueArr[1];
  rcv.asUint[2] = clearValueArr[2];
  rcv.asUint[3] = clearValueArr[3];

  Bind bind{image, mipLevel, layer, rcv};
  ctx.back.pipelineState.set_raw<StateFieldRenderPassTargets, Bind::Indexed, FrontGraphicsState, FrontRenderPassState>({index, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRenderPassResource)
  VULKAN_CONTEXT_COMMAND_PARAM(RenderPassResource *, ptr)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(ptr);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRenderPassSubpassIdx)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>(index);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetRenderPassArea)
  VULKAN_CONTEXT_COMMAND_PARAM(RenderPassArea, data)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineState.set_raw<StateFieldRenderPassArea, RenderPassArea, FrontGraphicsState, FrontRenderPassState>(data);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(GetWorkerCpuCore)
  VULKAN_CONTEXT_COMMAND_PARAM(int *, core)
  VULKAN_CONTEXT_COMMAND_PARAM(int *, thread_id)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.getWorkerCpuCore(core, thread_id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(SetSwappyTargetRate)
  VULKAN_CONTEXT_COMMAND_PARAM(int, rate)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.setSwappyTargetRate(rate);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(GetSwappyStatus)
  VULKAN_CONTEXT_COMMAND_PARAM(int *, status)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.getSwappyStatus(status);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(PipelineCompilationTimeBudget)
  VULKAN_CONTEXT_COMMAND_PARAM(unsigned, usecs)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.pipelineCompilationTimeBudget = usecs;
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(UpdateBindlessTexture)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
  VULKAN_CONTEXT_COMMAND_PARAM(ImageViewState, view)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.contextState.bindlessManagerBackend.updateBindlessTexture(index, image, view);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(UpdateBindlessSampler)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(SamplerInfo *, samplerInfo)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.contextState.bindlessManagerBackend.updateBindlessSampler(index, samplerInfo);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyBindlessTextureDescriptors)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, src)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.back.contextState.bindlessManagerBackend.copyBindlessTextureDescriptors(src, dst, count);
#endif
VULKAN_END_CONTEXT_COMMAND
