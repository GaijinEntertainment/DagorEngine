
namespace
{

template <typename T>
const char *asBooleanString(const T &value)
{
  return value ? "true" : "false";
}

String dumpCmdParam(float value, CmdDumpContext) { return String(16, "%f", value); }

String dumpCmdParam(uint32_t value, CmdDumpContext) { return String(16, "%u", value); }

String dumpCmdParam(int64_t value, CmdDumpContext) { return String(16, "%i", value); }

String dumpCmdParam(uint64_t value, CmdDumpContext) { return String(16, "%u", value); }

String dumpCmdParam(int value, CmdDumpContext) { return String(16, "%i", value); }

String dumpCmdParam(bool value, CmdDumpContext) { return String(16, "%s", asBooleanString(value)); }

String dumpCmdParam(VkExtent2D value, CmdDumpContext) { return String(32, "{%u, %u}", value.width, value.height); }

String dumpCmdParam(E3DCOLOR value, CmdDumpContext)
{
  return String(32, "E3DCOLOR{ r = %u, g = %u,  b = %u, a = %u }", value.r, value.g, value.b, value.a);
}

String dumpCmdParam(const ViewportState &value, CmdDumpContext)
{
  return String(32, "ViewportState{ x = %i, y = %i, w = %u, h = %u, min-z = %f, max-z = %f }", value.rect2D.offset.x,
    value.rect2D.offset.y, value.rect2D.extent.width, value.rect2D.extent.height, value.minZ, value.maxZ);
}

String dumpCmdParam(VulkanBufferHandle buffer, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_VK_HANDLE, generalize(buffer));
  return String(32, "VulkanBufferHandle{ 0x%X }", buffer.value);
}

String dumpCmdParam(VulkanQueryPoolHandle buffer, CmdDumpContext) { return String(32, "VulkanQueryPoolHandle{ 0x%X }", buffer.value); }

String dumpCmdParam(const BufferRef &value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value.buffer);
  return String(32, "BufferRef{ buffer = 0x%p{0x%X}, discardIndex = %u }", value.buffer,
    value.buffer ? value.buffer->getHandle().value : 0, value.discardIndex);
}

String dumpCmdParam(const VkRect2D &value, CmdDumpContext)
{
  return String(32, "VkRect2D{ x = %i, y = %i, w = %u, h = %u }", value.offset.x, value.offset.y, value.extent.width,
    value.extent.height);
}

String dumpCmdParam(const BufferSubAllocation &value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value.buffer);
  return String(32, "BufferSubAllocation{ buffer = 0x%p{0x%X}, offset = %u, size = %u }", value.buffer,
    value.buffer ? value.buffer->getHandle().value : 0, value.offset, value.size);
}

String dumpCmdParam(ProgramID value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_PIPE, (uint64_t)value.get());
  return String(32, "ProgramID{ %u }", value.get());
}

String dumpCmdParam(const VkImageSubresourceRange &value, CmdDumpContext)
{
  return String(32,
    "VkImageSubresourceRange{ aspectMask = %u, baseMipLevel = %u, levelCount = %u, "
    "baseArrayLayer = %u, layerCount = %u }",
    value.aspectMask, value.baseMipLevel, value.levelCount, value.baseArrayLayer, value.layerCount);
}

String dumpCmdParam(const VkClearDepthStencilValue &value, CmdDumpContext)
{
  return String(32, "VkClearDepthStencilValue{ depth = %f, stencil = %u }", value.depth, value.stencil);
}

String dumpCmdParam(const VkImageBlit &value, CmdDumpContext)
{
  return String(32,
    "VkImageBlit{ VkImageSubresourceLayers{ aspectMask = %u, mipLevel = %u, "
    "baseArrayLayer = %u, layerCount = %u },VkOffset3D[0]{ x = %u, y = %u, z = %u "
    "},VkOffset3D[1]{ x = %u, y = %u, z = %u },VkImageSubresourceLayers{ aspectMask = %u, "
    "mipLevel = %u, baseArrayLayer = %u, layerCount = %u },VkOffset3D[0]{ x = %u, y = %u, z "
    "= %u },VkOffset3D[1]{ x = %u, y = %u, z = %u } }",
    value.srcSubresource.aspectMask, value.srcSubresource.mipLevel, value.srcSubresource.baseArrayLayer,
    value.srcSubresource.layerCount, value.srcOffsets[0].x, value.srcOffsets[0].y, value.srcOffsets[0].z, value.srcOffsets[1].x,
    value.srcOffsets[1].y, value.srcOffsets[1].z, value.dstSubresource.aspectMask, value.dstSubresource.mipLevel,
    value.dstSubresource.baseArrayLayer, value.dstSubresource.layerCount, value.dstOffsets[0].x, value.dstOffsets[0].y,
    value.dstOffsets[0].z, value.dstOffsets[1].x, value.dstOffsets[1].y, value.dstOffsets[1].z);
}

String dumpCmdParam(const TimestampQueryRef &value, CmdDumpContext)
{
  return String(32, "TimestampQueryRef{ pool = 0x%p{0x%X}, index = %u }", value.pool, value.pool ? value.pool->pool.value : 0,
    value.index);
}

String dumpCmdParam(const VkClearColorValue &value, CmdDumpContext)
{
  return String(64,
    "\nVkClearColorValue.float32 { %f, %f, %f, %f }\n"
    "VkClearColorValue.int32 { %i, %i, %i, %i }\n"
    "VkClearColorValue.uint32 { %u, %u, %u, %u }\n",
    value.float32[0], value.float32[1], value.float32[2], value.float32[3], value.int32[0], value.int32[1], value.int32[2],
    value.int32[3], value.uint32[0], value.uint32[1], value.uint32[2], value.uint32[3]);
}

#define ENUM_TO_NAME(Name) \
  case Name: enumName = #Name; break

String dumpCmdParam(VkPrimitiveTopology top, CmdDumpContext) { return String(32, "%s", formatPrimitiveTopology(top)); }

String dumpCmdParam(VkCompareOp value, CmdDumpContext)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_COMPARE_OP_NEVER);
    ENUM_TO_NAME(VK_COMPARE_OP_LESS);
    ENUM_TO_NAME(VK_COMPARE_OP_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_LESS_OR_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_GREATER);
    ENUM_TO_NAME(VK_COMPARE_OP_NOT_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_GREATER_OR_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_ALWAYS);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

String dumpCmdParam(VkBlendOp value, CmdDumpContext)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_BLEND_OP_ADD);
    ENUM_TO_NAME(VK_BLEND_OP_SUBTRACT);
    ENUM_TO_NAME(VK_BLEND_OP_REVERSE_SUBTRACT);
    ENUM_TO_NAME(VK_BLEND_OP_MIN);
    ENUM_TO_NAME(VK_BLEND_OP_MAX);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

String dumpCmdParam(VkBlendFactor value, CmdDumpContext)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_BLEND_FACTOR_ZERO);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_DST_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_DST_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_CONSTANT_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_CONSTANT_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC1_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC1_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

String dumpCmdParam(VkStencilOp value, CmdDumpContext)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_STENCIL_OP_KEEP);
    ENUM_TO_NAME(VK_STENCIL_OP_ZERO);
    ENUM_TO_NAME(VK_STENCIL_OP_REPLACE);
    ENUM_TO_NAME(VK_STENCIL_OP_INCREMENT_AND_CLAMP);
    ENUM_TO_NAME(VK_STENCIL_OP_DECREMENT_AND_CLAMP);
    ENUM_TO_NAME(VK_STENCIL_OP_INVERT);
    ENUM_TO_NAME(VK_STENCIL_OP_INCREMENT_AND_WRAP);
    ENUM_TO_NAME(VK_STENCIL_OP_DECREMENT_AND_WRAP);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

String dumpCmdParam(VkIndexType value, CmdDumpContext)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_INDEX_TYPE_UINT16);
    ENUM_TO_NAME(VK_INDEX_TYPE_UINT32);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

#undef ENUM_TO_NAME

String dumpCmdParam(ImageViewState value, CmdDumpContext)
{
  return String(32,
    "ImageViewState{ sampleStencil = %u, isRenderTarget = %u, isCubemap = %u, isArray = "
    "%u, format = %s, mipMapBase = %u, mipMapCount = %u, arrayBase = %u, arrayCount = %u }",
    asBooleanString(value.sampleStencil), asBooleanString(value.isRenderTarget), asBooleanString(value.isCubemap),
    asBooleanString(value.isArray), value.getFormat().getNameString(), value.getMipBase(), value.getMipCount(), value.getArrayBase(),
    value.getArrayCount());
}

String dumpCmdParam(VulkanImageViewHandle value, CmdDumpContext) { return String(32, "%u", value.value); }

String dumpCmdParam(RenderPassResource *value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p{0x" PTR_LIKE_HEX_FMT "}", value, value ? value->getHandle().value : 0);
}

String dumpCmdParam(Buffer *value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p{0x" PTR_LIKE_HEX_FMT "}", value, value ? value->getHandle().value : 0);
}

String dumpCmdParam(Image *value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p{0x" PTR_LIKE_HEX_FMT "}", value, value ? value->getHandle().value : 0);
}

String dumpCmdParam(InputLayout *value, CmdDumpContext) { return String(32, "0x%p", value); }

String dumpCmdParam(ComputePipeline *value, CmdDumpContext) { return String(32, "0x%p", value); }

String dumpCmdParam(GraphicsProgram *value, CmdDumpContext) { return String(32, "0x%p", value); }

String dumpCmdParam(ThreadedFence *value, CmdDumpContext) { return String(32, "0x%p", value); }

String dumpCmdParam(AsyncCompletionState *value, CmdDumpContext) { return String(32, "0x%p", value); }

#if D3D_HAS_RAY_TRACING

String dumpCmdParam(RaytraceAccelerationStructure *value, CmdDumpContext ctx)
{
  ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p", value);
}

String dumpCmdParam(RaytracePipeline *value, CmdDumpContext) { return String(32, "0x%p", value); }

String dumpCmdParam(RaytraceBuildFlags value, CmdDumpContext) { return String(32, "0x%X", (uint32_t)value); }

#endif

template <typename T, size_t N>
String dumpCmdParam(const T (&values)[N], CmdDumpContext ctx)
{
  String ret("\n");
  for (auto &&value : values)
  {
    ret += dumpCmdParam(value, ctx);
    ret += "\n";
  }
  return ret;
}

String dumpCmdParam(const ShaderModuleUse &smu, CmdDumpContext)
{
  // TODO: debug print header...
  return String(32, "{<header>, %u}", /*smu.header, */ smu.blobId);
}


String dumpCmdParam(const ShaderModuleHeader &smh, CmdDumpContext)
{
  G_UNUSED(smh);
  return String(32, "...");
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA

String dumpCmdParam(const ShaderDebugInfo &sdi, CmdDumpContext)
{
  return String(128,
    "shader debug attachment\n"
    ">> name = %s\n"
    ">> spirvDisAsm = %s\n"
    ">> dxbcDisAsm = %s\n"
    ">> sourceGLSL = %s\n"
    ">> reconstructedHLSL = %s\n"
    ">> reconstructedHLSLAndSourceHLSLXDif = %s\n"
    ">> sourceHLSL = %s\n"
    ">> debugName = %s",
    sdi.name, sdi.spirvDisAsm, sdi.dxbcDisAsm, sdi.sourceGLSL, sdi.reconstructedHLSL, sdi.reconstructedHLSLAndSourceHLSLXDif,
    sdi.sourceHLSL, sdi.debugName);
}

String dumpCmdParam(ShaderModuleBlob *, CmdDumpContext)
{
  // blob is deleted after execution for CS pipes, so we can't print data at all here
  return String(32, "shader module ...");
}

#endif

String dumpCmdParam(StringIndexRef index, CmdDumpContext ctx)
{
  return String(32, "%s", ctx.renderWork.charStore.data() + index.get());
}

String dumpCmdParam(const ShaderWarmUpInfo &, CmdDumpContext) { return String(32, "ShaderWarmUpInfo ..."); }

String dumpCmdParam(const shaders::DriverRenderStateId &, CmdDumpContext) { return String(32, "shaders::DriverRenderStateId ..."); }

String dumpCmdParam(const shaders::RenderState &, CmdDumpContext) { return String(32, "shaders::RenderState ..."); }

String dumpCmdParam(const SwapchainMode &, CmdDumpContext) { return String(32, "SwapchainMode ..."); }

String dumpCmdParam(const InputLayoutID &val, CmdDumpContext) { return String(32, "InputLayoutID %u", val.get()); }

String dumpCmdParam(FrameEvents *, CmdDumpContext) { return String(32, "Frame event callback ..."); }

String dumpCmdParam(const VkBufferImageCopy &val, CmdDumpContext)
{
  return String(128,
    "VkBufferImageCopy\n"
    ">>bufferOffset = %llu\n"
    ">>bufferRowLength = %u\n"
    ">>bufferImageHeight = %u\n"
    ">>imageOffset %u %u %u\n"
    ">>imageExtent %u %u %u\n"
    ">>imageSubresource = mip %u baseLayer %u layers %u aspect %lu",
    val.bufferOffset, val.bufferRowLength, val.bufferImageHeight, val.imageOffset.x, val.imageOffset.y, val.imageOffset.z,
    val.imageExtent.width, val.imageExtent.height, val.imageExtent.depth, val.imageSubresource.mipLevel,
    val.imageSubresource.baseArrayLayer, val.imageSubresource.layerCount, val.imageSubresource.aspectMask);
}

String dumpCmdParam(const PipelineState &, CmdDumpContext)
{
  return String(32, "PipelineState ...");
  // TODO
  // const_cast<PipelineState&>(s).dumpLog();
}

String dumpCmdParam(const URegister &uReg, CmdDumpContext ctx)
{
  if (uReg.image)
    ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)uReg.image);
  if (uReg.buffer.buffer)
    ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)uReg.buffer.buffer);
  return String(32, "img %p buf %p", uReg.image, uReg.buffer.buffer);
}

String dumpCmdParam(const TRegister &tReg, CmdDumpContext ctx)
{
  switch (tReg.type)
  {
    case TRegister::TYPE_IMG:
      ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.img.ptr);
      return String(64, "img %p %s %s", tReg.img.ptr, tReg.isSwapchainColor ? "swc_clr" : "-",
        tReg.isSwapchainDepth ? "swc_depth" : "-");
    case TRegister::TYPE_BUF:
      ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.buf.buffer);
      return String(64, "buf %p\n", tReg.buf.buffer);
#if D3D_HAS_RAY_TRACING
    case TRegister::TYPE_AS:
      ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.rtas);
      return String(64, "as %p", tReg.rtas);
#endif
    default: return String(32, "empty");
  }
}

String dumpCmdParam(const SRegister &sReg, CmdDumpContext ctx)
{
  if (sReg.type == SRegister::TYPE_NULL)
    return String(32, "spl empty");
  else if (sReg.type == SRegister::TYPE_RES)
  {
    ctx.addRef(FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)sReg.resPtr);
    return String(32, "spl res %p", sReg.resPtr);
  }
  else if (sReg.type == SRegister::TYPE_STATE)
    return String(32, "spl state %016llX", sReg.state);
  else
    return String(32, "spl unknown type %u", sReg.type);
}


String dumpCmdParam(const RenderPassArea &area, CmdDumpContext)
{
  return String(32, "RPArea [%u, %u] - [%u, %u] Z [%f, %f]", area.left, area.top, area.width, area.height, area.minZ, area.maxZ);
}

} // namespace