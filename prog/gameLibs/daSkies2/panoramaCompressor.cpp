// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <panoramaCompressor.h>

#define GLOBAL_VARS_LIST            \
  VAR(rgbm_conversion_scale, false) \
  VAR(panorama_rect, false)         \
  VAR(panorama_stride_count, false) \
  VAR(panorama_height, false)

#define VAR(a, opt) static ShaderVariableInfo a##VarId;
GLOBAL_VARS_LIST
#undef VAR


PanoramaCompressor::PanoramaCompressor(ManagedTex &srcTexture, uint32_t compressionFmt) :
  panoramaTex(srcTexture), compressFmt(compressionFmt)
{
#define VAR(a, b) a##VarId = get_shader_variable_id(#a, b);
  GLOBAL_VARS_LIST
#undef VAR

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = d3d::AddressMode::Wrap;
  smpInfo.address_mode_v = d3d::AddressMode::Clamp;
  sampler = d3d::request_sampler(smpInfo);

  getPanoramaResolution();
}

PanoramaCompressor::~PanoramaCompressor() {}

void PanoramaCompressor::getPanoramaResolution()
{
  TextureInfo tinfo;
  tinfo.w = tinfo.h = 0;
  panoramaTex->getinfo(tinfo);
  panoramaWidth = tinfo.w;
  panoramaHeight = tinfo.h;

  G_ASSERT((panoramaHeight % strideHeight) == 0);
  strideAmt = panoramaHeight / strideHeight;
}

void PanoramaCompressor::initCompression()
{
  auto bcType = get_texture_compression_type(compressFmt);
  bool isMobile = bcType == BcCompressor::ECompressionType::COMPRESSION_ETC2_RGBA;
  cloudsPanoramaCompressor = eastl::make_unique<BcCompressor>(bcType, 1, panoramaWidth, strideHeight, 1, isMobile ? etc2_sh : dxt5_sh);
  G_ASSERT(cloudsPanoramaCompressor->isValid());
}

void PanoramaCompressor::updateCompressedTexture(Texture *dstTex, float rgbmScale)
{
  initCompression();

  ShaderGlobal::set_real(rgbm_conversion_scaleVarId, rgbmScale);
  ShaderGlobal::set_int(panorama_stride_countVarId, strideAmt);
  ShaderGlobal::set_int(panorama_heightVarId, panoramaHeight);
  for (int i = 0; i < strideAmt; i++)
  {
    ShaderGlobal::set_color4(panorama_rectVarId, 1, strideAmt, 0, i * (1.f / strideAmt));
    cloudsPanoramaCompressor->update(panoramaTex.getTexId(), sampler, 1);
    cloudsPanoramaCompressor->copyTo(dstTex, 0, i * strideHeight, 0, 0, panoramaWidth, strideHeight);
  }
}

void PanoramaCompressor::clearIntemediateData() { cloudsPanoramaCompressor->releaseBuffer(); }