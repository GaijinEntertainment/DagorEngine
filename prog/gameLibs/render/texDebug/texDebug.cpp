// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/texDebug.h>
#include <render/texDebug.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_computeShaders.h>
#include <generic/dag_enumerate.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <image/dag_dds.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>

#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>

#include "images.h"

namespace texdebug
{

static int imgui_channel_maskVarId = -1;
static int imgui_r_minVarId = -1;
static int imgui_r_maxVarId = -1;
static int imgui_g_minVarId = -1;
static int imgui_g_maxVarId = -1;
static int imgui_b_minVarId = -1;
static int imgui_b_maxVarId = -1;
static int imgui_a_minVarId = -1;
static int imgui_a_maxVarId = -1;
static int imgui_mip_levelVarId = -1;
static int imgui_cube_view_dirVarId = -1;
static int imgui_cube_view_upVarId = -1;
static int imgui_cube_view_rightVarId = -1;
static int imgui_use_custom_sampler = -1;
static int imgui_tex_type = -1;
static int imgui_slice = -1;
static int imgui_tex_diff = -1;
static int tex_debug_textureVarId = -1;
static int tex_debug_texture_for_diffVarId = -1;
static int tex_debug_readback_bufferVarId = -1;
static int tex_debug_readback_uvVarId = -1;
static int tex_debug_mip_levelVarId = -1;
static int tex_debug_sliceVarId = -1;
static int tex_debug_sliceIVarId = -1;
static int tex_debug_typeVarId = -1;
static int tex_debug_rangesVarId = -1;

struct ChannelFilter
{
  bool active = true;
  float min = 0.0f;
  float max = 1.0f;
};

static eastl::hash_map<eastl::string, TextureInfo> textures;
static eastl::vector<eastl::string> filteredTextures;
static eastl::unordered_set<eastl::string> pinnedTextures;

static bool filterRT = false;
static bool filterDepth = false;
static int filterTexType = RES3D_TEX;
static char filterName[128] = "";

static eastl::string selectedTextureName;
static eastl::string selectedTextureNameForDiff;

static int selectedMipLevel = 0;
static int selectedArraySlice = 0;
static int selectedDepthSlice = 0;

static Point2 uvTL = Point2::ZERO;
static Point2 uvBR = Point2(1, 1);
static Point2 cursorUV = Point2::ZERO;

static Point4 pickColor = Point4::ZERO;
static Point4 pickColorMin = Point4::ZERO;
static Point4 pickColorMax = Point4::ZERO;

static ChannelFilter channelFilters[4];
static bool rgbFiltersLocked = false;

static Ptr<ComputeShaderElement> processTextureShader;

static UniqueBuf pickBuffer;
static EventQueryHolder pickBufferQuery;
static bool pickBufferQueryActive = false;
static bool pickBufferQueryCalcRangesStart = false;
static bool pickBufferQueryCalcRangesRunning = false;

static d3d::SamplerHandle pointSampler = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle borderSampler = d3d::INVALID_SAMPLER_HANDLE;

static UniqueTex lockImage;
static UniqueTex unlockImage;

static TMatrix cubeViewMatrix;

static bool selectableNeedsFocus = false;

static void save_pinned_list()
{
#if _TARGET_PC | _TARGET_XBOX
  DataBlock blk;
  for (auto &pin : pinnedTextures)
    blk.addStr("pinned", pin.data());
  blk.saveToTextFile("tex_debug.blk");
#endif
}

static void reset_view()
{
  uvTL = Point2::ZERO;
  uvBR = Point2(1, 1);
  selectedMipLevel = 0;
  for (auto &filter : channelFilters)
  {
    filter.active = true;
    filter.min = 0;
    filter.max = 1;
  }
  channelFilters[3].active = false;
  selectedMipLevel = 0;
  selectedArraySlice = 0;
  selectedDepthSlice = 0;

  cubeViewMatrix.identity();
}

static TextureInfo *get_filtered_texture()
{
  if (selectedTextureName.empty())
    return nullptr;

  auto iter = textures.find(selectedTextureName);
  if (iter == textures.end())
    return nullptr;

  return &iter->second;
}

static bool is_depth_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg >= TEXFMT_FIRST_DEPTH && cflg <= TEXFMT_LAST_DEPTH;
}

static void update_filtered_textures()
{
  filteredTextures.clear();
  for (auto [iterIx, iter] : enumerate(textures))
  {
    if (iter.first.empty())
      continue;
    if (filterTexType != iter.second.resType)
      continue;
    if (filterRT && (iter.second.cflg & TEXCF_RTARGET) == 0)
      continue;
    if (filterDepth && !is_depth_format_flg(iter.second.cflg))
      continue;

    if (filterName[0] && !strstr(iter.first.data(), filterName))
      continue;

    filteredTextures.push_back(iter.first);
  }
  selectedTextureName.clear();
  eastl::sort(filteredTextures.begin(), filteredTextures.end());
  reset_view();
}

static void refresh_textures()
{
  textures.clear();
  selectedTextureName.clear();
  for (TEXTUREID id = first_managed_texture(1); id != BAD_TEXTUREID; id = next_managed_texture(id, 1))
  {
    if (auto texture = acquire_managed_tex(id))
    {
      TextureInfo entry;
      texture->getinfo(entry);
      textures.insert({eastl::string(texture->getResName()), entry});
      release_managed_tex(id);
    }
  }
}

static const char *get_tex_type_name(int type)
{
  switch (type)
  {
    case RES3D_TEX: return "2D";
    case RES3D_VOLTEX: return "Volume";
    case RES3D_CUBETEX: return "Cube";
    case RES3D_ARRTEX: return "Array";
    case RES3D_CUBEARRTEX: return "Cube array";
    default: return "Unknown";
  }
}

static int filter_count(const TextureFormatDesc &desc)
{
  int count = 0;
  if (desc.r.bits > 0)
    ++count;
  if (desc.g.bits > 0)
    ++count;
  if (desc.b.bits > 0)
    ++count;
  if (desc.a.bits > 0)
    ++count;
  if (desc.depth.bits > 0)
    ++count;
  return count;
}

static bool render_channel_filter(const char *name, ChannelFilter &filter, const TextureChannelFormatDesc &desc, int tail_size)
{
  if (desc.bits == 0)
    return false;

  float minValue = 0;
  float maxValue = 1;
  float step = 0.001f;
  if (desc.isNormalized)
  {
    // Defaults
  }
  else if (desc.isFloatPoint)
  {
    minValue = desc.isSigned ? -100 : 0;
    maxValue = desc.isSigned ? 100 : 100;
  }
  else if (desc.isSigned)
  {
    minValue = -100;
    maxValue = 100;
    step = 1;
  }

  ImGui::Checkbox(name, &filter.active);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(max(1.0f, ImGui::GetContentRegionAvail().x - tail_size));
  return ImGui::DragFloatRange2(String(0, "##%s_range", name), &filter.min, &filter.max, step, minValue, maxValue, "%.6f");
}

static void clip_uv()
{
  auto uvSize = uvBR.x - uvTL.x;
  if (uvSize >= 1)
  {
    uvTL = Point2::ZERO;
    uvBR = Point2(1, 1);
  }
  else
  {
    if (uvTL.x < 0)
    {
      uvBR.x -= uvTL.x;
      uvTL.x = 0;
    }
    if (uvTL.y < 0)
    {
      uvBR.y -= uvTL.y;
      uvTL.y = 0;
    }
    if (uvBR.x > 1)
    {
      uvTL.x -= uvBR.x - 1;
      uvBR.x = 1;
    }
    if (uvBR.y > 1)
    {
      uvTL.y -= uvBR.y - 1;
      uvBR.y = 1;
    }
  }
}

static bool is_integer_texture(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg == TEXFMT_R8UI || cflg == TEXFMT_R16UI || cflg == TEXFMT_R32UI || cflg == TEXFMT_A16B16G16R16UI ||
         cflg == TEXFMT_A32B32G32R32UI || cflg == TEXFMT_R32G32UI;
}

static int tex_type_ix(TextureInfo &ti)
{
  switch (ti.resType)
  {
    case RES3D_TEX: return is_integer_texture(ti.cflg) ? 4 : 0;
    case RES3D_VOLTEX: return 1;
    case RES3D_CUBETEX: return 3;
    case RES3D_ARRTEX: return 2;
    case RES3D_CUBEARRTEX: return 3;
    default: return 0;
  }
}

static float tex_slice_val(const TextureInfo &ti)
{
  switch (ti.resType)
  {
    case RES3D_TEX: return 0;
    case RES3D_VOLTEX: return (selectedDepthSlice + 0.5f) / ti.d;
    case RES3D_CUBETEX: return selectedArraySlice == 6 ? -1 : selectedArraySlice;
    case RES3D_ARRTEX: return selectedArraySlice;
    case RES3D_CUBEARRTEX: return selectedArraySlice;
    default: return 0;
  }
}

static float tex_sliceI_val(const TextureInfo &ti)
{
  switch (ti.resType)
  {
    case RES3D_TEX: return 0;
    case RES3D_VOLTEX: return selectedDepthSlice;
    case RES3D_CUBETEX: return selectedArraySlice == 6 ? -1 : selectedArraySlice;
    case RES3D_ARRTEX: return selectedArraySlice;
    case RES3D_CUBEARRTEX: return selectedArraySlice;
    default: return 0;
  }
}

static void set_image_mode(const ImDrawList *, const ImDrawCmd *cmd)
{
  static int custom_sampler_const_no = ShaderGlobal::get_slot_by_name("custom_sampler_const_no");
  if ((int)(uintptr_t)cmd->UserCallbackData == -1)
  {
    ShaderGlobal::set_int(imgui_use_custom_sampler, 1);
    d3d::set_sampler(STAGE_PS, custom_sampler_const_no, borderSampler);
    return;
  }

  auto entry = get_filtered_texture();
  if (!entry)
    return;

  uint32_t mask = 0;
  for (int i = 0; i < 4; ++i)
    if (channelFilters[i].active)
      mask |= 1 << i;

  int texType = tex_type_ix(*entry);
  float slice = tex_slice_val(*entry);

  ShaderGlobal::set_int(imgui_channel_maskVarId, mask);
  ShaderGlobal::set_real(imgui_r_minVarId, channelFilters[0].min);
  ShaderGlobal::set_real(imgui_r_maxVarId, channelFilters[0].max);
  ShaderGlobal::set_real(imgui_g_minVarId, channelFilters[1].min);
  ShaderGlobal::set_real(imgui_g_maxVarId, channelFilters[1].max);
  ShaderGlobal::set_real(imgui_b_minVarId, channelFilters[2].min);
  ShaderGlobal::set_real(imgui_b_maxVarId, channelFilters[2].max);
  ShaderGlobal::set_real(imgui_a_minVarId, channelFilters[3].min);
  ShaderGlobal::set_real(imgui_a_maxVarId, channelFilters[3].max);
  ShaderGlobal::set_int(imgui_mip_levelVarId, max((int)(uintptr_t)cmd->UserCallbackData, 0));
  ShaderGlobal::set_int(imgui_use_custom_sampler, 1);
  ShaderGlobal::set_int(imgui_tex_type, texType);
  ShaderGlobal::set_real(imgui_slice, slice);
  ShaderGlobal::set_color4(imgui_cube_view_dirVarId, cubeViewMatrix.getcol(2));
  ShaderGlobal::set_color4(imgui_cube_view_upVarId, cubeViewMatrix.getcol(1));
  ShaderGlobal::set_color4(imgui_cube_view_rightVarId, cubeViewMatrix.getcol(0));

  auto diffTexId = get_managed_texture_id(selectedTextureNameForDiff.data());
  ShaderGlobal::set_texture(imgui_tex_diff, diffTexId);

  d3d::set_sampler(STAGE_PS, custom_sampler_const_no, pointSampler);
}

static void reset_image_mode(const ImDrawList *, const ImDrawCmd *)
{
  auto entry = get_filtered_texture();
  if (!entry)
    return;

  ShaderGlobal::set_int(imgui_channel_maskVarId, 15);
  ShaderGlobal::set_real(imgui_r_minVarId, 0);
  ShaderGlobal::set_real(imgui_r_maxVarId, 1);
  ShaderGlobal::set_real(imgui_g_minVarId, 0);
  ShaderGlobal::set_real(imgui_g_maxVarId, 1);
  ShaderGlobal::set_real(imgui_b_minVarId, 0);
  ShaderGlobal::set_real(imgui_b_maxVarId, 1);
  ShaderGlobal::set_real(imgui_a_minVarId, 0);
  ShaderGlobal::set_real(imgui_a_maxVarId, 1);
  ShaderGlobal::set_int(imgui_mip_levelVarId, 0);
  ShaderGlobal::set_int(imgui_use_custom_sampler, 0);
  ShaderGlobal::set_int(imgui_tex_type, 0);
  ShaderGlobal::set_texture(imgui_tex_diff, BAD_TEXTUREID);
}

static String mip_name(int mipLevel)
{
  if (mipLevel < 0)
    return String(16, "All mip levels");

  return String(16, "Mip level %d", mipLevel);
}

static String array_slice_name(int slice) { return String(16, "Array slice %d", slice); }

static String depth_slice_name(int slice) { return String(16, "Depth slice %d", slice); }

static String cube_slice_name(int slice)
{
  switch (slice)
  {
    case 0: return String(16, "Positive X");
    case 1: return String(16, "Negative X");
    case 2: return String(16, "Positive Y");
    case 3: return String(16, "Negative Y");
    case 4: return String(16, "Positive Z");
    case 5: return String(16, "Negative Z");
    case 6: return String(16, "Free view");
    default: return String(16, "");
  }
}


static void render_selected_texture()
{
  auto entry = get_filtered_texture();
  if (!entry)
    return;

  auto texId = get_managed_texture_id(selectedTextureName.data());
  if (texId == BAD_TEXTUREID || get_managed_texture_refcount(texId) == 0)
    return;

  auto &formatDesc = get_tex_format_desc(entry->cflg);
  auto formatName = get_tex_format_name(entry->cflg);

  ImGui::Text("Name: %s", selectedTextureName.c_str());
  ImGui::Text("Type: %s", get_tex_type_name(entry->resType));
  ImGui::SameLine();
  ImGui::Text("Format: %s", formatName);
  ImGui::SameLine();
  ImGui::Text("Width: %d", entry->w);
  ImGui::SameLine();
  ImGui::Text("Height: %d", entry->h);
  switch (entry->resType)
  {
    case RES3D_VOLTEX:
      ImGui::SameLine();
      ImGui::Text("Depth: %d", entry->d);
      break;
    case RES3D_ARRTEX:
    case RES3D_CUBEARRTEX:
      ImGui::SameLine();
      ImGui::Text("Slices: %d", entry->a);
      break;
    default: break;
  }

  ImGui::SameLine();
  ImGui::Text("Mip levels: %d", entry->mipLevels);

  ImGui::BeginGroup();

  int tailSize = filter_count(formatDesc) > 1 ? 40 : 0;

  if (render_channel_filter("R", channelFilters[0], formatDesc.r, tailSize) && rgbFiltersLocked)
  {
    channelFilters[1].min = channelFilters[0].min;
    channelFilters[1].max = channelFilters[0].max;
    channelFilters[2].min = channelFilters[0].min;
    channelFilters[2].max = channelFilters[0].max;
  }
  if (render_channel_filter("G", channelFilters[1], formatDesc.g, tailSize) && rgbFiltersLocked)
  {
    channelFilters[0].min = channelFilters[1].min;
    channelFilters[0].max = channelFilters[1].max;
    channelFilters[2].min = channelFilters[1].min;
    channelFilters[2].max = channelFilters[1].max;
  }
  if (render_channel_filter("B", channelFilters[2], formatDesc.b, tailSize) && rgbFiltersLocked)
  {
    channelFilters[0].min = channelFilters[2].min;
    channelFilters[0].max = channelFilters[2].max;
    channelFilters[1].min = channelFilters[2].min;
    channelFilters[1].max = channelFilters[2].max;
  }

  ImGui::EndGroup();

  if (tailSize > 0)
  {
    ImGui::SameLine();

    auto filterHeight = ImGui::GetItemRectSize().y;
    auto id = rgbFiltersLocked ? lockImage.getTexId() : unlockImage.getTexId();
    auto buttonSize =
      ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, filterHeight - ImGui::GetStyle().ItemSpacing.y);
    auto buttonAspect = buttonSize.x / buttonSize.y;
    auto uv0 = ImVec2(0, buttonAspect - 1);
    auto uv1 = ImVec2(1, 1 - uv0.y);

    ImGui::GetWindowDrawList()->AddCallback(set_image_mode, (void *)(uintptr_t)-1);

    if (ImGui::ImageButton("texDebugImage", reinterpret_cast<ImTextureID>(unsigned(id)), buttonSize, uv0, uv1))
    {
      rgbFiltersLocked = !rgbFiltersLocked;
      if (rgbFiltersLocked)
      {
        channelFilters[1].min = channelFilters[0].min;
        channelFilters[1].max = channelFilters[0].max;
        channelFilters[2].min = channelFilters[0].min;
        channelFilters[2].max = channelFilters[0].max;
      }
    }
    ImGui::GetWindowDrawList()->AddCallback(reset_image_mode, nullptr);
  }

  render_channel_filter("A", channelFilters[3], formatDesc.a, 0);
  render_channel_filter("Depth", channelFilters[0], formatDesc.depth, 0);

  if (ImGui::Button("Zoom out"))
  {
    uvTL = Point2::ZERO;
    uvBR = Point2(1, 1);
    cubeViewMatrix.identity();
  }

  ImGui::SameLine();
  if (ImGui::Button("[0..1]"))
    for (auto &filter : channelFilters)
    {
      filter.min = 0;
      filter.max = 1;
    }

  ImGui::SameLine();
  if (ImGui::Button("[auto]"))
    pickBufferQueryCalcRangesStart = true;

  if (entry->mipLevels > 1)
  {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x / 2);
    if (ImGui::BeginCombo("##mipSelector", mip_name(selectedMipLevel)))
    {
      for (int mipIx = 0; mipIx < entry->mipLevels; ++mipIx)
        if (ImGui::Selectable(mip_name(mipIx), selectedMipLevel == mipIx))
          selectedMipLevel = mipIx;

      if (ImGui::Selectable("All mip levels", selectedMipLevel == -1))
        selectedMipLevel = -1;

      ImGui::EndCombo();
    }
  }

  if (entry->a > 1)
  {
    auto slice_name_func = entry->resType == RES3D_ARRTEX ? array_slice_name : cube_slice_name;
    auto slice_count = entry->resType == RES3D_ARRTEX ? entry->a : 7;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(entry->mipLevels > 1 ? ImGui::GetContentRegionAvail().x : ImGui::GetContentRegionAvail().x / 2);
    if (ImGui::BeginCombo("##arraySliceSelector", slice_name_func(selectedArraySlice)))
    {
      for (int sliceIx = 0; sliceIx < slice_count; ++sliceIx)
        if (ImGui::Selectable(slice_name_func(sliceIx), selectedArraySlice == sliceIx))
          selectedArraySlice = sliceIx;

      ImGui::EndCombo();
    }
  }

  if (entry->d > 1)
  {
    ImGui::SameLine();
    if (ImGui::BeginCombo("##depthSliceSelector", depth_slice_name(selectedDepthSlice)))
    {
      for (int sliceIx = 0; sliceIx < entry->d; ++sliceIx)
        if (ImGui::Selectable(depth_slice_name(sliceIx), selectedDepthSlice == sliceIx))
          selectedDepthSlice = sliceIx;

      ImGui::EndCombo();
    }
  }

  ImGui::Separator();

  ImGui::GetWindowDrawList()->AddCallback(set_image_mode, (void *)(uintptr_t)max(selectedMipLevel, 0));

  float aspect = float(entry->w) / entry->h;
  int width = ImGui::GetContentRegionAvail().x;
  int height = width / aspect;

  auto imagePosition = ImGui::GetCursorScreenPos();
  ImGui::Image(reinterpret_cast<ImTextureID>(unsigned(texId)), ImVec2(width, height), uvTL, uvBR);

  ImGui::GetWindowDrawList()->AddCallback(reset_image_mode, nullptr);

  auto &io = ImGui::GetIO();
  static ImGuiIO lastIO;
  static bool isZooming = false;
  static bool isPanning = false;
  static bool isRotating = false;
  static Point2 zoomCenter;

  auto dMouse = Point2(io.MousePos.x - lastIO.MousePos.x, io.MousePos.y - lastIO.MousePos.y);
  auto relPosition = Point2(io.MousePos.x - imagePosition.x, io.MousePos.y - imagePosition.y);
  auto uvRange = uvBR - uvTL;

  cursorUV = uvTL + Point2(uvRange.x * relPosition.x / width, uvRange.y * relPosition.y / height);


  if (ImGui::IsItemHovered())
  {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
      zoomCenter = cursorUV;
      isZooming = true;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      isPanning = true;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
      isRotating = true;
  }

  if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && isZooming)
    isZooming = false;
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && isPanning)
    isPanning = false;
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle) && isRotating)
    isRotating = false;

  if (isZooming && dMouse.x != 0)
  {
    auto zoom = 1.0f - dMouse.x * 0.001f;
    uvTL = (uvTL - zoomCenter) * zoom + zoomCenter;
    uvBR = (uvBR - zoomCenter) * zoom + zoomCenter;
  }

  if (isPanning && (dMouse.x != 0 || dMouse.y != 0))
  {
    auto pixelToUvRatio = uvRange.x / width;
    auto move = dMouse * pixelToUvRatio;
    uvTL -= move;
    uvBR -= move;
  }

  if (isRotating && (dMouse.x != 0 || dMouse.y != 0))
  {
    Point3 xaxis = cubeViewMatrix.getcol(0);
    Point3 yaxis = cubeViewMatrix.getcol(1);

    if (dMouse.x)
    {
      TMatrix rot;
      rot.makeTM(yaxis, dMouse.x * 0.01f);
      cubeViewMatrix = rot * cubeViewMatrix;
    }
    if (dMouse.y)
    {
      TMatrix rot;
      rot.makeTM(xaxis, dMouse.y * 0.01f);
      cubeViewMatrix = rot * cubeViewMatrix;
    }
  }

  clip_uv();

  lastIO = io;

  if (selectedMipLevel < 0)
  {
    int width = (ImGui::GetContentRegionAvail().x / 3) - 2 * ImGui::GetStyle().ItemSpacing.x;
    int height = width / aspect;
    for (int mipIx = 1; mipIx < entry->mipLevels; ++mipIx)
    {
      if ((mipIx - 1) % 3)
        ImGui::SameLine();

      ImGui::GetWindowDrawList()->AddCallback(set_image_mode, (void *)(uintptr_t)mipIx);
      ImGui::Image(reinterpret_cast<ImTextureID>(unsigned(texId)), ImVec2(width, height), uvTL, uvBR);
    }

    ImGui::GetWindowDrawList()->AddCallback(reset_image_mode, nullptr);
  }

  if (pickBufferQueryActive && d3d::get_event_query_status(pickBufferQuery.get(), false))
  {
    if (auto bufferData = lock_sbuffer<const Point4>(pickBuffer.getBuf(), 0, 0, VBLOCK_READONLY))
    {
      pickColor = bufferData[0];

      if (pickBufferQueryCalcRangesRunning)
      {
        pickColorMin = bufferData[1];
        pickColorMax = bufferData[2];

        for (int i = 0; i < 4; ++i)
        {
          channelFilters[i].min = pickColorMin[i];
          channelFilters[i].max = pickColorMax[i];
        }

        pickBufferQueryCalcRangesRunning = false;
      }
    }

    pickBufferQueryActive = false;
  }

  if (!pickBufferQueryActive && processTextureShader)
  {
    auto diffTexId = get_managed_texture_id(selectedTextureNameForDiff.data());

    int texType = tex_type_ix(*entry);
    float slice = tex_slice_val(*entry);
    int sliceI = tex_sliceI_val(*entry);

    ShaderGlobal::set_texture(tex_debug_textureVarId, texId);
    ShaderGlobal::set_texture(tex_debug_texture_for_diffVarId, diffTexId);
    ShaderGlobal::set_buffer(tex_debug_readback_bufferVarId, pickBuffer.getBufId());
    ShaderGlobal::set_color4(tex_debug_readback_uvVarId, cursorUV);
    ShaderGlobal::set_int(tex_debug_mip_levelVarId, max(selectedMipLevel, 0));
    ShaderGlobal::set_int(tex_debug_typeVarId, texType);
    ShaderGlobal::set_real(tex_debug_sliceVarId, slice);
    ShaderGlobal::set_int(tex_debug_sliceIVarId, sliceI);
    ShaderGlobal::set_int(tex_debug_rangesVarId, pickBufferQueryCalcRangesStart ? 1 : 0);

    d3d::set_sampler(STAGE_CS, 5, pointSampler);

    processTextureShader->dispatch(1, 1, 1);

    if (pickBuffer->lock(0, 0, (void **)nullptr, VBLOCK_READONLY))
      pickBuffer->unlock();
    d3d::issue_event_query(pickBufferQuery.get());
    pickBufferQueryActive = true;
    pickBufferQueryCalcRangesRunning = pickBufferQueryCalcRangesStart;
    pickBufferQueryCalcRangesStart = false;
  }

  if (processTextureShader)
  {
    String colorText(0, "Color: ");
    if (formatDesc.r.bits > 0)
      colorText += String(0, "R: %.3f ", pickColor[0]);
    if (formatDesc.g.bits > 0)
      colorText += String(0, "G: %.3f ", pickColor[1]);
    if (formatDesc.b.bits > 0)
      colorText += String(0, "B: %.3f ", pickColor[2]);
    if (formatDesc.a.bits > 0)
      colorText += String(0, "A: %.3f ", pickColor[3]);
    if (formatDesc.depth.bits > 0)
      colorText += String(0, "Depth: %.8f ", pickColor[0]);
    ImGui::Text("%s", colorText.data());
  }

  // For debugging uv handling
  // ImGui::Text("TL UV: %.3f, %.3f", uvTL.x, uvTL.y);
  // ImGui::Text("BR UV: %.3f, %.3f", uvBR.x, uvBR.y);
  // ImGui::Text("Cursor UV: %.3f, %.3f", cursorUV.x, cursorUV.y);
  // ImGui::Text("Zoom center: %.3f, %.3f", zoomCenter.x, zoomCenter.y);

  // For min/max color debugging
  // ImGui::Text("MinValue: %.6f, %.6f, %.6f, %.6f", pickColorMin.x, pickColorMin.y, pickColorMin.z, pickColorMin.w);
  // ImGui::Text("MaxValue: %.6f, %.6f, %.6f, %.6f", pickColorMax.x, pickColorMax.y, pickColorMax.z, pickColorMax.w);
}

static void imguiWindow()
{
  if (ImGui::Button("Reload"))
  {
    refresh_textures();
    update_filtered_textures();
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("RT", &filterRT))
    update_filtered_textures();
  ImGui::SameLine();
  if (ImGui::Checkbox("Depth", &filterDepth))
    update_filtered_textures();
  ImGui::SameLine();
  if (ImGui::RadioButton("2D", filterTexType == RES3D_TEX))
  {
    filterTexType = RES3D_TEX;
    update_filtered_textures();
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("3D", filterTexType == RES3D_VOLTEX))
  {
    filterTexType = RES3D_VOLTEX;
    update_filtered_textures();
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Cube", filterTexType == RES3D_CUBETEX))
  {
    filterTexType = RES3D_CUBETEX;
    update_filtered_textures();
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Array", filterTexType == RES3D_ARRTEX))
  {
    filterTexType = RES3D_ARRTEX;
    update_filtered_textures();
  }

  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::InputTextWithHint("##texDebugFilter", "Filter...", filterName, sizeof(filterName)))
    update_filtered_textures();

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Diff texture").x);
  if (ImGui::BeginCombo("Diff texture", selectedTextureNameForDiff.empty() ? "##empty" : selectedTextureNameForDiff.data()))
  {
    if (ImGui::Selectable("##empty", selectedTextureNameForDiff.empty()))
    {
      selectedTextureNameForDiff.clear();
      reset_view();
    }

    for (auto &name : filteredTextures)
    {
      auto entry = textures.find(name);
      if (entry == textures.end())
        continue;
      if (!entry->first.empty() && ImGui::Selectable(entry->first.data(), entry->first == selectedTextureNameForDiff))
      {
        selectedTextureNameForDiff = entry->first;
        reset_view();
      }
    }

    ImGui::EndCombo();
  }

  bool needSameLine = false;

  if (!selectedTextureName.empty())
  {
    bool isPinned = pinnedTextures.find(selectedTextureName) != pinnedTextures.end();

    if (isPinned && ImGui::Button("Unpin"))
    {
      pinnedTextures.erase(selectedTextureName);
      save_pinned_list();
    }
    else if (!isPinned && ImGui::Button("Pin"))
    {
      pinnedTextures.insert(selectedTextureName);
      save_pinned_list();
    }

    needSameLine = true;
  }

  if (!filteredTextures.empty() || filterName[0] != 0)
    for (auto &pin : pinnedTextures)
    {
      if (needSameLine)
        ImGui::SameLine();

      needSameLine = true;

      if (ImGui::Button(pin.data()))
      {
        selectedTextureName = pin;
        reset_view();
      }
    }

  ImGui::Separator();

  ImVec2 wSize = ImGui::GetContentRegionAvail();
  ImVec2 cursorPos = ImGui::GetCursorPos();

  ImGui::BeginChild("##TextureSelectionChild", ImVec2(wSize.x * 0.25, wSize.y));
  ImGui::Text("Textures:");

  if ((!filteredTextures.empty() || filterName[0] != 0) &&
      ImGui::BeginListBox("##TextureSelection", ImVec2(wSize.x * 0.25 - 10, wSize.y - ImGui::CalcTextSize("Textures:").y - 5)))
  {
    if (ImGui::Selectable("##empty", selectedTextureName.empty()))
    {
      selectedTextureName.clear();
      reset_view();
    }
    for (auto &name : filteredTextures)
    {
      auto entry = textures.find(name);
      if (entry == textures.end())
        continue;

      bool selected = entry->first == selectedTextureName;

      if (selected && selectableNeedsFocus)
      {
        ImGui::SetScrollHereY();
        selectableNeedsFocus = false;
      }

      if (!entry->first.empty() && ImGui::Selectable(entry->first.data(), selected))
      {
        selectedTextureName = entry->first;
        reset_view();
      }
    }
    ImGui::EndListBox();
  }
  else
  {
    ImGui::Text("No textures loaded yet");
  }
  ImGui::EndChild();

  ImGui::SetCursorPos(ImVec2(cursorPos.x + wSize.x * 0.25, cursorPos.y));
  ImGui::BeginChild("##Rest", ImVec2(wSize.x * 0.75 - 5, wSize.y));

  render_selected_texture();

  ImGui::EndChild();
}

void init()
{
  imgui_channel_maskVarId = get_shader_variable_id("imgui_channel_mask", true);
  imgui_r_minVarId = get_shader_variable_id("imgui_r_min", true);
  imgui_r_maxVarId = get_shader_variable_id("imgui_r_max", true);
  imgui_g_minVarId = get_shader_variable_id("imgui_g_min", true);
  imgui_g_maxVarId = get_shader_variable_id("imgui_g_max", true);
  imgui_b_minVarId = get_shader_variable_id("imgui_b_min", true);
  imgui_b_maxVarId = get_shader_variable_id("imgui_b_max", true);
  imgui_a_minVarId = get_shader_variable_id("imgui_a_min", true);
  imgui_a_maxVarId = get_shader_variable_id("imgui_a_max", true);
  imgui_mip_levelVarId = get_shader_variable_id("imgui_mip_level", true);
  imgui_cube_view_dirVarId = get_shader_variable_id("imgui_cube_view_dir", true);
  imgui_cube_view_upVarId = get_shader_variable_id("imgui_cube_view_up", true);
  imgui_cube_view_rightVarId = get_shader_variable_id("imgui_cube_view_right", true);
  imgui_use_custom_sampler = get_shader_variable_id("imgui_use_custom_sampler", true);
  imgui_tex_type = get_shader_variable_id("imgui_tex_type", true);
  imgui_slice = get_shader_variable_id("imgui_slice", true);
  imgui_tex_diff = get_shader_variable_id("imgui_tex_diff", true);
  tex_debug_textureVarId = get_shader_variable_id("tex_debug_texture", true);
  tex_debug_texture_for_diffVarId = get_shader_variable_id("tex_debug_texture_for_diff", true);
  tex_debug_readback_bufferVarId = get_shader_variable_id("tex_debug_readback_buffer", true);
  tex_debug_readback_uvVarId = get_shader_variable_id("tex_debug_readback_uv", true);
  tex_debug_mip_levelVarId = get_shader_variable_id("tex_debug_mip_level", true);
  tex_debug_sliceVarId = get_shader_variable_id("tex_debug_slice", true);
  tex_debug_sliceIVarId = get_shader_variable_id("tex_debug_sliceI", true);
  tex_debug_typeVarId = get_shader_variable_id("tex_debug_type", true);
  tex_debug_rangesVarId = get_shader_variable_id("tex_debug_ranges", true);

  processTextureShader = new_compute_shader("tex_debug", true);

  refresh_textures();
  channelFilters[3].active = false;

  if (processTextureShader)
  {
    pickBuffer = dag::buffers::create_ua_structured_readback(sizeof(Point4), 3, "texDebugPickBuffer");
    pickBufferQuery.reset(d3d::create_event_query());
  }

  d3d::SamplerInfo psi;
  psi.filter_mode = d3d::FilterMode::Point;
  pointSampler = d3d::request_sampler(psi);

  d3d::SamplerInfo bsi;
  bsi.address_mode_u = d3d::AddressMode::Border;
  bsi.address_mode_v = d3d::AddressMode::Border;
  borderSampler = d3d::request_sampler(bsi);

  ImageInfoDDS padlock_lock_icon_dds;
  load_dds((void *)padlock_lock_icon, sizeof(padlock_lock_icon), 1, 0, padlock_lock_icon_dds);
  lockImage = resptr_detail::ResPtrFactory(create_dds_texture(false, padlock_lock_icon_dds, "texDebug_lockImage"));
  lockImage->texaddr(TEXADDR_BORDER);

  ImageInfoDDS padlock_unlock_icon_dds;
  load_dds((void *)padlock_unlock_icon, sizeof(padlock_unlock_icon), 1, 0, padlock_unlock_icon_dds);
  unlockImage = resptr_detail::ResPtrFactory(create_dds_texture(false, padlock_unlock_icon_dds, "texDebug_unlockImage"));
  unlockImage->texaddr(TEXADDR_BORDER);

#if _TARGET_PC | _TARGET_XBOX
  if (::dd_file_exist("tex_debug.blk"))
  {
    DataBlock blk;
    blk.load("tex_debug.blk");

    int pinnedNameId = blk.getNameId("pinned");

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == pinnedNameId && blk.getParamType(i) == DataBlock::TYPE_STRING)
        pinnedTextures.insert(blk.getStr(i));
  }
#endif
}

void teardown()
{
  textures.clear();
  filteredTextures.clear();
  pickBuffer.close();
  pickBufferQuery.reset();
  processTextureShader.destroy();
  lockImage.close();
  unlockImage.close();
}

void select_texture(const char *name)
{
  if (strcmp(name, "") == 0)
  {
    imgui_window_set_visible("Render", "Texture debug", false);
    return;
  }

  imgui_window_set_visible("Render", "Texture debug", true);
  refresh_textures();
  update_filtered_textures();
  selectedTextureName = name;
  selectableNeedsFocus = true;
}

} // namespace texdebug

REGISTER_IMGUI_WINDOW("Render", "Texture debug", texdebug::imguiWindow);
