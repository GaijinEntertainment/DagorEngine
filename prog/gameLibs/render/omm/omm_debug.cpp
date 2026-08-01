// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omm.h>

#if DAGOR_DBGLEVEL > 0

#include <3d/dag_resPtr.h>
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
#include <math.h>

namespace render::omm
{
namespace
{

static constexpr ResourceTagType OMM_DEBUG_RESOURCE_TAG = "omm_debug";
static constexpr int TILE_SIDE = 96;
static constexpr float TILE_GAP_RATIO = 0.1f;
static constexpr float EQUILATERAL_TRIANGLE_HEIGHT = 0.8660254038f;

struct DebugEntry
{
  const BakeResult *owner = nullptr;
  DebugBakeResultInfo info;
  String label;

  IndexFormat indexFormat = IndexFormat::UINT32;
  uint32_t indexCount = 0;
  uint32_t arrayDataSizeInBytes = 0;
  uint32_t descArraySizeInBytes = 0;
  uint32_t indexBufferSizeInBytes = 0;
  dag::Vector<raytrace::OpacityMicroMapDescription> arrayBuildDescs;
  dag::Vector<raytrace::OpacityMicroMapDescription> blasLinkageDescs;
};

dag::Vector<DebugEntry> entries;
int selectedEntry = -1;
ImVec2 viewOrigin = ImVec2(0.f, 0.f);
float viewScale = 1.f;
bool autoRefresh = true;
bool needsRefresh = true;
bool fitViewOnNextDraw = true;
bool scrollSelectionOnNextDraw = false;

Ptr<ComputeShaderElement> visualizeShader;
UniqueTex visualizationTex;
int visualizationWidth = 0;
int visualizationHeight = 0;

int targetVarId = -1;
int arrayDataVarId = -1;
int descArrayVarId = -1;
int indexBufferVarId = -1;
int indexFormatVarId = -1;
int indexCountVarId = -1;
int descCountVarId = -1;
int arrayDataSizeVarId = -1;
int tileSizeVarId = -1;
int gridColumnsVarId = -1;
int viewParamsVarId = -1;

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

  if (targetVarId < 0)
  {
    targetVarId = get_shader_variable_id("omm_debug_target", true);
    arrayDataVarId = get_shader_variable_id("omm_debug_array_data", true);
    descArrayVarId = get_shader_variable_id("omm_debug_desc_array", true);
    indexBufferVarId = get_shader_variable_id("omm_debug_index_buffer", true);
    indexFormatVarId = get_shader_variable_id("omm_debug_index_format", true);
    indexCountVarId = get_shader_variable_id("omm_debug_index_count", true);
    descCountVarId = get_shader_variable_id("omm_debug_desc_count", true);
    arrayDataSizeVarId = get_shader_variable_id("omm_debug_array_data_size", true);
    tileSizeVarId = get_shader_variable_id("omm_debug_tile_size", true);
    gridColumnsVarId = get_shader_variable_id("omm_debug_grid_columns", true);
    viewParamsVarId = get_shader_variable_id("omm_debug_view_params", true);
  }

  return true;
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
  const BakeResult *result = entry.owner;
  return result && result->arrayData && result->descArray && result->indexBuffer && entry.indexCount > 0 &&
         entry.descArraySizeInBytes >= sizeof(raytrace::InBufferOpacityMicroMapDescription);
}

static int get_grid_columns(const DebugEntry &entry)
{
  return eastl::max(1, int(ceilf(sqrtf(float(eastl::max(entry.indexCount, 1u))))));
}

static ImVec2 get_atlas_size(const DebugEntry &entry)
{
  const int columns = get_grid_columns(entry);
  const int rows = eastl::max(1, (int(entry.indexCount) + columns - 1) / columns);
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
  const BakeResult &result = *entry.owner;
  ShaderGlobal::set_texture(targetVarId, visualizationTex.getTexId());
  ShaderGlobal::set_buffer(arrayDataVarId, result.arrayData.getBufId());
  ShaderGlobal::set_buffer(descArrayVarId, result.descArray.getBufId());
  ShaderGlobal::set_buffer(indexBufferVarId, result.indexBuffer.getBufId());
  ShaderGlobal::set_int(indexFormatVarId, static_cast<int>(entry.indexFormat));
  ShaderGlobal::set_int(indexCountVarId, entry.indexCount);
  ShaderGlobal::set_int(descCountVarId, entry.descArraySizeInBytes / sizeof(raytrace::InBufferOpacityMicroMapDescription));
  ShaderGlobal::set_int(arrayDataSizeVarId, entry.arrayDataSizeInBytes);
  ShaderGlobal::set_int(tileSizeVarId, TILE_SIDE);
  ShaderGlobal::set_int(gridColumnsVarId, get_grid_columns(entry));
  ShaderGlobal::set_float4(viewParamsVarId, Point4(viewOrigin.x, viewOrigin.y, viewScale, 0.f));
}

static void unbind_visualization()
{
  if (targetVarId >= 0)
    ShaderGlobal::set_texture(targetVarId, BAD_TEXTUREID);
  if (arrayDataVarId >= 0)
    ShaderGlobal::set_buffer(arrayDataVarId, BAD_D3DRESID);
  if (descArrayVarId >= 0)
    ShaderGlobal::set_buffer(descArrayVarId, BAD_D3DRESID);
  if (indexBufferVarId >= 0)
    ShaderGlobal::set_buffer(indexBufferVarId, BAD_D3DRESID);
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
  const uint32_t descCount = entry.descArraySizeInBytes / sizeof(raytrace::InBufferOpacityMicroMapDescription);
  const int columns = get_grid_columns(entry);
  const int rows = eastl::max(1, (int(entry.indexCount) + columns - 1) / columns);
  ImGui::Text("index format: %s", index_format_name(entry.indexFormat));
  ImGui::Text("triangles: %u, descs: %u, grid: %d x %d", entry.indexCount, descCount, columns, rows);
  ImGui::Text("bytes: array %u, desc %u, index %u", entry.arrayDataSizeInBytes, entry.descArraySizeInBytes,
    entry.indexBufferSizeInBytes);
  ImGui::Text("object: %llu, geometry: %u, slot: %u%s%s", static_cast<unsigned long long>(entry.info.objectId),
    entry.info.geometryIndex, entry.info.slotId, entry.info.impostor ? ", impostor" : "", entry.info.secondary ? ", secondary" : "");

  draw_histogram("array build histogram", entry.arrayBuildDescs);
  draw_histogram("BLAS linkage histogram", entry.blasLinkageDescs);

  ImGui::Separator();

  ImGui::Checkbox("Auto refresh", &autoRefresh);
  ImGui::SameLine();
  if (ImGui::Button("Refresh"))
    needsRefresh = true;
  ImGui::SameLine();
  const bool fitRequested = ImGui::Button("Fit all");

  if (!can_visualize(entry))
  {
    ImGui::TextUnformatted("Selected result has no live bake buffers.");
    ImGui::TextUnformatted("Enable graphics/bvhRetainOmmBakeResults or the producer's OMM retention flag to display it.");
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
  const ImGuiIO &io = ImGui::GetIO();
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !io.WantTextInput)
  {
    if (ImGui::IsKeyPressed(ImGuiKey_W) && selectedEntry > 0)
      select_entry(selectedEntry - 1);
    if (ImGui::IsKeyPressed(ImGuiKey_S) && selectedEntry + 1 < int(entries.size()))
      select_entry(selectedEntry + 1);
  }

  ImGui::BeginChild("omm_bake_results", ImVec2(320.f, 0.f), true);
  for (int i = 0; i < int(entries.size()); ++i)
  {
    const DebugEntry &entry = entries[i];
    const String name(0, "%s##%d", entry.label.c_str(), i);
    const bool selected = selectedEntry == i;
    if (ImGui::Selectable(name.c_str(), selected))
      select_entry(i);
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

} // namespace

void debug_register_bake_result(const BakeResult &result, const DebugBakeResultInfo &info)
{
  debug_unregister_bake_result(result);

  const bool wasEmpty = entries.empty();
  DebugEntry &entry = entries.emplace_back();
  entry.owner = &result;
  entry.info = info;
  entry.info.label = nullptr;
  entry.label = info.label ? info.label : "OMM bake result";
  entry.indexFormat = result.indexFormat;
  entry.indexCount = result.indexCount;
  entry.arrayDataSizeInBytes = result.arrayDataSizeInBytes;
  entry.descArraySizeInBytes = result.descArraySizeInBytes;
  entry.indexBufferSizeInBytes = result.indexBufferSizeInBytes;
  entry.arrayBuildDescs = result.arrayBuildDescs;
  entry.blasLinkageDescs = result.blasLinkageDescs;
  if (wasEmpty || selectedEntry < 0)
    fitViewOnNextDraw = true;
  needsRefresh = true;
}

void debug_unregister_bake_result(const BakeResult &result)
{
  for (auto it = entries.begin(); it != entries.end(); ++it)
  {
    if (it->owner != &result)
      continue;

    const int removedIndex = int(it - entries.begin());
    const int movedFromIndex = int(entries.size()) - 1;
    entries.erase_unsorted(it);

    if (selectedEntry == removedIndex)
    {
      // The watched entry was deleted; keep the selection in range and reset the view.
      selectedEntry = eastl::min(selectedEntry, int(entries.size()) - 1);
      viewOrigin = ImVec2(0.f, 0.f);
      viewScale = 1.f;
      fitViewOnNextDraw = true;
      needsRefresh = true;
    }
    else if (selectedEntry == movedFromIndex)
    {
      // erase_unsorted relocated the watched entry from the back into the freed slot; follow it.
      selectedEntry = removedIndex;
    }

    if (entries.empty())
      close_visualization_texture();
    return;
  }
}

void debug_shutdown() { clear_debug_resources(); }

REGISTER_IMGUI_WINDOW("Render", "OMM bake debug", imgui_window);

} // namespace render::omm

#else

namespace render::omm
{

void debug_register_bake_result(const BakeResult &, const DebugBakeResultInfo &) {}

void debug_unregister_bake_result(const BakeResult &) {}

void debug_shutdown() {}

} // namespace render::omm

#endif
