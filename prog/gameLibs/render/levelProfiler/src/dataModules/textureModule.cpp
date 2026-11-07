// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texIdSet.h>
#include <3d/dag_resMgr.h>
#include <3d/dag_buildOnDemandTexFactory.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_texture.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <gui/dag_imguiUtil.h>
#include "textureModule.h"

namespace levelprofiler
{

// Conversion constant: 1 MB = 1024 * 1024 bytes
static const float BYTES_TO_MEGABYTES = 1048576.0f;

// Texture dimension threshold for filtering significant sizes
static const int TEXTURE_SIZE_THRESHOLD = 256;

// Number of RGBA channels in texture
static constexpr int RGBA_CHANNEL_COUNT = 4;

// Bit mask for all RGBA channels enabled (RGBA = 1+2+4+8 = 15)
static constexpr int RGBA_CHANNELS_MASK = (1 << RGBA_CHANNEL_COUNT) - 1;

// Number of style colors pushed for channel buttons (Button, ButtonHovered, ButtonActive)
static const int CHANNEL_BUTTON_STYLE_COUNT = 3;

TextureModule::TextureModule()
{
  for (int i = 0; i < RGBA_CHANNEL_COUNT; ++i)
    channelMask[i] = true; // Default state - all channels visible
}

TextureModule::~TextureModule() { shutdownImpl(); }

void TextureModule::init()
{
  imguiChannelMaskVariableId = get_shader_variable_id("imgui_channel_mask", true);
  imguiMipLevelVariableId = get_shader_variable_id("imgui_mip_level", true);
  imguiUseCustomSamplerVariableId = get_shader_variable_id("imgui_use_custom_sampler", true);

  // Create point sampler for texture preview (nearest-neighbor filtering)
  d3d::SamplerInfo samplerInfo;
  samplerInfo.filter_mode = d3d::FilterMode::Point;
  pointSamplerHandle = d3d::request_sampler(samplerInfo);
}

void TextureModule::shutdown() { shutdownImpl(); }

void TextureModule::shutdownImpl() { clearImpl(); }

void TextureModule::clear() { clearImpl(); }

void TextureModule::clearImpl()
{
  textures.clear();

  dataGeneration++;
}

// The memory of the mips [first_mip, info.mipLevels) of a texture in the format of info.cflg.
static uint64_t calc_mips_memory(const TextureInfo &info, int first_mip)
{
  const TextureFormatDesc &format = get_tex_format_desc(info.cflg);
  const bool isVolume = info.type == D3DResourceType::VOLTEX;
  uint64_t total = 0;
  for (int mip = first_mip; mip < info.mipLevels; ++mip)
  {
    const uint64_t columns = (max(info.w >> mip, 1) + format.elementWidth - 1) / format.elementWidth;
    const uint64_t rows = (max(info.h >> mip, 1) + format.elementHeight - 1) / format.elementHeight;
    total += columns * rows * format.bytesPerElement * (isVolume ? max(info.d >> mip, 1) : 1);
  }
  return total * (isVolume ? 1 : max<int>(info.a, 1));
}

// The state of a streamed texture when its top mip is the texture manager level (as get_managed_res_loaded_lev). Level 1 of a
// texture with more levels is the stub texture (downgrade_managed_tex_to_lev): it has no texture data.
static TextureStateSize calc_state_at_level(const TextureInfo &full, int full_level, unsigned level)
{
  const int skip = max(full_level - int(level), 0);
  if ((level <= 1 && full_level >= 2) || skip >= full.mipLevels)
    return {};
  return {max(full.w >> skip, 1), max(full.h >> skip, 1), full.mipLevels - skip, calc_mips_memory(full, skip)};
}

void TextureModule::collect()
{
  clear();

  for (TEXTUREID tid = first_managed_texture(1); tid != BAD_TEXTUREID; tid = next_managed_texture(tid, 1))
  {
    BaseTexture *tex = acquire_managed_tex(tid);
    if (!tex)
      continue;

    TextureData textureData;
    textureData.name = get_managed_texture_name(tid);

    TextureInfo ti;
    tex->getinfo(ti, 0);

    // The manager entry holds the full size and level of the texture, with its HQ or UHQ part, for any texture it streams.
    ManagedTexEntryDesc entry;
    textureData.isStreamed = get_managed_tex_entry_desc(tid, entry) && entry.maxLev > 0 && entry.mipLevels > 0;
    if (textureData.isStreamed)
    {
      ti.w = entry.width;
      ti.h = entry.height;
      ti.mipLevels = entry.mipLevels;
      if (ti.type == D3DResourceType::VOLTEX)
        ti.d = entry.depth;
      else if (ti.type == D3DResourceType::ARRTEX)
        ti.a = entry.depth;

      textureData.states[*TextureState::FULL] = {ti.w, ti.h, ti.mipLevels, calc_mips_memory(ti, 0)};
      // Only the textures that the game holds are listed (refcount >= 1), so the manager has set their quality limit.
      textureData.states[*TextureState::MAX] = calc_state_at_level(ti, entry.maxLev, get_managed_res_quality_limit_lev(tid));
      textureData.states[*TextureState::RES] = calc_state_at_level(ti, entry.maxLev, entry.ldLev);
      textureData.quality = {.selectedQuality = ddsx::get_tex_quality(tid),
        .settings = ::dgs_tex_quality,
        .explicitList = ddsx::get_explicit_tex_list(tid),
        .maxTql = get_managed_res_max_tql(tid)};
    }
    else
    {
      const TextureStateSize resource = {ti.w, ti.h, ti.mipLevels, uint64_t(tex->getSize())};
      for (TextureStateSize &state : textureData.states)
        state = resource;
    }

    textureData.info = ti;

    release_managed_tex(tid);

    textures[textureData.name] = textureData;
  }
}

void TextureModule::drawUI() {}

const eastl::hash_map<ProfilerString, TextureData> &TextureModule::getTextures() const { return textures; }

float TextureModule::getTextureMemorySize(const TextureData &texture_data) const
{
  return float(texture_data.getState(TextureState::FULL).memBytes) / BYTES_TO_MEGABYTES;
}

void TextureModule::drawTextureView(const ProfilerString &texture_name)
{
  // Look up texture info
  auto iterator = textures.find(texture_name);
  if (iterator == textures.end())
    return;

  // Get engine texture ID
  TEXTUREID textureId = get_managed_texture_id(texture_name.c_str());
  if (textureId == BAD_TEXTUREID)
    return;

  // Get texture info for UI
  auto &currentTextureData = iterator->second;
  auto &currentTextureInfo = currentTextureData.info;

  // Channel selector buttons (R/G/B/A) with color coding
  {
    static constexpr const char *labels[RGBA_CHANNEL_COUNT] = {"R", "G", "B", "A"};
    // Color schemes for each channel button
    const ImVec4 activeCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(0.90f, 0.20f, 0.20f, 0.85f), // Red channel
      ImVec4(0.20f, 0.90f, 0.20f, 0.85f), // Green channel
      ImVec4(0.20f, 0.20f, 0.90f, 0.85f), // Blue channel
      ImVec4(0.85f, 0.85f, 0.85f, 0.85f), // Alpha channel
    };
    const ImVec4 hoverCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(1.00f, 0.30f, 0.30f, 1.00f),
      ImVec4(0.30f, 1.00f, 0.30f, 1.00f),
      ImVec4(0.30f, 0.30f, 1.00f, 1.00f),
      ImVec4(1.00f, 1.00f, 1.00f, 1.00f),
    };
    const ImVec4 disabledCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(0.50f, 0.10f, 0.10f, 0.20f),
      ImVec4(0.10f, 0.50f, 0.10f, 0.20f),
      ImVec4(0.10f, 0.10f, 0.50f, 0.20f),
      ImVec4(0.50f, 0.50f, 0.50f, 0.20f),
    };

    ImGuiStyle &style = ImGui::GetStyle();
    float oldRounding = style.FrameRounding;
    style.FrameRounding = 6.0f;

    for (int channelIndex = 0; channelIndex < RGBA_CHANNEL_COUNT; ++channelIndex)
    {
      ImGui::PushID(channelIndex);
      if (channelIndex > 0)
        ImGui::SameLine();

      bool isChannelVisible = channelMask[channelIndex];

      ImVec4 buttonColor = isChannelVisible ? activeCol[channelIndex] : disabledCol[channelIndex];
      ImVec4 hoverColor = isChannelVisible ? hoverCol[channelIndex] : disabledCol[channelIndex];
      ImVec4 activeColor = activeCol[channelIndex];

      ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

      if (ImGui::Button(labels[channelIndex], ImVec2(30, 0)))
        isChannelVisible = !isChannelVisible; // Left click - toggle this channel

      // Right click - solo this channel (turn off others)
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
      {
        for (int j = 0; j < RGBA_CHANNEL_COUNT; ++j)
          channelMask[j] = false;

        isChannelVisible = true;
      }

      channelMask[channelIndex] = isChannelVisible;

      ImGui::PopStyleColor(CHANNEL_BUTTON_STYLE_COUNT);
      ImGui::PopID();
    }

    // Restore original style
    style.FrameRounding = oldRounding;
  }

  // Mip level slider
  {
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    ImGui::SliderInt("##mip_level", &selectedMipLevel, 0, currentTextureInfo.mipLevels - 1, "Mip Level: %d");
    ImGui::PopItemWidth();
  }

  // Setup custom texture preview rendering
  ImGui::GetWindowDrawList()->AddCallback(
    [](const ImDrawList *parent_list, const ImDrawCmd *cmd) {
      TextureModule *self = static_cast<TextureModule *>(cmd->UserCallbackData);
      self->setImagePreviewMode(parent_list, cmd);
    },
    this);

  // Calculate display size preserving aspect ratio
  ImVec2 availableRegion = ImGui::GetContentRegionAvail();
  float textureAspectRatio = float(currentTextureInfo.w) / currentTextureInfo.h;
  float displayWidth = availableRegion.x, displayHeight = availableRegion.y;

  if (displayWidth / displayHeight > textureAspectRatio)
    displayWidth = displayHeight * textureAspectRatio; // Constrained by height
  else
    displayHeight = displayWidth / textureAspectRatio; // Constrained by width

  ImGui::Image(ImGuiDagor::EncodeTexturePtr(D3dResManagerData::getBaseTex(textureId)), ImVec2(displayWidth, displayHeight));

  if (ImGui::IsItemHovered())
  {
    float sizeMB = getTextureMemorySize(currentTextureData);
    ImGui::SetTooltip("%s\n%s\n%dx%d\n%.2f MB", texture_name.c_str(), getFormatName(currentTextureInfo.cflg), currentTextureInfo.w,
      currentTextureInfo.h, sizeMB);
  }

  // Restore rendering state
  ImGui::GetWindowDrawList()->AddCallback(
    [](const ImDrawList *parent_list, const ImDrawCmd *cmd) {
      TextureModule *self = static_cast<TextureModule *>(cmd->UserCallbackData);
      self->resetImagePreviewMode(parent_list, cmd);
    },
    this);
}

void TextureModule::setImagePreviewMode(const ImDrawList * /* draw_list */, const ImDrawCmd * /* command */)
{
  // Encode channel mask as bit field
  int maskValue = 0;
  for (int i = 0; i < RGBA_CHANNEL_COUNT; ++i)
  {
    if (channelMask[i])
      maskValue |= 1 << i;
  }

  ShaderGlobal::set_int(imguiChannelMaskVariableId, maskValue);
  ShaderGlobal::set_int(imguiMipLevelVariableId, selectedMipLevel);
  ShaderGlobal::set_int(imguiUseCustomSamplerVariableId, 1);

  static int samplerSlot = ShaderGlobal::get_slot_by_name("custom_sampler_const_no");
  d3d::set_sampler(STAGE_PS, samplerSlot, pointSamplerHandle);
}

void TextureModule::resetImagePreviewMode(const ImDrawList * /* draw_list */, const ImDrawCmd * /* command */)
{
  ShaderGlobal::set_int(imguiChannelMaskVariableId, RGBA_CHANNELS_MASK); // Enable all RGBA channels
  ShaderGlobal::set_int(imguiUseCustomSamplerVariableId, 0);             // Use default sampler
}

const char *TextureModule::getFormatName(uint32_t format) { return get_tex_format_name(format); }

int TextureModule::getSizeFilterKey(int size)
{
  bool isPowerOf2 = (size & (size - 1)) == 0;
  bool isMultipleOfThreshold = (size % TEXTURE_SIZE_THRESHOLD) == 0;
  return (isPowerOf2 || isMultipleOfThreshold) ? size : -1;
}

} // namespace levelprofiler