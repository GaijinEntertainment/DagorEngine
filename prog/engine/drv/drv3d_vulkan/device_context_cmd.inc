
VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetConstRegisterBuffer)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldGlobalConstBuffer, BufferRef>(buffer);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetBlendConstantFactor)
VULKAN_CONTEXT_COMMAND_PARAM(E3DCOLOR, constant)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsBlendConstantFactor, E3DCOLOR, FrontGraphicsState>(constant);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetDepthBoundsRange)
VULKAN_CONTEXT_COMMAND_PARAM(float, from)
VULKAN_CONTEXT_COMMAND_PARAM(float, to)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
using FieldName = StateFieldGraphicsDepthBounds;
Backend::State::pipe.set_raw<FieldName, FieldName::DataType, FrontGraphicsState>({from, to});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetStencilRefOverride)
VULKAN_CONTEXT_COMMAND_PARAM(uint16_t, ref)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(ref);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetPolygonLineEnable)
VULKAN_CONTEXT_COMMAND_PARAM(bool, enable)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsPolygonLine, bool, FrontGraphicsState>(enable);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetScissorRect)
VULKAN_CONTEXT_COMMAND_PARAM(VkRect2D, rect)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsScissorRect, VkRect2D, FrontGraphicsState>(rect);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetInputLayout)
VULKAN_CONTEXT_COMMAND_PARAM(InputLayoutID, layout)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(layout);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(NotifyBufferDiscard)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, oldBuf)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, newBuf)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, bufFlags)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
ctx.queueBufferDiscard(oldBuf, newBuf, bufFlags);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(AddRefFrameMem)
VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buf)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
buf->addFrameMemRef();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(ReleaseFrameMem)
VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buf)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
buf->removeFrameMemRef();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetIndexBuffer)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
VULKAN_CONTEXT_COMMAND_PARAM(VkIndexType, type)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(buffer);
Backend::State::pipe.set_raw<StateFieldGraphicsIndexBuffer, VkIndexType, FrontGraphicsState>(type);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetVertexBuffer)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stream)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
using Bind = StateFieldGraphicsVertexBufferBind;

Bind bind{buffer, offset};
Backend::State::pipe.set_raw<StateFieldGraphicsVertexBuffers, Bind::Indexed, FrontGraphicsState>({stream, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetVertexStride)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stream)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
using Bind = StateFieldGraphicsVertexBufferStride;
Backend::State::pipe.set_raw<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({stream, stride});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyBufferToImageOrdered)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(VkBufferImageCopy, region)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("orderedBufferToImageCopy");
  ctx.verifyResident(dst);
  ctx.verifyResident(src);

  const auto sz =
    dst->getFormat().calculateImageSize(region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth, 1);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src, {region.bufferOffset, sz});
  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    {region.imageSubresource.mipLevel, 1, region.imageSubresource.baseArrayLayer, region.imageSubresource.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(
    Backend::cb.wCmdCopyBufferToImage(src->getHandle(), dst->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CompileGraphicsPipeline)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // skip compilation of pipelines with subpass inputs, as we can't supply them using this algorithm
  ProgramID prog = Backend::State::pipe.getRO<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>();
  if (prog != ProgramID::Null())
  {
    auto &pipe = Globals::pipelines.get<VariatedGraphicsPipeline>(prog);
    auto &regs = pipe.getLayout()->registers;
    if (regs.fs().header.inputAttachmentCount)
      return;
  }

  Backend::State::pod.nonDrawCompile = true;
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::GRAPHICS);
  ctx.flushGrahpicsState(top);
  Backend::State::pod.nonDrawCompile = false;
  ctx.invalidateActiveGraphicsPipeline();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CompileComputePipeline)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
// nothing to do
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(Dispatch)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, x)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, y)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, z)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.startNode(ExecutionNode::CS);
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  ctx.dispatch(x, y, z);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DispatchIndirect)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.startNode(ExecutionNode::CS);
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
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
  ctx.startNode(ExecutionNode::PS);
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
  ctx.startNode(ExecutionNode::PS);
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
  ctx.startNode(ExecutionNode::PS);
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.draw(count, instanceCount, start, firstInstance);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawUserData)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stride)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, userData)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.startNode(ExecutionNode::PS);
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
  ctx.startNode(ExecutionNode::PS);
  ctx.ensureActivePass();
  ctx.flushGrahpicsState(top);
  ctx.drawIndexed(count, instanceCount, indexStart, vertexBase, firstInstance);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DrawIndexedUserData)
  VULKAN_CONTEXT_COMMAND_PARAM(VkPrimitiveTopology, top)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, vertexStride)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, vertexData)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, indexData)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.startNode(ExecutionNode::PS);
  ctx.ensureActivePass();
  uint32_t oldStride = ctx.beginVertexUserData(vertexStride);
  ctx.flushGrahpicsState(top);
  ctx.bindVertexUserData(vertexData);

  Backend::cb.wCmdBindIndexBuffer(indexData.getHandle(), indexData.bufOffset(0), VK_INDEX_TYPE_UINT16);
  // set state dirty to restore after this intervention
  Backend::State::exec.set<StateFieldGraphicsIndexBuffer, uint32_t, BackGraphicsState>(1);

  ctx.drawIndexed(count, 1, 0, 0, 0);
  ctx.endVertexUserData(oldStride);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetComputeProgram)
VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldComputeProgram, ProgramID, FrontComputeState>(program);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetGraphicsProgram)
VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(program);
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
  ctx.verifyResident(source.buffer);
  ctx.verifyResident(dest.buffer);

  uint32_t sourceBufOffset = source.bufOffset(sourceOffset);
  uint32_t destBufOffset = dest.bufOffset(destOffset);

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, source.buffer,
    {sourceBufOffset, dataSize});
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dest.buffer,
    {destBufOffset, dataSize});
  Backend::sync.completeNeeded();

  VkBufferCopy bc;
  bc.srcOffset = sourceBufOffset;
  bc.dstOffset = destBufOffset;
  bc.size = dataSize;
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(source.buffer->getHandle(), dest.buffer->getHandle(), 1, &bc));
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
  ctx.fillBuffer(buffer.buffer, buffer.bufOffset(0), buffer.visibleDataSize, fAsI.i);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ClearBufferInt)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
  VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(int32_t, values, 4)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO: this is incorrect, but it works for now
  ctx.beginCustomStage("ClearBufferInt");
  ctx.fillBuffer(buffer.buffer, buffer.bufOffset(0), buffer.visibleDataSize, values[0]);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(PushEvent)
VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, index)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
ctx.pushEvent(index);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(PopEvent)

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
  bool valueCopy = value;
  const bool ret = drv3d_vulkan::Globals::pipelines.visit(id, [valueCopy](auto &pipeline) { pipeline.blockUsage(!valueCopy); });
  G_ASSERTF(ret, "Tried to block an invalid pipeline!");
#endif
VULKAN_END_CONTEXT_COMMAND

#endif // VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT


VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetViewport)
VULKAN_CONTEXT_COMMAND_PARAM(ViewportState, viewport)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>(viewport);
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

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.clearColorImage(image, area, value);
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

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(FlushDraws)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::profilerStack.finish(); // ends frame core

TIME_PROFILE(vulkan_cmdFlushDraws)
FrameInfo &frame = Backend::gpuJob.get();
ctx.flush(frame.frameDone);
Globals::timelines.get<TimelineManager::GpuExecute>().advance();
Backend::gpuJob.restart();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(BlitImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(VkImageBlit, region)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, whole_subres)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // TODO flush_clear_state is only needed if either image is a cleared attachment
  ctx.beginCustomStage("BlitImage");
  G_ASSERTF(src != dst, "Don't blit image on it self, if you need to build mipmaps, use the "
                        "dedicated functions for it!");
  ValueRange<uint8_t> srcMipRange(region.srcSubresource.mipLevel, region.srcSubresource.mipLevel + 1);
  ValueRange<uint8_t> dstMipRange(region.dstSubresource.mipLevel, region.dstSubresource.mipLevel + 1);
  ValueRange<uint16_t> srcArrayRange(region.srcSubresource.baseArrayLayer,
    region.srcSubresource.baseArrayLayer + region.srcSubresource.layerCount);
  ValueRange<uint16_t> dstArrayRange(region.dstSubresource.baseArrayLayer,
    region.dstSubresource.baseArrayLayer + region.dstSubresource.layerCount);

  auto srcI = src;
  auto dstI = dst;

  if (!srcI)
    srcI = ctx.swapchain.getColorImage();
  if (!dstI)
    dstI = ctx.swapchain.getColorImage();

  ctx.verifyResident(srcI);
  ctx.verifyResident(dstI);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, srcI,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    {region.srcSubresource.mipLevel, 1, region.srcSubresource.baseArrayLayer, region.srcSubresource.layerCount});

  if (whole_subres)
    Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dstI,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {region.dstSubresource.mipLevel, 1, region.dstSubresource.baseArrayLayer, region.dstSubresource.layerCount});
  else
    Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dstI,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {region.dstSubresource.mipLevel, 1, region.dstSubresource.baseArrayLayer, region.dstSubresource.layerCount});
  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(srcI->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstI->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR));
  ctx.writeExectionChekpoint(VK_PIPELINE_STAGE_TRANSFER_BIT);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(BeginSurvey)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanQueryPoolHandle, pool)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("BeginSurvey");
  VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(pool, index, 1));
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(EndSurvey)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanQueryPoolHandle, pool)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("EndSurvey");
  ctx.copyQueryResult(pool, index, 1, buffer);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetGraphicsQuery)
VULKAN_CONTEXT_COMMAND_PARAM(VulkanQueryPoolHandle, pool)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({pool, index});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RecordFrameTimings)
  VULKAN_CONTEXT_COMMAND_PARAM(Drv3dTimings *, timingData)
  VULKAN_CONTEXT_COMMAND_PARAM(uint64_t, kickoffTime)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // profile ref ticks can be inconsistent between threads, skip such data
  uint64_t now = profile_ref_ticks();
  timingData->frontendToBackendUpdateScreenLatency = (int64_t)(now > kickoffTime ? now - kickoffTime : 0);

  timingData->gpuWaitDuration = Backend::timings.gpuWaitDuration;
  timingData->backendFrontendWaitDuration = Backend::timings.workWaitDuration;
  timingData->backbufferAcquireDuration = Backend::timings.acquireBackBufferDuration;
  // time blocked in present are counted from last frame
  timingData->presentDuration = Backend::timings.presentWaitDuration;
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(Present)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::profilerStack.finish(); // ends frame core
TIME_PROFILE(vulkan_cmdPresent)
ctx.present();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(InsertTimesampQuery)
VULKAN_CONTEXT_COMMAND_PARAM(TimestampQueryIndex, queryIndex)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
TimestampQueryBlock &qb = *ctx.data.timestampQueryBlock;

if (ctx.frameCoreQueue != DeviceQueueType::GRAPHICS)
{
  qb.dataIndexes.push_back(qb.data.size() - 1);
  return;
}

if (ctx.actionIdx > ctx.lastTimestampActionIdx || qb.data.empty())
{
  VULKAN_LOG_CALL(Backend::cb.wCmdWriteTimestamp(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, qb.pool, qb.data.size()));
  qb.data.push_back(0);
}
qb.dataIndexes.push_back(qb.data.size() - 1);
ctx.lastTimestampActionIdx = ctx.actionIdx;
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(StartPreRotate)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, bindingSlot)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.swapchain.preRotateStart(bindingSlot, Backend::gpuJob.get(), ctx);
#endif

VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(HoldPreRotate)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.swapchain.holdPreRotatedStateForOneFrame();
#endif

VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(GenerateMipmaps)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, img)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("GenerateMipmaps");
  ctx.verifyResident(img);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, img,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, img->getArrayLayers()});
  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {1, 1, 0, img->getArrayLayers()});
  Backend::sync.completeNeeded();

  // blit mip 0 into mip 1
  VkImageBlit blit;
  blit.srcSubresource.aspectMask = img->getFormat().getAspektFlags();
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = img->getArrayLayers();
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1].x = img->getBaseExtent().width;
  blit.srcOffsets[1].y = img->getBaseExtent().height;
  blit.srcOffsets[1].z = 1;
  blit.dstSubresource.aspectMask = img->getFormat().getAspektFlags();
  blit.dstSubresource.mipLevel = 1;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = img->getArrayLayers();
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  auto mipExtent = img->getMipExtents2D(1);
  blit.dstOffsets[1].x = mipExtent.width;
  blit.dstOffsets[1].y = mipExtent.height;
  blit.dstOffsets[1].z = 1;

  VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR));

  if (img->getMipLevels() > 2)
  {
    auto lastMipExtent = mipExtent;
    for (auto &&m : img->getMipLevelRange().front(2))
    {
      Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {(uint32_t)(m - 1), 1, 0, img->getArrayLayers()});
      Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, img,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {(uint32_t)m, 1, 0, img->getArrayLayers()});
      Backend::sync.completeNeeded();

      // blit mip M-1 into mip M
      blit.srcSubresource.mipLevel = m - 1;
      blit.srcOffsets[1].x = lastMipExtent.width;
      blit.srcOffsets[1].y = lastMipExtent.height;

      auto mipExtent = img->getMipExtents2D(m);
      blit.dstSubresource.mipLevel = m;
      blit.dstOffsets[1].x = mipExtent.width;
      blit.dstOffsets[1].y = mipExtent.height;
      VULKAN_LOG_CALL(Backend::cb.wCmdBlitImage(img->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->getHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR));

      lastMipExtent = mipExtent;
    }
  }
  ctx.writeExectionChekpoint(VK_PIPELINE_STAGE_TRANSFER_BIT);

  // we can't know is it readed or not, so assume readed
  if (img->isUsedInBindless())
  {
    ctx.trackBindlessRead(img);
    Backend::sync.completeNeeded();
  }
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ResolveMultiSampleImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, dst)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("ResolveMultiSampleImage");
  VkImageResolve area = {};
  area.srcSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.srcSubresource.layerCount = 1;
  area.dstSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
  area.dstSubresource.layerCount = 1;
  area.extent = src->getBaseExtent();

  ctx.verifyResident(src);
  ctx.verifyResident(dst);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, src,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  Backend::sync.addImageWriteDiscard({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, dst,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {0, 1, 0, 1});

  Backend::sync.completeNeeded();

  VULKAN_LOG_CALL(Backend::cb.wCmdResolveImage(src->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->getHandle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &area));
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

VULKAN_BEGIN_CONTEXT_COMMAND(AllowOpLoad)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.getFramebufferState().clearMode |= FramebufferState::CLEAR_LOAD;
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetFramebufferAttachment)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
VULKAN_CONTEXT_COMMAND_PARAM(ImageViewState, view)
VULKAN_CONTEXT_COMMAND_PARAM(bool, useSwapchain)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
using Bind = StateFieldFramebufferAttachment;
Bind bind{image, view, useSwapchain};
Backend::State::pipe.set_raw<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
  {index, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetDepthStencilRWState)
VULKAN_CONTEXT_COMMAND_PARAM(bool, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldFramebufferReadOnlyDepth, bool, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetFramebufferSwapchainSrgbWrite)
VULKAN_CONTEXT_COMMAND_PARAM(bool, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldFramebufferSwapchainSrgbWrite, bool, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetFramebufferClearColor)
VULKAN_CONTEXT_COMMAND_PARAM(E3DCOLOR, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldFramebufferClearColor, E3DCOLOR, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetFramebufferClearDepth)
VULKAN_CONTEXT_COMMAND_PARAM(float, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldFramebufferClearDepth, float, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetFramebufferClearStencil)
VULKAN_CONTEXT_COMMAND_PARAM(uint8_t, value)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldFramebufferClearStencil, uint8_t, FrontGraphicsState, FrontFramebufferState>(value);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CaptureScreen)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("CaptureScreen");
  auto colorImage = ctx.swapchain.getColorImage();

  // do the copy to buffer
  auto extent = colorImage->getMipExtents2D(0);
  VkBufferImageCopy copy{};
  copy.bufferOffset = buffer->bufOffsetLoc(0);
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.layerCount = 1;
  copy.imageExtent.width = extent.width;
  copy.imageExtent.height = extent.height;
  copy.imageExtent.depth = 1;
  const auto copySize = colorImage->getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, 1, 1);

  ctx.verifyResident(colorImage);
  ctx.verifyResident(buffer);

  Backend::sync.addImageAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, colorImage,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {0, 1, 0, 1});
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buffer, {copy.bufferOffset, copySize});

  Backend::sync.completeNeeded();
  VULKAN_LOG_CALL(
    Backend::cb.wCmdCopyImageToBuffer(colorImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->getHandle(), 1, &copy));
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetConditionalRender)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, buffer)
VULKAN_CONTEXT_COMMAND_PARAM(VkDeviceSize, offset)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
  ConditionalRenderingState{buffer, offset});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddShaderModule)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleBlob *, sci)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::shaderModules.add(sci, id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RemoveShaderModule)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::shaderModules.remove(id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddComputeProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleBlob *, sci)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderModuleHeader, header)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Globals::pipelines.addCompute(ctx.vkDev, Globals::pipeCache.getHandle(), program, *sci, header);
  delete sci;
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
  struct ShaderModules
  {
    ShaderModule *module = nullptr;
    ShaderModuleHeader *header = nullptr;
    ShaderModuleUse *use = nullptr;
  };

  auto getShaderModules = [&ctx](uint32_t index) {
    ShaderModules modules;
    if (index < ctx.data.shaderModuleUses.size())
    {
      modules.use = &ctx.data.shaderModuleUses[index];
      modules.header = &modules.use->header;

      modules.module = Backend::shaderModules.find(modules.use->blobId);
    }
    return modules;
  };

  ShaderModules vsModules = getShaderModules(vs);
  ShaderModules fsModules = getShaderModules(fs);
  ShaderModules tcModules = getShaderModules(tc);
  ShaderModules teModules = getShaderModules(te);
  ShaderModules gsModules = getShaderModules(gs);

  Globals::pipelines.addGraphics(ctx.vkDev, program, *vsModules.module, *vsModules.header, *fsModules.module, *fsModules.header,
    gsModules.module, gsModules.header, tcModules.module, tcModules.header, teModules.module, teModules.header);

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  Globals::pipelines.get<VariatedGraphicsPipeline>(program).setDebugInfo(
    {{vsModules.use->dbg, fsModules.use->dbg, gsModules.use ? gsModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      tcModules.use ? tcModules.use->dbg : GraphicsPipeline::emptyDebugInfo,
      teModules.use ? teModules.use->dbg : GraphicsPipeline::emptyDebugInfo}});
#endif
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(PlaceAftermathMarker)
  VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, stringIndex)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, stringLength)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if VK_EXT_debug_utils
  if (ctx.vkDev.getInstance().hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsLabelEXT dul = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    dul.pLabelName = ctx.data.charStore.data() + stringIndex.get();
    dul.color[0] = 1.f;
    dul.color[1] = 1.f;
    dul.color[2] = 1.f;
    dul.color[3] = 1.f;
    VULKAN_LOG_CALL(Backend::cb.wCmdInsertDebugUtilsLabelEXT(&dul));
  }
#endif
#endif
VULKAN_END_CONTEXT_COMMAND


#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
VULKAN_BEGIN_CONTEXT_COMMAND(AttachComputeProgramDebugInfo)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
  VULKAN_CONTEXT_COMMAND_PARAM(ShaderDebugInfo *, dbg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Globals::pipelines.get<ComputePipeline>(program).setDebugInfo({{*dbg}});
  delete dbg;
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
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::RAYTRACE);
  G_ASSERTF(false, "CmdTraceRays called on API without support");
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RaytraceBuildStructures)
  VULKAN_CONTEXT_COMMAND_PARAM(size_t, index)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  ctx.beginCustomStage("RaytraceBuildAccelerationStructures");
  ctx.buildAccelerationStructures(&ctx.data.raytraceStructureBuildStore[index], count);
#endif

#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyRaytraceAccelerationStructure)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, src)
  VULKAN_CONTEXT_COMMAND_PARAM(RaytraceAccelerationStructure *, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, compact)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  ctx.beginCustomStage("RaytraceCopyAccelerationStructure");
  Backend::sync.addAccelerationStructureAccess(
    {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR}, src);
  Backend::sync.addAccelerationStructureAccess(
    {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR}, dst);
  Backend::sync.completeNeeded();

  VkCopyAccelerationStructureInfoKHR ci{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR, nullptr};
  ci.mode = compact ? VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR : VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;
  ci.src = src->getHandle();
  ci.dst = dst->getHandle();
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyAccelerationStructureKHR(&ci));
#endif

#endif
VULKAN_END_CONTEXT_COMMAND

#endif

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(ChangeSwapchainMode)
VULKAN_CONTEXT_COMMAND_PARAM(SwapchainMode, newMode)
VULKAN_CONTEXT_COMMAND_PARAM(ThreadedFence *, fence)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::profilerStack.finish(); // ends frame core
TIME_PROFILE(vulkan_cmdChangeSwapchainMode);
ctx.flushAndWait(nullptr);
ctx.swapchain.changeSwapchainMode(ctx, newMode);
fence->setSignaledExternally();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(FlushAndWait)
VULKAN_CONTEXT_COMMAND_PARAM(ThreadedFence *, userFence)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::profilerStack.finish(); // ends frame core
TIME_PROFILE(vulkan_cmdFlushAndWait);
ctx.flushAndWait(userFence);
#endif
VULKAN_END_CONTEXT_COMMAND


VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownSwapchain)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.swapchain.shutdown(Backend::gpuJob.get());
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddPipelineCache)
  VULKAN_CONTEXT_COMMAND_PARAM(VulkanPipelineCacheHandle, cache)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (VULKAN_CHECK_OK(ctx.vkDev.vkMergePipelineCaches(ctx.vkDev.get(), Globals::pipeCache.getHandle(), 1, ptr(cache))))
    debug("vulkan: additional pipeline cache added");

  VULKAN_LOG_CALL(ctx.vkDev.vkDestroyPipelineCache(ctx.vkDev.get(), cache, NULL));
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AddRenderState)
  VULKAN_CONTEXT_COMMAND_PARAM(shaders::DriverRenderStateId, id)
  VULKAN_CONTEXT_COMMAND_PARAM(shaders::RenderState, data)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::renderStateSystem.setRenderStateData(id, data);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderState)
VULKAN_CONTEXT_COMMAND_PARAM(shaders::DriverRenderStateId, id)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldGraphicsRenderState, shaders::DriverRenderStateId, FrontGraphicsState>(id);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RemoveProgram)
  VULKAN_CONTEXT_COMMAND_PARAM(ProgramID, program)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (Backend::State::pendingCleanups.removeWithReferenceCheck(program, Backend::State::pipe))
  {
    Globals::pipelines.prepareRemoval(program);
    Globals::shaderProgramDatabase.reuseId(program);
  }
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyImage)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, image)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  image->markDead();
  Backend::gpuJob.get().cleanups.enqueueFromBackend<Image::CLEANUP_DESTROY>(*image);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyRenderPassResource)
  VULKAN_CONTEXT_COMMAND_PARAM(RenderPassResource *, rp)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  rp->markDead();
  if (Backend::State::pendingCleanups.removeWithReferenceCheck(rp, Backend::State::pipe))
    Backend::gpuJob.get().cleanups.enqueueFromBackend<RenderPassResource::CLEANUP_DESTROY>(*rp);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyBuffer)
  VULKAN_CONTEXT_COMMAND_PARAM(Buffer *, buffer)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  buffer->markDead();
  if (Backend::State::pendingCleanups.removeWithReferenceCheck(buffer, Backend::State::pipe))
    Backend::gpuJob.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*buffer);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DestroyHeap)
  VULKAN_CONTEXT_COMMAND_PARAM(MemoryHeapResource *, heap)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  heap->markDead();
  Backend::gpuJob.get().cleanups.enqueueFromBackend<MemoryHeapResource::CLEANUP_DESTROY>(*heap);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CleanupPendingReferences)
  VULKAN_CONTEXT_COMMAND_PARAM(PipelineState, state)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  // apply queued discards before syncing state and scrapping resources
  // otherwise chain of discards may be corrupted
  // by removal of buffer in its tail
  ctx.applyQueuedDiscards();
  Backend::State::pendingCleanups.cleanupAllNonUsed(state, Backend::State::pipe);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownPendingReferences)
  VULKAN_CONTEXT_COMMAND_PARAM(PipelineState, state)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::State::pendingCleanups.shutdown();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ShutdownImmediateConstBuffers)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::immediateConstBuffers.shutdown();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(WriteDebugMessage)
  VULKAN_CONTEXT_COMMAND_PARAM(StringIndexRef, message_index)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, message_length)
  VULKAN_CONTEXT_COMMAND_PARAM(int, severity)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (severity < 1)
    debug("%s", ctx.data.charStore.data() + message_index.get());
  else if (severity < 2)
    logwarn("%s", ctx.data.charStore.data() + message_index.get());
  else
    D3D_ERROR("%s", ctx.data.charStore.data() + message_index.get());
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ExecuteFSR)
  VULKAN_CONTEXT_COMMAND_PARAM(amd::FSR *, fsr)
  VULKAN_CONTEXT_COMMAND_PARAM(FSRUpscalingArgs, params)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.beginCustomStage("executeFSR");
  ctx.executeFSR(fsr, params);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(RegisterFrameEventsCallback)
  VULKAN_CONTEXT_COMMAND_PARAM(FrameEvents *, callback)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.registerFrameEventsCallback(callback);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(TransitURegister)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
VULKAN_CONTEXT_COMMAND_PARAM(URegister, uReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldURegisterSet, StateFieldURegister::Indexed>({index, uReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(TransitTRegister)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
VULKAN_CONTEXT_COMMAND_PARAM(TRegister, tReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({index, tReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(TransitBRegister)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, bReg)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldBRegisterSet, StateFieldBRegister::Indexed>({index, bReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(TransitSRegister)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)
VULKAN_CONTEXT_COMMAND_PARAM(SRegister, sReg)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldSRegisterSet, StateFieldSRegister::Indexed>({index, sReg});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetImmediateConsts)
VULKAN_CONTEXT_COMMAND_PARAM(ShaderStage, stage)
VULKAN_CONTEXT_COMMAND_PARAM(bool, enable)
VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(uint32_t, data, MAX_IMMEDIATE_CONST_WORDS)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
auto &resBinds = Backend::State::pipe.getStageResourceBinds(stage);
resBinds.set_raw<StateFieldImmediateConst, StateFieldImmediateConst::SrcData>(
  {(uint8_t)(enable ? MAX_IMMEDIATE_CONST_WORDS : 0), data});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(ImageBarrier)
  VULKAN_CONTEXT_COMMAND_PARAM(Image *, img)
  VULKAN_CONTEXT_COMMAND_PARAM(ResourceBarrier, state)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, res_index)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, res_range)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.imageBarrier(img, state, res_index, res_range);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(BufferBarrier)
  VULKAN_CONTEXT_COMMAND_PARAM(BufferRef, bRef)
  VULKAN_CONTEXT_COMMAND_PARAM(ResourceBarrier, state)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  ctx.bufferBarrier(bRef, state);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderPassTarget)
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
Backend::State::pipe.set_raw<StateFieldRenderPassTargets, Bind::Indexed, FrontGraphicsState, FrontRenderPassState>({index, bind});
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderPassResource)
VULKAN_CONTEXT_COMMAND_PARAM(RenderPassResource *, ptr)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>(ptr);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderPassIndex)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldRenderPassIndex, uint32_t, FrontGraphicsState, FrontRenderPassState>(index);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderPassSubpassIdx)
VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, index)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>(index);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(SetRenderPassArea)
VULKAN_CONTEXT_COMMAND_PARAM(RenderPassArea, data)

#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
Backend::State::pipe.set_raw<StateFieldRenderPassArea, RenderPassArea, FrontGraphicsState, FrontRenderPassState>(data);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(GetWorkerCpuCore)
  VULKAN_CONTEXT_COMMAND_PARAM(int *, core)
  VULKAN_CONTEXT_COMMAND_PARAM(int *, threadId)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
#if _TARGET_ANDROID
  *core = sched_getcpu();
  *threadId = gettid();
#endif
  G_UNUSED(core);
  G_UNUSED(threadId);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(PipelineCompilationTimeBudget)
VULKAN_CONTEXT_COMMAND_PARAM(PipelineManager::AsyncMask, mask)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
drv3d_vulkan::Globals::pipelines.setAsyncCompile(mask);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(CopyBindlessDescriptors)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, resourceType)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, src)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, dst)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, count)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::bindless.copyBindlessDescriptors(resourceType, src, dst, count);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(AsyncPipeFeedbackPtr)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t *, feedbackPtr)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::State::pod.asyncPipelineCompileFeedbackPtr = feedbackPtr;
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(UpdateAliasedMemoryInfo)
  VULKAN_CONTEXT_COMMAND_PARAM(ResourceMemoryId, id)
  VULKAN_CONTEXT_COMMAND_PARAM(AliasedResourceMemory, info)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  Backend::aliasedMemory.update(id, info);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(DelaySyncCompletion)
  VULKAN_CONTEXT_COMMAND_PARAM(bool, enable)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (enable)
  {
    ctx.beginCustomStage("DelayedSyncStart");
    Backend::sync.setCompletionDelay(enable);
    Backend::cb.startReorder();
  }
  else
  {
    Backend::sync.setCompletionDelay(enable);
    ctx.finishReorderAndPerformSync();
  }
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(QueueSwitch)
  VULKAN_CONTEXT_COMMAND_PARAM(int, queue)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (!Globals::cfg.bits.allowMultiQueue)
    return;
  if ((int)ctx.frameCoreQueue == queue)
    return;

  VulkanCommandBufferHandle frameCoreOld = ctx.frameCore;
  ctx.beginCustomStage("QueueSwitch");
  G_UNUSED(frameCoreOld);
  G_ASSERTF(frameCoreOld == ctx.frameCore, "vulkan: should have not interrupted frame core here!");
  ctx.switchFrameCoreForQueueChange((DeviceQueueType)queue);
  ctx.onFrameCoreReset();
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(QueueSignal)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, signalIdx)
  VULKAN_CONTEXT_COMMAND_PARAM(int, queue)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (!Globals::cfg.bits.allowMultiQueue)
    return;
  ctx.recordUserQueueSignal(signalIdx, (DeviceQueueType)queue);
#endif
VULKAN_END_CONTEXT_COMMAND

VULKAN_BEGIN_CONTEXT_COMMAND(QueueWait)
  VULKAN_CONTEXT_COMMAND_PARAM(uint32_t, signalIdx)
  VULKAN_CONTEXT_COMMAND_PARAM(int, queue)
#if VULKAN_CONTEXT_COMMAND_IMPLEMENTATION
  if (!Globals::cfg.bits.allowMultiQueue)
    return;

  // interrupt command buffer on wait, to avoid waiting for previous work together with next work
  VulkanCommandBufferHandle frameCoreOld = ctx.frameCore;
  ctx.beginCustomStage("QueueWait");
  if (frameCoreOld == ctx.frameCore)
    ctx.interruptFrameCore();
  ctx.waitUserQueueSignal(signalIdx, (DeviceQueueType)queue);
#endif
VULKAN_END_CONTEXT_COMMAND
