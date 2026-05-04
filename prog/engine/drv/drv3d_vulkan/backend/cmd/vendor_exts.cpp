// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vendor_exts.h"
#include "backend/context.h"
#include "execution_sync.h"
#include "dlss.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdExecuteFSR &cmd)
{
  beginCustomStage("executeFSR");

  auto prepareImage = [&](Image *src) {
    if (src)
    {
      verifyResident(src);
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(ExtendedShaderStage::CS, RegisterType::T), src,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
    }
  };

  auto toPair = [&](Image *src) -> eastl::pair<VkImage, VkImageCreateInfo> {
    if (src)
    {
      auto ici = src->getDescription().ici.toVk();
      ici.format = src->getDescription().format.asVkFormat();
      return eastl::make_pair(src->getHandle(), ici);
    }
    return eastl::make_pair(VkImage(0), VkImageCreateInfo{});
  };

  prepareImage(cmd.params.colorTexture);
  prepareImage(cmd.params.depthTexture);
  prepareImage(cmd.params.motionVectors);
  prepareImage(cmd.params.exposureTexture);
  prepareImage(cmd.params.reactiveTexture);
  prepareImage(cmd.params.transparencyAndCompositionTexture);
  prepareImage(cmd.params.outputTexture);

  auto colorTexture = toPair(cmd.params.colorTexture);
  auto depthTexture = toPair(cmd.params.depthTexture);
  auto motionTexture = toPair(cmd.params.motionVectors);
  auto exposureTexture = toPair(cmd.params.exposureTexture);
  auto reactiveTexture = toPair(cmd.params.reactiveTexture);
  auto transparencyAndCompositionTexture = toPair(cmd.params.transparencyAndCompositionTexture);
  auto outputTexture = toPair(cmd.params.outputTexture);

  amd::FSR::UpscalingPlatformArgs args = cmd.params;
  args.colorTexture = &colorTexture;
  args.depthTexture = &depthTexture;
  args.motionVectors = &motionTexture;
  args.exposureTexture = &exposureTexture;
  args.outputTexture = &outputTexture;
  args.reactiveTexture = &reactiveTexture;
  args.transparencyAndCompositionTexture = &transparencyAndCompositionTexture;

  Backend::sync.completeNeeded();

  cmd.fsr->doApplyUpscaling(args, frameCore);

  // FSR modifies state in command buffer, so we must setup it back,
  // same way as if command buffer was interrupted
  onFrameCoreReset();
}

TSPEC void BEContext::execCmd(const CmdExecuteDLSS &cmd)
{
  beginCustomStage("executeDLSS");
  G_UNUSED(cmd);
#if !USE_STREAMLINE_FOR_DLSS
  auto prepareImage = [&](Image *src, bool out = false) {
    if (src)
    {
      verifyResident(src);
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(ExtendedShaderStage::CS, out ? RegisterType::U : RegisterType::T),
        src, out ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
    }
  };

  auto toTuple = [&](Image *src, bool out = false) -> eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> {
    if (!src)
      return eastl::make_tuple(VkImage(0), VkImageCreateInfo{}, VkImageView(0));

    ImageViewState ivs;
    ivs.setMipBase(0);
    ivs.setMipCount(1);
    ivs.setArrayBase(0);
    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = out ? 1 : 0;
    ivs.setFormat(src->getFormat());
    auto ici = src->getDescription().ici.toVk();
    ici.format = src->getDescription().format.asVkFormat();
    return eastl::make_tuple(src->getHandle(), ici, src->getImageView(ivs));
  };

  prepareImage(cmd.params.inColor);
  prepareImage(cmd.params.inDepth);
  prepareImage(cmd.params.inMotionVectors);
  prepareImage(cmd.params.inExposure);
  prepareImage(cmd.params.inAlbedo);
  prepareImage(cmd.params.inSpecularAlbedo);
  prepareImage(cmd.params.inNormalRoughness);
  prepareImage(cmd.params.inHitDist);
  prepareImage(cmd.params.outColor, true);

  auto tColor = toTuple(cmd.params.inColor);
  auto tDepth = toTuple(cmd.params.inDepth);
  auto tMotionVectors = toTuple(cmd.params.inMotionVectors);
  auto tExposure = toTuple(cmd.params.inExposure);
  auto tAlbedo = toTuple(cmd.params.inAlbedo);
  auto tSpecularAlbedo = toTuple(cmd.params.inSpecularAlbedo);
  auto tNormalRoughness = toTuple(cmd.params.inNormalRoughness);
  auto tHitDist = toTuple(cmd.params.inHitDist);
  auto tOutColor = toTuple(cmd.params.outColor, true);

  auto args = nv::convertDlssParams(cmd.params, [](Image *src) -> void * { return src; });
  args.inColor = &tColor;
  args.inDepth = &tDepth;
  args.inMotionVectors = &tMotionVectors;
  args.inExposure = &tExposure;
  args.inAlbedo = &tAlbedo;
  args.inSpecularAlbedo = &tSpecularAlbedo;
  args.inNormalRoughness = &tNormalRoughness;
  args.inHitDist = &tHitDist;
  args.outColor = &tOutColor;

  Backend::sync.completeNeeded();

  Globals::dlss.evaluate((nv::DlssParams<void> &)args, frameCore);

  // DLSS modifies state in command buffer, so we must setup it back,
  // same way as if command buffer was interrupted
  onFrameCoreReset();
#endif
}

TSPEC void BEContext::execCmd(const CmdInitializeDLSS &cmd)
{
  beginCustomStage("initializeDLSS");
  G_UNUSED(cmd);
#if !USE_STREAMLINE_FOR_DLSS
  Globals::dlss.setOptionsBackend(Globals::VK::dev.get(), frameCore, nv::DLSS::Mode(cmd.mode), IPoint2(cmd.width, cmd.height),
    cmd.use_rr, cmd.use_legacy_model);
#endif
}

TSPEC void BEContext::execCmd(const CmdReleaseDLSS &cmd)
{
  beginCustomStage("releaseDLSS");
  G_UNUSED(cmd);
#if !USE_STREAMLINE_FOR_DLSS
  Globals::dlss.DeleteFeature();
#endif
}

TSPEC void BEContext::execCmd(const CmdReleaseStreamlineDLSS &cmd)
{
  beginCustomStage("releaseStreamlineDlss");
  G_UNUSED(cmd);
#if USE_STREAMLINE_FOR_DLSS
  auto &streamlineAdapter = Globals::VK::loader.streamlineAdapter;

  streamlineAdapter->releaseDlssFeature(0);
  if (cmd.stereo_render)
    streamlineAdapter->releaseDlssFeature(1);

  if (streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    streamlineAdapter->releaseDlssGFeature(0);
#endif
}

TSPEC void BEContext::execCmd(const CmdInitializeStreamlineDLSS &cmd)
{
  beginCustomStage("initializeStreamlineDlss");
  G_UNUSED(cmd);
#if USE_STREAMLINE_FOR_DLSS
  if (Globals::VK::loader.streamlineAdapter->isDlssSupported() == nv::SupportState::Supported)
    Globals::VK::loader.streamlineAdapter->createDlssFeature(0, {cmd.width, cmd.height}, frameCore);

  if (Globals::VK::loader.streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    Globals::VK::loader.streamlineAdapter->createDlssGFeature(0, frameCore);
#endif
}

TSPEC void BEContext::execCmd(const CmdExecuteStreamlineDLSS &cmd)
{
  beginCustomStage("executeStreamlineDlss");
  G_UNUSED(cmd);
#if USE_STREAMLINE_FOR_DLSS
  const nv::DlssParams<Image> &params = cmd.dlssParams;
  auto prepareImage = [&](Image *src, bool out = false) {
    if (src)
    {
      verifyResident(src);
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(ExtendedShaderStage::CS, out ? RegisterType::U : RegisterType::T),
        src, out ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
    }
  };

  auto toTuple = [&](Image *src, bool out = false) -> eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> {
    if (!src)
      return eastl::make_tuple(VkImage(0), VkImageCreateInfo{}, VkImageView(0));

    ImageViewState ivs;
    ivs.setMipBase(0);
    ivs.setMipCount(1);
    ivs.setArrayBase(0);
    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = out ? 1 : 0;
    ivs.setFormat(src->getFormat());
    auto ici = src->getDescription().ici.toVk();
    ici.format = src->getDescription().format.asVkFormat();
    return eastl::make_tuple(src->getHandle(), ici, src->getImageView(ivs));
  };

  prepareImage(params.inColor);
  prepareImage(params.inDepth);
  prepareImage(params.inMotionVectors);
  prepareImage(params.inExposure);
  prepareImage(params.inAlbedo);
  prepareImage(params.inSpecularAlbedo);
  prepareImage(params.inNormalRoughness);
  prepareImage(params.inHitDist);
  prepareImage(params.inSsssGuide);
  prepareImage(params.inColorBeforeTransparency);
  prepareImage(params.outColor, true);

  auto tColor = toTuple(params.inColor);
  auto tDepth = toTuple(params.inDepth);
  auto tMotionVectors = toTuple(params.inMotionVectors);
  auto tExposure = toTuple(params.inExposure);
  auto tAlbedo = toTuple(params.inAlbedo);
  auto tSpecularAlbedo = toTuple(params.inSpecularAlbedo);
  auto tNormalRoughness = toTuple(params.inNormalRoughness);
  auto tHitDist = toTuple(params.inHitDist);
  auto tSsssGuide = toTuple(params.inSsssGuide);
  auto tColorBeforeTransparency = toTuple(params.inColorBeforeTransparency);
  auto tOutColor = toTuple(params.outColor, true);

  auto args = nv::convertDlssParams(params, [](Image *src) -> void * { return src; });
  auto ptrOrNull = [](auto &t) -> void * { return eastl::get<0>(t) ? &t : nullptr; };
  args.inColor = &tColor;
  args.inColorState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inDepth = &tDepth;
  args.inDepthState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inMotionVectors = &tMotionVectors;
  args.inMotionVectorsState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inExposure = ptrOrNull(tExposure);
  args.inExposureState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inAlbedo = &tAlbedo;
  args.inAlbedoState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inSpecularAlbedo = &tSpecularAlbedo;
  args.inSpecularAlbedoState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inNormalRoughness = &tNormalRoughness;
  args.inNormalRoughnessState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inHitDist = &tHitDist;
  args.inHitDistState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inSsssGuide = ptrOrNull(tSsssGuide);
  args.inSsssGuideState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inColorBeforeTransparency = ptrOrNull(tColorBeforeTransparency);
  args.inColorBeforeTransparencyState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.outColor = &tOutColor;
  args.outColorState = VK_IMAGE_LAYOUT_GENERAL;

  Backend::sync.completeNeeded();

  Globals::VK::loader.streamlineAdapter->getDlssFeature(cmd.viewIndex)->evaluate(args, frameCore);

  // DLSS modifies state in command buffer, so we must setup it back,
  // same way as if command buffer was interrupted
  onFrameCoreReset();
#endif
}

TSPEC void BEContext::execCmd(const CmdExecuteStreamlineDLSSG &cmd)
{
  beginCustomStage("executeStreamlineDlssG");
  G_UNUSED(cmd);
#if USE_STREAMLINE_FOR_DLSS
  const nv::DlssGParams<Image> &params = cmd.dlssGParams;
  auto prepareImage = [&](Image *src, bool out = false) {
    if (src)
    {
      verifyResident(src);
      Backend::sync.addImageAccess(LogicAddress::forImageOnExecStage(ExtendedShaderStage::CS, out ? RegisterType::U : RegisterType::T),
        src, out ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {0, 1, 0, 1});
    }
  };

  auto toTuple = [&](Image *src, bool out = false) -> eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> {
    if (!src)
      return eastl::make_tuple(VkImage(0), VkImageCreateInfo{}, VkImageView(0));

    ImageViewState ivs;
    ivs.setMipBase(0);
    ivs.setMipCount(1);
    ivs.setArrayBase(0);
    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = out ? 1 : 0;
    ivs.setFormat(src->getFormat());
    auto ici = src->getDescription().ici.toVk();
    ici.format = src->getDescription().format.asVkFormat();
    return eastl::make_tuple(src->getHandle(), ici, src->getImageView(ivs));
  };

  prepareImage(params.inHUDless);
  prepareImage(params.inUI);
  prepareImage(params.inDepth);
  prepareImage(params.inMotionVectors);

  auto tHUDless = toTuple(params.inHUDless);
  auto tUI = toTuple(params.inUI);
  auto tDepth = toTuple(params.inDepth);
  auto tMotionVectors = toTuple(params.inMotionVectors);

  auto args = nv::convertDlssGParams(params, [](Image *src) -> void * { return src; });
  args.inHUDless = &tHUDless;
  args.inHUDlessState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inUI = &tUI;
  args.inUIState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inDepth = &tDepth;
  args.inDepthState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  args.inMotionVectors = &tMotionVectors;
  args.inMotionVectorsState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  Backend::sync.completeNeeded();

  DLSSFrameGeneration *dlss =
    static_cast<DLSSFrameGeneration *>(Globals::VK::loader.streamlineAdapter->getDlssGFeature(cmd.viewIndex));
  dlss->evaluate(args, frameCore);

  // DLSS modifies state in command buffer, so we must setup it back,
  // same way as if command buffer was interrupted
  onFrameCoreReset();
#endif
}
