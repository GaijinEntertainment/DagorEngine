#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/bcCompressor.h>


class PanoramaCompressor
{
public:
  PanoramaCompressor() = delete;
  PanoramaCompressor(ManagedTex &srcTexture, uint32_t compressionFmt);
  ~PanoramaCompressor();

  void updateCompressedTexture(Texture *dstTex, float rgbmScale = 6.f);
  void clearIntemediateData();

private:
  void getPanoramaResolution();

  void initCompression();

  static constexpr int strideHeight = 256;
  int strideAmt{0};

  ManagedTex &panoramaTex;
  uint32_t panoramaWidth{0};
  uint32_t panoramaHeight{0};

  const uint32_t compressFmt;

  static constexpr const char *etc2_sh = "compress_panorama_etc2";
  static constexpr const char *dxt5_sh = "compress_panorama_bc3";

  eastl::unique_ptr<BcCompressor> cloudsPanoramaCompressor{nullptr};
};
