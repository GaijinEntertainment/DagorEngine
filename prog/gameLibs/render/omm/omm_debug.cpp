// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omm.h>

#if DAGOR_DBGLEVEL > 0

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui.h>
#include <math/dag_Point4.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVar.h>
#include <util/dag_string.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>

#include "shaders/omm_debug_uv_record.hlsli"
#include <math.h>

namespace render::omm
{

extern uint32_t texcoord_format_to_shader_value(TexCoordFormat format);
extern d3d::SamplerHandle request_runtime_sampler(const SamplerDesc &desc);

namespace
{

static constexpr ResourceTagType OMM_DEBUG_RESOURCE_TAG = "omm_debug";
static constexpr int TILE_SIDE = 96;
static constexpr float TILE_GAP_RATIO = 0.1f;
static constexpr float EQUILATERAL_TRIANGLE_HEIGHT = 0.8660254038f;
static const ImVec4 FAILED_COLOR = ImVec4(1.f, 0.38f, 0.35f, 1.f);
// An adopted entry owns its buffers and no producer unregisters it, thus the count needs a cap.
static constexpr int MAX_RETAINED_FAILED_ENTRIES = 64;

struct DebugEntry
{
  // Exactly one is in use; read through result(). A borrowed entry is keyed for removal by the address
  // of the producer's result.
  const BakeResult *borrowed = nullptr;
  BakeResult adopted;

  const BakeResult &result() const { return borrowed ? *borrowed : adopted; }
  bool isAdopted() const { return borrowed == nullptr; }

  DebugBakeResultInfo info;
  String label;
  String failReason; // this string is empty for a bake that made a usable OMM

  // Decoded at the registration: the producer owns the source vertex buffer, and the object of a failed
  // bake is usually destroyed right after.
  UniqueBuf triangleUvs;
  // An adopted entry outlives its producer. Without a reference of its own, the id could be released and
  // reused for a different texture.
  TEXTUREID heldAlphaTexture = BAD_TEXTUREID;
};

dag::Vector<DebugEntry> entries;
int selectedEntry = -1;
ImVec2 viewOrigin = ImVec2(0.f, 0.f);
float viewScale = 1.f;
bool autoRefresh = true;
bool needsRefresh = true;
bool fitViewOnNextDraw = true;
bool scrollSelectionOnNextDraw = false;
char entryFilter[128] = "";

Ptr<ComputeShaderElement> visualizeShader;
UniqueTex visualizationTex;
int visualizationWidth = 0;
int visualizationHeight = 0;

ShaderVariableInfo targetVar("omm_debug_target", true);
ShaderVariableInfo arrayDataVar("omm_debug_array_data", true);
ShaderVariableInfo descArrayVar("omm_debug_desc_array", true);
ShaderVariableInfo indexBufferVar("omm_debug_index_buffer", true);
ShaderVariableInfo indexFormatVar("omm_debug_index_format", true);
ShaderVariableInfo indexCountVar("omm_debug_index_count", true);
ShaderVariableInfo descCountVar("omm_debug_desc_count", true);
ShaderVariableInfo arrayDataSizeVar("omm_debug_array_data_size", true);
ShaderVariableInfo tileSizeVar("omm_debug_tile_size", true);
ShaderVariableInfo gridColumnsVar("omm_debug_grid_columns", true);
ShaderVariableInfo viewParamsVar("omm_debug_view_params", true);
ShaderVariableInfo showGridVar("omm_debug_show_grid", true);
ShaderVariableInfo gridThicknessVar("omm_debug_grid_thickness", true);
ShaderVariableInfo alphaTexVar("omm_debug_alpha_tex", true);
ShaderVariableInfo alphaSamplerVar("omm_debug_alpha_tex_samplerstate", true);
ShaderVariableInfo uvBufferVar("omm_debug_uv_buffer", true);
ShaderVariableInfo showTextureVar("omm_debug_show_texture", true);
ShaderVariableInfo alphaChannelVar("omm_debug_alpha_channel", true);
ShaderVariableInfo alphaCutoffVar("omm_debug_alpha_cutoff", true);
ShaderVariableInfo cutoutLinesVar("omm_debug_cutout_lines", true);
ShaderVariableInfo cutoutEnabledVar("omm_debug_cutout_enabled", true);
ShaderVariableInfo textureOpacityVar("omm_debug_texture_opacity", true);
ShaderVariableInfo textureThresholdVar("omm_debug_texture_threshold", true);
ShaderVariableInfo showPredictedGridVar("omm_debug_show_predicted_grid", true);

ShaderVariableInfo srcTexcoordsVar("omm_debug_src_texcoords", true);
ShaderVariableInfo srcIndicesVar("omm_debug_src_indices", true);
ShaderVariableInfo uvOutVar("omm_debug_uv_out", true);
ShaderVariableInfo uvTcFormatVar("omm_debug_uv_tc_format", true);
ShaderVariableInfo uvTcOffsetVar("omm_debug_uv_tc_offset", true);
ShaderVariableInfo uvTcStrideVar("omm_debug_uv_tc_stride", true);
ShaderVariableInfo uvIndexFormatVar("omm_debug_uv_index_format", true);
ShaderVariableInfo uvIndexOffsetVar("omm_debug_uv_index_offset", true);
ShaderVariableInfo uvIndexStrideVar("omm_debug_uv_index_stride", true);
ShaderVariableInfo uvTriangleCountVar("omm_debug_uv_triangle_count", true);
ShaderVariableInfo uvPredictParamsVar("omm_debug_uv_predict_params", true);

bool showMicroTriangleGrid = true;
float gridThickness = 1.5f;
bool showAlphaTexture = false;
bool thresholdAlphaTexture = true;
float textureOpacity = 0.5f;
bool showPredictedGrid = true;

Ptr<ComputeShaderElement> extractUvShader;

static bool is_failed(const DebugEntry &entry) { return !entry.failReason.empty(); }

static const char *index_format_name(IndexFormat format)
{
  switch (format)
  {
    case IndexFormat::UINT8: return "u8";
    case IndexFormat::UINT16: return "u16";
    case IndexFormat::UINT32: return "u32";
  }
  return "?";
}

// The same rule the SDK uses: a zero stride means the indices are packed to the format size.
static uint32_t index_stride(const DebugBakeSource &source)
{
  if (source.indexStrideInBytes)
    return source.indexStrideInBytes;
  switch (source.indexFormat)
  {
    case IndexFormat::UINT8: return 1;
    case IndexFormat::UINT16: return 2;
    case IndexFormat::UINT32: return 4;
  }
  return 4;
}

static const char *format_name(raytrace::OpacityMicroMapFormat format)
{
  switch (format)
  {
    case raytrace::OpacityMicroMapFormat::OpacityCompression1_2State: return "OC1_2";
    case raytrace::OpacityMicroMapFormat::OpacityCompression1_4State: return "OC1_4";
  }
  return "?";
}

static bool init_shader()
{
  if (!visualizeShader)
    visualizeShader = new_compute_shader("omm_debug_visualize", true);
  if (!visualizeShader)
    return false;

  return true;
}

// This runs at the registration and not at the selection: see DebugEntry::triangleUvs.
static void snapshot_triangle_uvs(DebugEntry &entry)
{
  const DebugBakeSource &source = entry.info.source;
  // BakeResult::indexCount is one OMM index for each triangle, not for each vertex index.
  const uint32_t triangleCount = entry.result().indexCount;
  if (!source.texCoordBuffer || !source.indexBuffer || triangleCount == 0 || source.texCoordStrideInBytes == 0)
    return;

  if (!extractUvShader)
    extractUvShader = new_compute_shader("omm_debug_extract_uv", true);
  if (!extractUvShader)
    return;

  // The size of the alpha source in texels, thus the predicted level uses the same units as the bake.
  float texelsW = 1.f, texelsH = 1.f;
  if (source.alphaTextureId != BAD_TEXTUREID)
    if (BaseTexture *tex = acquire_managed_tex(source.alphaTextureId))
    {
      TextureInfo ti;
      tex->getinfo(ti, 0);
      texelsW = ti.w > 0 ? float(ti.w) : 1.f;
      texelsH = ti.h > 0 ? float(ti.h) : 1.f;
      release_managed_tex(source.alphaTextureId);
    }

  // A UniqueRes is keyed by its name, thus two entries with the same name would share one buffer. The
  // serial makes it unique: the same slot can bake again while an adopted entry for it is still there.
  static uint32_t snapshotSerial = 0;
  const String bufferName(0, "omm_debug_triangle_uvs_%llX_%u_%u_%u", static_cast<unsigned long long>(entry.info.objectId),
    entry.info.geometryIndex, entry.info.slotId, ++snapshotSerial);
  entry.triangleUvs = dag::buffers::create_ua_sr_byte_address(triangleCount * (OMM_DEBUG_UV_RECORD_SIZE / 4), bufferName.c_str(),
    dag::buffers::Init::No, OMM_DEBUG_RESOURCE_TAG);
  if (!entry.triangleUvs)
    return;

  // These buffers come from the producer and have no managed id to bind with.
  srcTexcoordsVar.set_buffer(source.texCoordBuffer);
  srcIndicesVar.set_buffer(source.indexBuffer);
  uvOutVar.set_buffer(entry.triangleUvs.getBufId());
  // The same value as the bake shaders got, thus the two decode in the same manner.
  uvTcFormatVar.set_int(int(texcoord_format_to_shader_value(source.texCoordFormat)));
  uvTcOffsetVar.set_int(int(source.texCoordOffsetInBytes));
  uvTcStrideVar.set_int(int(source.texCoordStrideInBytes));
  uvIndexFormatVar.set_int(int(source.indexFormat));
  uvIndexOffsetVar.set_int(int(source.indexBufferOffsetInBytes));
  uvIndexStrideVar.set_int(int(index_stride(source)));
  uvTriangleCountVar.set_int(int(triangleCount));
  uvPredictParamsVar.set_float4(texelsW, texelsH, source.dynamicSubdivisionScale, float(source.maxSubdivisionLevel));

  extractUvShader->dispatchThreads(triangleCount, 1, 1);

  srcTexcoordsVar.set_buffer(static_cast<Sbuffer *>(nullptr));
  srcIndicesVar.set_buffer(static_cast<Sbuffer *>(nullptr));
  uvOutVar.set_buffer(BAD_D3DRESID);
}

static bool ensure_visualization_tex(int width, int height)
{
  if (visualizationTex)
  {
    TextureInfo info;
    visualizationTex->getinfo(info);
    if (info.w == width && info.h == height)
      return true;
    visualizationTex.close();
  }

  visualizationTex = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "omm_debug_visualization",
    OMM_DEBUG_RESOURCE_TAG);
  visualizationWidth = width;
  visualizationHeight = height;
  needsRefresh = true;
  return bool(visualizationTex);
}

static bool can_visualize(const DebugEntry &entry)
{
  const BakeResult &result = entry.result();
  return result.arrayData && result.descArray && result.indexBuffer && result.indexCount > 0 &&
         result.descArraySizeInBytes >= sizeof(raytrace::InBufferOpacityMicroMapDescription);
}

static int get_grid_columns(const DebugEntry &entry)
{
  return eastl::max(1, int(ceilf(sqrtf(float(eastl::max(entry.result().indexCount, 1u))))));
}

static ImVec2 get_atlas_size(const DebugEntry &entry)
{
  const int columns = get_grid_columns(entry);
  const int rows = eastl::max(1, (int(entry.result().indexCount) + columns - 1) / columns);
  const float gap = float(TILE_SIDE) * TILE_GAP_RATIO;
  const float triangleHeight = float(TILE_SIDE) * EQUILATERAL_TRIANGLE_HEIGHT;
  return ImVec2(float(columns) * (float(TILE_SIDE) + gap) + gap, float(rows) * (triangleHeight + gap) + gap);
}

static void fit_atlas_to_view(const DebugEntry &entry, const ImVec2 &view_size)
{
  const ImVec2 atlasSize = get_atlas_size(entry);
  viewScale = eastl::min(view_size.x / eastl::max(atlasSize.x, 1.f), view_size.y / eastl::max(atlasSize.y, 1.f));
  viewScale = eastl::clamp(viewScale, 0.02f, 64.f);
  viewOrigin.x = -0.5f * eastl::max(view_size.x / viewScale - atlasSize.x, 0.f);
  viewOrigin.y = -0.5f * eastl::max(view_size.y / viewScale - atlasSize.y, 0.f);
  needsRefresh = true;
}

static void bind_visualization(const DebugEntry &entry)
{
  const BakeResult &result = entry.result();
  const DebugBakeSource &source = entry.info.source;

  targetVar.set_texture(visualizationTex.getTexId());
  arrayDataVar.set_buffer(result.arrayData.getBufId());
  descArrayVar.set_buffer(result.descArray.getBufId());
  indexBufferVar.set_buffer(result.indexBuffer.getBufId());
  indexFormatVar.set_int(int(result.indexFormat));
  indexCountVar.set_int(result.indexCount);
  descCountVar.set_int(result.descArraySizeInBytes / sizeof(raytrace::InBufferOpacityMicroMapDescription));
  arrayDataSizeVar.set_int(result.arrayDataSizeInBytes);
  tileSizeVar.set_int(TILE_SIDE);
  gridColumnsVar.set_int(get_grid_columns(entry));
  viewParamsVar.set_float4(viewOrigin.x, viewOrigin.y, viewScale, 0.f);
  showGridVar.set_int(showMicroTriangleGrid ? 1 : 0);
  gridThicknessVar.set_float(gridThickness);

  // The copy supplies the texture overlay and also the predicted grid, thus bind it for the two.
  const bool haveUvs = bool(entry.triangleUvs);
  if (haveUvs)
    uvBufferVar.set_buffer(entry.triangleUvs.getBufId());
  showPredictedGridVar.set_int(haveUvs && showPredictedGrid ? 1 : 0);

  const bool canShowTexture = showAlphaTexture && haveUvs && source.alphaTextureId != BAD_TEXTUREID;
  showTextureVar.set_int(canShowTexture ? 1 : 0);
  if (canShowTexture)
  {
    alphaTexVar.set_texture(source.alphaTextureId);
    // The bake's sampler, not the texture's own: an impostor bakes through a transparent border.
    alphaSamplerVar.set_sampler(request_runtime_sampler(source.runtimeSamplerDesc));
    alphaChannelVar.set_int(int(source.alphaTextureChannel));
    alphaCutoffVar.set_float(source.alphaCutoff);
    const Point4 &cutout = source.uvCutout.lines;
    cutoutLinesVar.set_float4(cutout.x, cutout.y, cutout.z, cutout.w);
    cutoutEnabledVar.set_int(source.uvCutout.enabled ? 1 : 0);
    textureOpacityVar.set_float(textureOpacity);
    textureThresholdVar.set_int(thresholdAlphaTexture ? 1 : 0);
  }
}

static void unbind_visualization()
{
  targetVar.set_texture(BAD_TEXTUREID);
  arrayDataVar.set_buffer(BAD_D3DRESID);
  descArrayVar.set_buffer(BAD_D3DRESID);
  indexBufferVar.set_buffer(BAD_D3DRESID);
  uvBufferVar.set_buffer(BAD_D3DRESID);
  alphaTexVar.set_texture(BAD_TEXTUREID);
}

static void draw_dispatch_callback(const ImDrawList *, const ImDrawCmd *)
{
  if (visualizeShader && visualizationTex)
    visualizeShader->dispatchThreads(visualizationWidth, visualizationHeight, 1);
}

static void draw_unbind_callback(const ImDrawList *, const ImDrawCmd *) { unbind_visualization(); }

static void close_visualization_texture()
{
  visualizationTex.close();
  visualizationWidth = 0;
  visualizationHeight = 0;
  needsRefresh = true;
}

static void clear_debug_resources()
{
  unbind_visualization();
  close_visualization_texture();
  visualizeShader = nullptr;
  extractUvShader = nullptr;
  for (DebugEntry &entry : entries)
    if (entry.heldAlphaTexture != BAD_TEXTUREID)
      release_managed_tex(entry.heldAlphaTexture);
  entries.clear();
  selectedEntry = -1;
  viewOrigin = ImVec2(0.f, 0.f);
  viewScale = 1.f;
  fitViewOnNextDraw = true;
  scrollSelectionOnNextDraw = false;
}

static void draw_histogram(const char *name, dag::ConstSpan<raytrace::OpacityMicroMapDescription> descs)
{
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (!ImGui::TreeNode(name))
    return;

  if (descs.empty())
    ImGui::TextUnformatted("empty");
  else if (ImGui::BeginTable(name, 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("count");
    ImGui::TableSetupColumn("subdivision");
    ImGui::TableSetupColumn("format");
    ImGui::TableHeadersRow();
    for (const raytrace::OpacityMicroMapDescription &desc : descs)
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%u", desc.count);
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", desc.subdivisionLevel);
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(format_name(desc.format));
    }
    ImGui::EndTable();
  }

  ImGui::TreePop();
}

static void select_entry(int index)
{
  selectedEntry = eastl::clamp(index, 0, int(entries.size()) - 1);
  viewOrigin = ImVec2(0.f, 0.f);
  viewScale = 1.f;
  fitViewOnNextDraw = true;
  scrollSelectionOnNextDraw = true;
  needsRefresh = true;
}

static void draw_entry(DebugEntry &entry)
{
  if (is_failed(entry))
  {
    ImGui::PushStyleColor(ImGuiCol_Text, FAILED_COLOR);
    ImGui::TextUnformatted("BAKE FAILED");
    ImGui::TextWrapped("%s", entry.failReason.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
  }

  const BakeResult &result = entry.result();
  const uint32_t descCount = result.descArraySizeInBytes / sizeof(raytrace::InBufferOpacityMicroMapDescription);
  const int columns = get_grid_columns(entry);
  const int rows = eastl::max(1, (int(result.indexCount) + columns - 1) / columns);
  ImGui::Text("index format: %s", index_format_name(result.indexFormat));
  ImGui::Text("triangles: %u, descs: %u, grid: %d x %d", result.indexCount, descCount, columns, rows);
  ImGui::Text("bytes: array %u, desc %u, index %u", result.arrayDataSizeInBytes, result.descArraySizeInBytes,
    result.indexBufferSizeInBytes);
  const char *impostorTag = entry.info.impostor ? ", impostor" : "";
  const char *secondaryTag = entry.info.secondary ? ", secondary" : "";
  if (entry.info.geometryIndex == NO_GEOMETRY_INDEX)
    ImGui::Text("no mesh identity, the label names the source%s%s", impostorTag, secondaryTag);
  else
    ImGui::Text("object: %llu, geometry: %u, slot: %u%s%s", static_cast<unsigned long long>(entry.info.objectId),
      entry.info.geometryIndex, entry.info.slotId, impostorTag, secondaryTag);

  draw_histogram("array build histogram", result.arrayBuildDescs);
  draw_histogram("BLAS linkage histogram", result.blasLinkageDescs);

  ImGui::Separator();

  ImGui::Checkbox("Auto refresh", &autoRefresh);
  ImGui::SameLine();
  if (ImGui::Button("Refresh"))
    needsRefresh = true;
  ImGui::SameLine();
  const bool fitRequested = ImGui::Button("Fit all");

  if (ImGui::Checkbox("Micro-triangle grid", &showMicroTriangleGrid))
    needsRefresh = true;
  if (showMicroTriangleGrid)
  {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderFloat("thickness", &gridThickness, 0.5f, 4.f, "%.1f px"))
      needsRefresh = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(fades out when denser than a few pixels -- zoom in)");
    if (ImGui::Checkbox("Predicted level for collapsed triangles", &showPredictedGrid))
      needsRefresh = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(amber; a triangle that collapsed to one state stores no level)");
    ImGui::Text("level ceiling the bake was given: %u, dynamic scale: %.1f", entry.info.source.maxSubdivisionLevel,
      entry.info.source.dynamicSubdivisionScale);
  }

  const bool hasAlphaSource = entry.triangleUvs && entry.info.source.alphaTextureId != BAD_TEXTUREID;
  if (!hasAlphaSource)
    ImGui::TextDisabled("Alpha texture overlay unavailable (no texcoord snapshot or no alpha texture).");
  else
  {
    if (ImGui::Checkbox("Alpha texture overlay", &showAlphaTexture))
      needsRefresh = true;
    if (showAlphaTexture)
    {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(120.f);
      if (ImGui::SliderFloat("opacity", &textureOpacity, 0.f, 1.f, "%.2f"))
        needsRefresh = true;
      ImGui::SameLine();
      if (ImGui::Checkbox("threshold at cutoff", &thresholdAlphaTexture))
        needsRefresh = true;
      ImGui::Text("channel: %c, cutoff: %.3f", "rgba"[eastl::min(entry.info.source.alphaTextureChannel, 3u)],
        entry.info.source.alphaCutoff);
    }
  }

  if (!can_visualize(entry))
  {
    // An adopted entry owns whatever the bake made; the retention flag was already on when it was
    // adopted, thus that advice cannot apply to it.
    if (entry.isAdopted())
      ImGui::TextUnformatted("The failed bake made no buffers to draw.");
    else
    {
      ImGui::TextUnformatted("Selected result has no live bake buffers.");
      ImGui::TextUnformatted("Enable graphics/bvhRetainOmmBakeResults or the producer's OMM retention flag to display it.");
    }
    return;
  }

  if (!init_shader())
  {
    ImGui::TextUnformatted("omm_debug_visualize shader is not available.");
    return;
  }

  ImVec2 canvasSize = ImGui::GetContentRegionAvail();
  canvasSize.x = eastl::max(canvasSize.x, 128.f);
  canvasSize.y = eastl::max(canvasSize.y, 128.f);
  const int canvasWidth = eastl::max(16, int(canvasSize.x) & ~15);
  const int canvasHeight = eastl::max(16, int(canvasSize.y) & ~15);

  if (fitRequested || fitViewOnNextDraw)
  {
    fit_atlas_to_view(entry, ImVec2(float(canvasWidth), float(canvasHeight)));
    fitViewOnNextDraw = false;
  }

  if (!ensure_visualization_tex(canvasWidth, canvasHeight))
  {
    ImGui::TextUnformatted("Failed to create visualization texture.");
    return;
  }

  const bool dispatchThisFrame = autoRefresh || needsRefresh;
  if (dispatchThisFrame)
  {
    bind_visualization(entry);
    ImGui::GetWindowDrawList()->AddCallback(draw_dispatch_callback, nullptr);
    ImGui::GetWindowDrawList()->AddCallback(draw_unbind_callback, nullptr);
    needsRefresh = false;
  }

  ImVec2 imagePos = ImGui::GetCursorScreenPos();
  ImGuiDagor::Image(visualizationTex.getTexId(), canvasWidth, canvasHeight);
  if (ImGui::IsItemHovered())
  {
    const ImGuiIO &io = ImGui::GetIO();
    if (io.MouseWheel != 0.f)
    {
      const ImVec2 mouseLocal = ImVec2(io.MousePos.x - imagePos.x, io.MousePos.y - imagePos.y);
      const ImVec2 before = ImVec2(viewOrigin.x + mouseLocal.x / viewScale, viewOrigin.y + mouseLocal.y / viewScale);
      viewScale = eastl::clamp(viewScale * powf(1.2f, io.MouseWheel), 0.02f, 64.f);
      viewOrigin = ImVec2(before.x - mouseLocal.x / viewScale, before.y - mouseLocal.y / viewScale);
      needsRefresh = true;
    }
  }
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
  {
    const ImGuiIO &io = ImGui::GetIO();
    viewOrigin.x -= io.MouseDelta.x / viewScale;
    viewOrigin.y -= io.MouseDelta.y / viewScale;
    needsRefresh = true;
  }

  ImGui::Text("origin: %.1f, %.1f  zoom: %.3f", viewOrigin.x, viewOrigin.y, viewScale);
}

// fzf-style, case-insensitive: each character of `pattern` must occur in `str`, in the same sequence.
static bool fuzzy_match(const char *pattern, const char *str)
{
  auto lc = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + 32) : c; };
  const char *p = pattern;
  for (; *str && *p; ++str)
    if (lc(*str) == lc(*p))
      ++p;
  return !*p;
}

// Failed bakes first, stable inside each group, thus the list does not jump as bakes start and stop.
static void rebuild_display_order(dag::Vector<int> &order)
{
  order.clear();
  for (int i = 0; i < int(entries.size()); ++i)
    if (fuzzy_match(entryFilter, entries[i].label.c_str()))
      order.push_back(i);

  eastl::stable_sort(order.begin(), order.end(), [](int a, int b) { return is_failed(entries[a]) && !is_failed(entries[b]); });
}

static void imgui_window()
{
  if (entries.empty())
  {
    close_visualization_texture();
    ImGui::TextUnformatted("No active OMM bake results.");
    ImGui::TextUnformatted("BVH releases bake buffers by default. Enable graphics/bvhRetainOmmBakeResults to inspect them.");
    ImGui::TextUnformatted("Other OMM users may need their own retention flags.");
    return;
  }

  selectedEntry = eastl::min(selectedEntry < 0 ? 0 : selectedEntry, int(entries.size()) - 1);

  ImGui::SetNextItemWidth(320.f);
  ImGui::InputTextWithHint("##omm_filter", "fuzzy filter (name, texture, object=...)", entryFilter, sizeof(entryFilter));

  static dag::Vector<int> displayOrder;
  rebuild_display_order(displayOrder);

  // W and S move in screen order and not in memory order, thus they obey the failed-first sequence.
  const ImGuiIO &io = ImGui::GetIO();
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !io.WantTextInput && !displayOrder.empty())
  {
    int shownAt = -1;
    for (int i = 0; i < int(displayOrder.size()); ++i)
      if (displayOrder[i] == selectedEntry)
        shownAt = i;
    if (ImGui::IsKeyPressed(ImGuiKey_W) && shownAt > 0)
      select_entry(displayOrder[shownAt - 1]);
    if (ImGui::IsKeyPressed(ImGuiKey_S) && shownAt >= 0 && shownAt + 1 < int(displayOrder.size()))
      select_entry(displayOrder[shownAt + 1]);
  }

  const int failedCount = eastl::count_if(entries.begin(), entries.end(), [](const DebugEntry &e) { return is_failed(e); });
  if (failedCount > 0)
    ImGui::TextColored(FAILED_COLOR, "%d failed bake(s)", failedCount);

  ImGui::BeginChild("omm_bake_results", ImVec2(320.f, 0.f), true);
  for (int i : displayOrder)
  {
    const DebugEntry &entry = entries[i];
    const String name(0, "%s##%d", entry.label.c_str(), i);
    const bool selected = selectedEntry == i;
    const bool failed = is_failed(entry);
    if (failed)
      ImGui::PushStyleColor(ImGuiCol_Text, FAILED_COLOR);
    if (ImGui::Selectable(name.c_str(), selected))
      select_entry(i);
    if (failed)
      ImGui::PopStyleColor();
    if (selected && scrollSelectionOnNextDraw)
    {
      ImGui::SetScrollHereY(0.5f);
      scrollSelectionOnNextDraw = false;
    }
  }
  ImGui::EndChild();

  ImGui::SameLine();

  ImGui::BeginChild("omm_bake_details", ImVec2(0.f, 0.f), true);
  draw_entry(entries[selectedEntry]);
  ImGui::EndChild();
}

// erase_unsorted moves the last element into the free slot. For the last entry that is a move onto
// itself, which UniqueRes does not permit.
static void remove_entry(int index)
{
  DebugEntry &entry = entries[index];
  // UniqueRes does not release a buffer whose managed refcount is above one, thus unbind first.
  unbind_visualization();
  entry.triangleUvs.close();
  if (entry.heldAlphaTexture != BAD_TEXTUREID)
  {
    release_managed_tex(entry.heldAlphaTexture);
    entry.heldAlphaTexture = BAD_TEXTUREID;
  }
  clear_result(entry.adopted);

  const int movedFromIndex = int(entries.size()) - 1;
  if (index == movedFromIndex)
    entries.pop_back();
  else
    entries.erase_unsorted(entries.begin() + index);

  if (selectedEntry == index)
  {
    selectedEntry = eastl::min(selectedEntry, int(entries.size()) - 1);
    viewOrigin = ImVec2(0.f, 0.f);
    viewScale = 1.f;
    fitViewOnNextDraw = true;
    needsRefresh = true;
  }
  else if (selectedEntry == movedFromIndex)
    selectedEntry = index; // erase_unsorted moved the selected entry into the free slot: follow it

  if (entries.empty())
    close_visualization_texture();
}

// Which entry goes is arbitrary: erase_unsorted leaves no insertion order to find the oldest.
static void evict_adopted_entry_if_full()
{
  int adoptedCount = 0;
  for (const DebugEntry &e : entries)
    adoptedCount += e.isAdopted() ? 1 : 0;
  if (adoptedCount < MAX_RETAINED_FAILED_ENTRIES)
    return;

  for (int i = 0; i < int(entries.size()); ++i)
    if (entries[i].isAdopted())
    {
      logwarn("omm debug: already retaining %d failed bakes, dropping the first of them, '%s', to make room",
        MAX_RETAINED_FAILED_ENTRIES, entries[i].label.c_str());
      remove_entry(i);
      return;
    }
}

static DebugEntry &add_entry(const DebugBakeResultInfo &info)
{
  const bool wasEmpty = entries.empty();
  DebugEntry &entry = entries.emplace_back();
  entry.info = info;
  // Copy the caller's strings; the viewer must not keep its pointers.
  entry.label = info.label ? info.label : "OMM bake result";
  entry.info.label = nullptr;
  entry.failReason = info.failReason ? info.failReason : "";
  entry.info.failReason = nullptr;

  if (info.source.alphaTextureId != BAD_TEXTUREID && acquire_managed_tex(info.source.alphaTextureId))
    entry.heldAlphaTexture = info.source.alphaTextureId;

  if (wasEmpty || selectedEntry < 0)
    fitViewOnNextDraw = true;
  needsRefresh = true;
  return entry;
}

static void finish_entry(DebugEntry &entry)
{
  snapshot_triangle_uvs(entry);
  // The code needs this only for the copy, and the pointer is not valid after the producer stops.
  entry.info.source.texCoordBuffer = nullptr;
  entry.info.source.indexBuffer = nullptr;
}

} // namespace

DebugBakeSource make_debug_bake_source(const BakeInput &input, TEXTUREID alpha_texture_id)
{
  return {
    .alphaTextureId = alpha_texture_id,
    .alphaTextureChannel = input.alphaTextureChannel,
    .alphaCutoff = input.alphaCutoff,
    .runtimeSamplerDesc = input.runtimeSamplerDesc,
    .dynamicSubdivisionScale = input.dynamicSubdivisionScale,
    .maxSubdivisionLevel = input.maxSubdivisionLevel,
    .uvCutout = input.uvCutout,
    .texCoordBuffer = input.texCoordBuffer,
    .texCoordOffsetInBytes = input.texCoordOffsetInBytes,
    .texCoordStrideInBytes = input.texCoordStrideInBytes,
    .texCoordFormat = input.texCoordFormat,
    .indexBuffer = input.indexBuffer,
    .indexBufferOffsetInBytes = input.indexBufferOffsetInBytes,
    .indexStrideInBytes = input.indexStrideInBytes,
    .indexFormat = input.indexFormat,
  };
}

void debug_register_bake_result(const BakeResult &result, const DebugBakeResultInfo &info)
{
  debug_unregister_bake_result(result);

  DebugEntry &entry = add_entry(info);
  entry.borrowed = &result;
  finish_entry(entry);
}

void debug_adopt_bake_result(BakeResult &&result, const DebugBakeResultInfo &info)
{
  debug_unregister_bake_result(result);
  evict_adopted_entry_if_full();

  DebugEntry &entry = add_entry(info);
  entry.adopted = eastl::move(result);
  finish_entry(entry);
}

void debug_unregister_bake_result(const BakeResult &result)
{
  for (int i = 0; i < int(entries.size()); ++i)
    if (entries[i].borrowed == &result)
    {
      remove_entry(i);
      return;
    }
}

void debug_shutdown() { clear_debug_resources(); }

REGISTER_IMGUI_WINDOW("Render", "OMM bake debug", imgui_window);

} // namespace render::omm

#else

namespace render::omm
{

DebugBakeSource make_debug_bake_source(const BakeInput &, TEXTUREID) { return {}; }

void debug_register_bake_result(const BakeResult &, const DebugBakeResultInfo &) {}

void debug_adopt_bake_result(BakeResult &&, const DebugBakeResultInfo &) {}

void debug_unregister_bake_result(const BakeResult &) {}

void debug_shutdown() {}

} // namespace render::omm

#endif
