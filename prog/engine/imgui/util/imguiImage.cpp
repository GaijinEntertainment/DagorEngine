// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imguiUtil.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>

#ifdef ADD_NET_IMGUI
#include <netImgui/NetImgui_Api.h>

#include <util/dag_string.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_atomic.h>
#include <3d/dag_lockTexture.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>

#include <zstd.h>


static constexpr uint32_t FORMAT_SIZE = sizeof(TEXFMT_MASK);
static constexpr uint32_t COMPRESSION_LEVEL = 7;

struct RemoteTexConfig
{
  uint32_t originalDataSize = 1;
  uint32_t compressedDataSize = 1;
  int sendEveryNframes = 100;
  int framesSinceLastSend = sendEveryNframes;
  bool send = false;
};
static eastl::unordered_map<ImTextureID, eastl::unique_ptr<RemoteTexConfig>> remote_tex_configs;

static dag::Vector<uint8_t> imageBuffer;
static class CompressAndSendJob final : public cpujobs::IJob
{
  void *imTexId = nullptr;
  RemoteTexConfig *texConfigPtr = nullptr;

  uint32_t textureFormat = 0u;
  uint32_t textureElementSize = 0u;
  uint32_t textureRowStride = 0u;
  uint32_t textureHeight = 0u;

  uint32_t originalDataSize = 0u;
  uint32_t compressedDataSize = 0u;

  dag::Vector<uint8_t> msgBuffer = {};

public:
  CompressAndSendJob *prepare(void *texId, RemoteTexConfig *confPtr, uint32_t format, uint32_t elemSize, uint32_t rowStride,
    uint32_t height, uint32_t dataSize)
  {
    imTexId = texId;
    texConfigPtr = confPtr;

    textureFormat = format;
    textureElementSize = elemSize;
    textureRowStride = rowStride;
    textureHeight = height;

    originalDataSize = dataSize;

    return this;
  }
  const char *getJobName(bool &) const override { return "CompressAndSendJob"; }
  void doJob() override
  {
    const uint64_t compressedDataSizeBound = ZSTD_compressBound(originalDataSize);
    msgBuffer.resize(FORMAT_SIZE + compressedDataSizeBound);
    void *bufferData = msgBuffer.data();
    void *formatData = msgBuffer.data();
    void *textureData = msgBuffer.data() + FORMAT_SIZE;

    memcpy(formatData, &textureFormat, FORMAT_SIZE);
    compressedDataSize = ZSTD_compress(textureData, compressedDataSizeBound, imageBuffer.data(), originalDataSize, COMPRESSION_LEVEL);
    if (!ZSTD_isError(compressedDataSize))
    {
      texConfigPtr->compressedDataSize = compressedDataSize;
      texConfigPtr->originalDataSize = originalDataSize;
      NetImgui::SendDataTexture(imTexId, bufferData, textureRowStride / textureElementSize, textureHeight,
        NetImgui::eTexFormat::kTexFmtCustom, FORMAT_SIZE + compressedDataSize);
    }
  }
} compress_and_send_job;
#endif

static void set_sampler_cb(const ImDrawList *, const ImDrawCmd *cmd)
{
  d3d::SamplerHandle smp = (d3d::SamplerHandle)(uintptr_t)cmd->UserCallbackData;
  static int imgui_tex_samplerstateVarId = ::get_shader_variable_id("imgui_tex_samplerstate", true);
  ShaderGlobal::set_sampler(imgui_tex_samplerstateVarId, smp);
}

void ImGuiDagor::Sampler(d3d::SamplerHandle smp) { ImGui::GetWindowDrawList()->AddCallback(set_sampler_cb, (void *)smp); }

void ImGuiDagor::Image(const TEXTUREID &id, int width, int height, const ImVec2 &uv0, const ImVec2 &uv1)
{
  Image(D3dResManagerData::getBaseTex(id), width, height, uv0, uv1);
}

void ImGuiDagor::Image(const TEXTUREID &id, float aspect, const ImVec2 &uv0, const ImVec2 &uv1)
{
  int width = ImGui::GetContentRegionAvail().x;
  int height = width / aspect;

  ImGuiDagor::Image(id, width, height, uv0, uv1);
}

void ImGuiDagor::Image(const TEXTUREID &id, Texture *texture, const ImVec2 &uv0, const ImVec2 &uv1)
{
  TextureInfo ti;
  texture->getinfo(ti);

  ImGuiDagor::Image(id, float(ti.w) / ti.h, uv0, uv1);
}


void ImGuiDagor::Image(Texture *texture, const ImVec2 &uv0, const ImVec2 &uv1)
{
  TextureInfo ti;
  texture->getinfo(ti);

  ImGuiDagor::Image(texture, float(ti.w) / ti.h, uv0, uv1);
}

void ImGuiDagor::Image(Texture *texture, float aspect, const ImVec2 &uv0, const ImVec2 &uv1)
{
  int width = ImGui::GetContentRegionAvail().x;
  int height = width / aspect;

  ImGuiDagor::Image(texture, width, height, uv0, uv1);
}

void ImGuiDagor::Image(Texture *texture, int width, int height, const ImVec2 &uv0, const ImVec2 &uv1)
{
  if (!texture)
  {
    return;
  }

  auto imTexId = EncodeTexturePtr(texture);
  ImGui::Image(imTexId, ImVec2(width, height), uv0, uv1);

#ifdef ADD_NET_IMGUI
  if (!NetImgui::IsConnected())
    return;

  RemoteTexConfig *texConfigPtr = nullptr;
  {
    auto texConfigIterator = remote_tex_configs.find(imTexId);
    if (texConfigIterator == remote_tex_configs.end())
      texConfigIterator = remote_tex_configs.insert_or_assign(imTexId, eastl::make_unique<RemoteTexConfig>()).first;

    texConfigPtr = texConfigIterator->second.get();
  }

  ImGui::Checkbox(String(0, "Send image##%p", imTexId), &texConfigPtr->send);
  ImGui::SameLine();
  ImGui::SliderInt(String(0, "Send every N frames##%p", imTexId), &texConfigPtr->sendEveryNframes, 0, 100);
  ImGui::Text("Compression rate: %.2f%%", 100.f * texConfigPtr->compressedDataSize / texConfigPtr->originalDataSize);

  if (texConfigPtr->framesSinceLastSend < texConfigPtr->sendEveryNframes)
  {
    ++texConfigPtr->framesSinceLastSend;
  }
  else
  {
    if (texConfigPtr->send && interlocked_acquire_load(compress_and_send_job.done))
    {
      texConfigPtr->framesSinceLastSend = 0;

      TextureInfo ti;
      texture->getinfo(ti);
      if (ti.type == D3DResourceType::TEX)
        if (auto lockedTex = lock_texture<const ImageRawBytes>(texture, 0, TEXLOCK_READ))
        {
          const uint32_t textureFormat = ti.cflg & TEXFMT_MASK;
          const uint32_t textureElementSize = lockedTex.getBytesPerElement();
          const uint32_t textureRowStride = lockedTex.getByteStride();
          const uint32_t textureHeight = lockedTex.getHeightInElems();

          const uint64_t originalDataSize = textureRowStride * textureHeight;
          imageBuffer.resize(originalDataSize);
          memcpy(imageBuffer.data(), lockedTex.get(), originalDataSize);

          threadpool::add(compress_and_send_job.prepare(imTexId, texConfigPtr, textureFormat, textureElementSize, textureRowStride,
                            textureHeight, originalDataSize),
            threadpool::PRIO_LOW, true);
        }
    }
  }
#endif
}
