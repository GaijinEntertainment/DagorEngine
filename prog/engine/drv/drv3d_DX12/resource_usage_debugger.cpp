// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"

#include <gui/dag_imgui.h>
#include <imgui.h>
// #include <ioSys/dag_dataBlock.h>
// #include <implot.h>
// #include <psapi.h>

using namespace drv3d_dx12;

namespace
{
bool begin_sub_section(const char *id, const char *caption, int height)
{
  if (ImGui::BeginChild(id, ImVec2(0, height), ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar))
  {
    if (ImGui::BeginMenuBar())
    {
      ImGui::TextUnformatted(caption);
      ImGui::EndMenuBar();
    }
    return true;
  }
  return false;
}

void end_sub_section() { ImGui::EndChild(); }

void make_table_header(std::initializer_list<const char *> names)
{
  for (auto &&name : names)
  {
    ImGui::TableSetupColumn(name);
  }
  ImGui::TableHeadersRow();
}

void begin_selectable_row(const char *text)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::Selectable(text, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
}

// TODO needs refinement
bool needs_transition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
  if (from == to)
  {
    return false;
  }
  if (D3D12_RESOURCE_STATE_COMMON == to)
  {
    return true;
  }
  if (to == (from & to))
  {
    return false;
  }
  return true;
}

struct ScopedTooltip
{
  ScopedTooltip()
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  }
  ~ScopedTooltip()
  {
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
};

template <size_t N>
char *translate_to_string(char (&buf)[N], D3D12_RESOURCE_STATES state)
{
  if (D3D12_RESOURCE_STATE_COMMON == state)
  {
    strcpy_s(buf, "D3D12_RESOURCE_STATE_COMMON / D3D12_RESOURCE_STATE_PRESENT");
    return buf;
  }

  auto at = buf;
  auto start = buf;
  auto ed = buf + N - 1;
  auto concat = [&at, ed, start](const auto &s) {
    auto len = countof(s) - 1;
    auto left = ed - at;
    if (left >= 3 && start != at)
    {
      *at++ = ' ';
      *at++ = '|';
      *at++ = ' ';
      left -= 3;
    }
    auto count = min<size_t>(len, left);

    memcpy(at, s, count);
    at += count;
  };
#define CAT_STATE(name)       \
  if (name == (name & state)) \
  {                           \
    concat(#name);            \
  }
#define CAT_STATE_A2(name, name2) \
  if (name == (name & state))     \
  {                               \
    concat(#name " / " #name2);   \
  }
#define CAT_STATE_R(name)     \
  if (name == (name & state)) \
  {                           \
    state &= ~name;           \
    concat(#name);            \
  }
  CAT_STATE_R(D3D12_RESOURCE_STATE_GENERIC_READ);

  CAT_STATE_A2(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_PREDICATION);

  CAT_STATE(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  CAT_STATE(D3D12_RESOURCE_STATE_INDEX_BUFFER);
  CAT_STATE(D3D12_RESOURCE_STATE_RENDER_TARGET);
  CAT_STATE(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  CAT_STATE(D3D12_RESOURCE_STATE_DEPTH_WRITE);
  CAT_STATE(D3D12_RESOURCE_STATE_DEPTH_READ);
  CAT_STATE(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  CAT_STATE(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  CAT_STATE(D3D12_RESOURCE_STATE_STREAM_OUT);
  CAT_STATE(D3D12_RESOURCE_STATE_COPY_DEST);
  CAT_STATE(D3D12_RESOURCE_STATE_COPY_SOURCE);
  CAT_STATE(D3D12_RESOURCE_STATE_RESOLVE_DEST);
  CAT_STATE(D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
#if !_TARGET_XBOXONE
  CAT_STATE(D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
  CAT_STATE(D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
#endif
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_DECODE_READ);
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE);
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ);
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE);
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ);
  CAT_STATE(D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE);

#if _TARGET_XBOX
#include "resource_usage_debugger_xbox.inc.h"
#endif

#undef CAT_STATE_R
#undef CAT_STATE_A2
#undef CAT_STATE

  *at = '\0';
  return buf;
}

template <size_t N>
char *translate_to_string(char (&buf)[N], ResourceBarrier barrier)
{
  if (RB_NONE == barrier)
  {
    strcpy_s(buf, "RB_NONE");
    return buf;
  }

  auto at = buf;
  auto start = buf;
  auto ed = buf + N - 1;
  auto concat = [&at, ed, start](const auto &s) {
    auto len = countof(s) - 1;
    auto left = ed - at;
    if (left >= 3 && start != at)
    {
      *at++ = ' ';
      *at++ = '|';
      *at++ = ' ';
      left -= 3;
    }
    auto count = min<size_t>(len, left);

    memcpy(at, s, count);
    at += count;
  };
#define CAT_STATE_R(name)                                           \
  if (uint32_t(name) == (uint32_t(name) & uint32_t(barrier)))       \
  {                                                                 \
    barrier = ResourceBarrier(uint32_t(barrier) & ~uint32_t(name)); \
    concat(#name);                                                  \
  }
#define CAT_STATE_A2(name, name2)                                   \
  if (uint32_t(name) == (uint32_t(name) & uint32_t(barrier)))       \
  {                                                                 \
    barrier = ResourceBarrier(uint32_t(barrier) & ~uint32_t(name)); \
    concat(#name " / " #name);                                      \
  }

  CAT_STATE_R(RB_STAGE_ALL_SHADERS);
  CAT_STATE_R(RB_STAGE_ALL_GRAPHICS);

  CAT_STATE_R(RB_RO_GENERIC_READ_TEXTURE);
  CAT_STATE_R(RB_RO_GENERIC_READ_BUFFER);

  CAT_STATE_R(RB_RO_CONSTANT_DEPTH_STENCIL_TARGET);

  CAT_STATE_R(RB_SOURCE_STAGE_ALL_SHADERS);
  CAT_STATE_R(RB_SOURCE_STAGE_ALL_GRAPHICS);
  CAT_STATE_R(RB_ALIAS_TO_AND_DISCARD);
  CAT_STATE_R(RB_ALIAS_ALL);
  CAT_STATE_R(RB_ALIAS_TO);
  CAT_STATE_R(RB_ALIAS_FROM);
  CAT_STATE_R(RB_SOURCE_STAGE_RAYTRACE);
  CAT_STATE_R(RB_SOURCE_STAGE_COMPUTE);
  CAT_STATE_R(RB_SOURCE_STAGE_PIXEL);
  CAT_STATE_R(RB_SOURCE_STAGE_VERTEX);
  CAT_STATE_R(RB_FLAG_DONT_PRESERVE_CONTENT);
  CAT_STATE_R(RB_FLUSH_UAV);
  CAT_STATE_R(RB_STAGE_RAYTRACE);
  CAT_STATE_R(RB_STAGE_COMPUTE);
  CAT_STATE_R(RB_STAGE_PIXEL);
  CAT_STATE_R(RB_STAGE_VERTEX);
  CAT_STATE_R(RB_FLAG_SPLIT_BARRIER_END);
  CAT_STATE_R(RB_FLAG_SPLIT_BARRIER_BEGIN);
  CAT_STATE_R(RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP);
  CAT_STATE_R(RB_FLAG_RELEASE_PIPELINE_OWNERSHIP);
  CAT_STATE_R(RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE);
  CAT_STATE_R(RB_RO_BLIT_SOURCE);
  CAT_STATE_R(RB_RO_COPY_SOURCE);
  CAT_STATE_R(RB_RO_VARIABLE_RATE_SHADING_TEXTURE);
  CAT_STATE_R(RB_RO_INDIRECT_BUFFER);
  CAT_STATE_R(RB_RO_INDEX_BUFFER);
  CAT_STATE_R(RB_RO_VERTEX_BUFFER);
  CAT_STATE_R(RB_RO_CONSTANT_BUFFER);
  CAT_STATE_R(RB_RO_SRV);
  CAT_STATE_R(RB_RW_BLIT_DEST);
  CAT_STATE_R(RB_RW_COPY_DEST);
  CAT_STATE_R(RB_RW_UAV);
  CAT_STATE_A2(RB_RW_RENDER_TARGET, RB_RW_DEPTH_STENCIL_TARGET);
#undef CAT_STATE_A2
#undef CAT_STATE_R

  *at = '\0';
  return buf;
}
} // namespace

#if DX12_RESOURCE_USAGE_TRACKER
ResourceUsageHistoryDataSetDebugger::AnalyzerMode ResourceUsageHistoryDataSetDebugger::drawAnalyzerModeSelector(AnalyzerMode mode)
{
  auto nextMode = mode;
  if (ImGui::BeginCombo("Analyzer Mode", to_string(mode)))
  {
    for (auto m : //
      {AnalyzerMode::UNIQUE_MISSING_BARRIERS, AnalyzerMode::UNIQUE_USES, AnalyzerMode::ALL_MISSING_BARRIERS, AnalyzerMode::ALL_USES})
    {
      if (ImGui::Selectable(to_string(m), m == mode))
      {
        if (mode != m)
        {
          nextMode = m;
        }
      }
      if (m == mode)
      {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  return nextMode;
}

void ResourceUsageHistoryDataSetDebugger::addAnalyzedTextureInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state,
  eastl::string_view segment, ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition,
  eastl::string_view what_string, AnalyzedIssueType issue_type)
{
  AnalyzedEntry e;
  e.currentState = current_state;
  e.expectedState = expected_state;
  e.segment = segment;
  e.what = what;
  e.isAutomaticTransition = is_automatic_transition;
  e.whatString = what_string;
  e.count = 1;
  e.issueType = issue_type;
  analyzedTextureEntries.push_back(e);
}

void ResourceUsageHistoryDataSetDebugger::addUniqueAnalyzedTextureInfo(D3D12_RESOURCE_STATES current_state,
  ResourceBarrier expected_state, eastl::string_view segment, ResourceUsageHistoryDataSet::UsageEntryType what,
  bool is_automatic_transition, eastl::string_view what_string, AnalyzedIssueType issue_type)
{
  auto ref = eastl::find_if(begin(analyzedTextureEntries), end(analyzedTextureEntries),
    [=](auto &e) //
    {
      return e.currentState == current_state && e.expectedState == expected_state && e.segment == segment && e.what == what &&
             e.isAutomaticTransition == is_automatic_transition && e.whatString == what_string && e.issueType == issue_type;
    });
  if (ref != end(analyzedTextureEntries))
  {
    ++ref->count;
    return;
  }
  addAnalyzedTextureInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string, issue_type);
}

void ResourceUsageHistoryDataSetDebugger::addAnalyzedBufferInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state,
  eastl::string_view segment, ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition,
  eastl::string_view what_string, AnalyzedIssueType issue_type)
{
  AnalyzedEntry e;
  e.currentState = current_state;
  e.expectedState = expected_state;
  e.segment = segment;
  e.what = what;
  e.isAutomaticTransition = is_automatic_transition;
  e.whatString = what_string;
  e.count = 1;
  e.issueType = issue_type;
  analyzedBufferEntries.push_back(e);
}

void ResourceUsageHistoryDataSetDebugger::addUniqueAnalyzedBufferInfo(D3D12_RESOURCE_STATES current_state,
  ResourceBarrier expected_state, eastl::string_view segment, ResourceUsageHistoryDataSet::UsageEntryType what,
  bool is_automatic_transition, eastl::string_view what_string, AnalyzedIssueType issue_type)
{
  auto ref = eastl::find_if(begin(analyzedBufferEntries), end(analyzedBufferEntries),
    [=](auto &e) //
    {
      return e.currentState == current_state && e.expectedState == expected_state && e.segment == segment && e.what == what &&
             e.isAutomaticTransition == is_automatic_transition && e.whatString == what_string && e.issueType == issue_type;
    });
  if (ref != end(analyzedBufferEntries))
  {
    ++ref->count;
    return;
  }
  addAnalyzedBufferInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string, issue_type);
}

void ResourceUsageHistoryDataSetDebugger::addTextureInfo(eastl::string_view name, Image *texture, SubresourceIndex sub_res,
  FormatStore fmt, bool needs_transition)
{
  auto ref = eastl::lower_bound(begin(textures), end(textures), 0,
    [=](auto &v, auto) //
    {
      if (eastl::string_view(v.name) < name)
      {
        return true;
      }
      if (eastl::string_view(v.name) > name)
      {
        return false;
      }
      if (v.subRes < sub_res)
      {
        return true;
      }
      if (v.subRes > sub_res)
      {
        return false;
      }
      return v.texture < texture;
    });
  if (ref != end(textures) && ref->texture == texture && eastl::string_view(ref->name) == name && ref->subRes == sub_res)
  {
    ++ref->entries;
    ref->neededTransitionCount += needs_transition ? 1 : 0;
  }
  else
  {
    if (inspectingTexture >= eastl::distance(begin(textures), ref) && ~size_t(0) != inspectingTexture)
    {
      // have to adjust index for the new entry
      ++inspectingTexture;
    }
    TextureInfo info;
    info.name = name;
    info.texture = texture;
    info.subRes = sub_res;
    info.fmt = fmt;
    info.entries = 1;
    info.neededTransitionCount = needs_transition ? 1 : 0;
    textures.insert(ref, eastl::move(info));
  }
}

void ResourceUsageHistoryDataSetDebugger::addBufferInfo(eastl::string_view name, BufferResourceReference buffer, bool needs_transition)
{
  auto ref = eastl::lower_bound(begin(buffers), end(buffers), 0,
    [=](auto &v, auto) //
    {
      if (eastl::string_view(v.name) < name)
      {
        return true;
      }
      if (eastl::string_view(v.name) > name)
      {
        return false;
      }
      if (v.buffer.resourceId.index() < buffer.resourceId.index())
      {
        return true;
      }
      if (v.buffer.resourceId.index() > buffer.resourceId.index())
      {
        return false;
      }
      return v.buffer.buffer < buffer.buffer;
    });
  if (ref != end(buffers) && ref->buffer.buffer == buffer.buffer && ref->buffer.resourceId.index() &&
      eastl::string_view(ref->name) == name)
  {
    ++ref->entries;
    ref->neededTransitionCount += needs_transition ? 1 : 0;
  }
  else
  {
    if (inspectingBuffer >= eastl::distance(begin(buffers), ref) && ~size_t(0) != inspectingBuffer)
    {
      // have to adjust index for the new entry
      ++inspectingBuffer;
    }
    BufferInfo info;
    info.name = name;
    info.buffer = buffer;
    info.entries = 1;
    info.neededTransitionCount = needs_transition ? 1 : 0;
    buffers.insert(ref, eastl::move(info));
  }
}

void ResourceUsageHistoryDataSetDebugger::processDataSet(FrameDataSet &data_set)
{
  for (auto &set : data_set.dataSet)
  {
    set.iterateTextures([this](auto texture, auto name, auto fmt, auto sub_res, auto state, auto expected_state, auto segment,
                          auto what, auto is_automatic, auto what_name) //
      {
        G_UNUSED(segment);
        G_UNUSED(what);
        G_UNUSED(what_name);
        bool needsTransition = false;
        if (!is_automatic)
        {
          if (D3D12_RESOURCE_STATE_COMMON != state)
          {
            needsTransition = needs_transition(state, translate_texture_barrier_to_state(expected_state, !fmt.isColor()));
          }
        }
        addTextureInfo(name, texture, sub_res, fmt, needsTransition);
        return true;
      });
    set.iterateBuffers(
      [this](auto buffer, auto name, auto state, auto expected_state, auto segment, auto what, auto is_automatic, auto what_name) //
      {
        G_UNUSED(segment);
        G_UNUSED(what);
        G_UNUSED(what_name);
        bool needsTransition = false;
        if (!is_automatic)
        {
          if (D3D12_RESOURCE_STATE_COMMON != state)
          {
            needsTransition = needs_transition(state, translate_buffer_barrier_to_state(expected_state));
          }
        }
        addBufferInfo(name, buffer, needsTransition);
        return true;
      });
  }
}

void ResourceUsageHistoryDataSetDebugger::processDataSets()
{
  if (processedHistoryCount >= dataSets.size())
  {
    return;
  }
  processDataSet(dataSets[processedHistoryCount++]);
}

void ResourceUsageHistoryDataSetDebugger::debugOverlay()
{
  WinAutoLock lock{mutex};
  if (ImGui::TreeNodeEx("Settings##Resource-Usage-History-Settings", ImGuiTreeNodeFlags_Framed))
  {
    if (begin_sub_section("Resource-Usage-History-Settings-Segment", "Settings", ImGui::GetFrameHeightWithSpacing() * 10))
    {
      uint32_t nextValue = max<uint32_t>(1, framesToRecord);
      uint32_t minValue = 1;
      if (ImGui::DragScalar("Number of frames to capture", ImGuiDataType_U32, &nextValue, 1.f, &minValue, nullptr, nullptr,
            ImGuiSliderFlags_AlwaysClamp))
      {
        framesToRecord = nextValue;
      }
      ImGui::Checkbox("Capture frames", &recordHistory);
      ImGui::Checkbox("Process captured frames", &processHistory);

      ByteUnits s;
      for (auto &ds : dataSets)
      {
        for (auto &d : ds.dataSet)
        {
          s += d.memoryUse();
        }
      }
      ImGui::Text("%u history entries using %.2f %s", uint32_t(dataSets.size()), s.units(), s.name());
      ImGui::Text("%u processed entries, found %u unique textures and %u unique buffers", uint32_t(processedHistoryCount),
        uint32_t(textures.size()), uint32_t(buffers.size()));

      float progressFraction = 1.f;
      if (!dataSets.empty())
      {
        progressFraction = static_cast<float>(processedHistoryCount) / dataSets.size();
      }
      char overlay[64];
      sprintf_s(overlay, "%3.2f%% Processed", progressFraction * 100);
      ImGui::ProgressBar(progressFraction, ImVec2(-FLT_MIN, 0), overlay);

      bool clearHistory = ImGui::Button("Clear history");
      bool clearInfo = ImGui::Button("Clear resource info");
      if (ImGui::Button("Clear all"))
      {
        clearHistory = true;
        clearInfo = true;
      }
      if (clearHistory)
      {
        dataSets.clear();
        processedHistoryCount = 0;
      }
      if (clearInfo)
      {
        textures.clear();
        buffers.clear();
        processedHistoryCount = 0;
      }
    }
    end_sub_section();
    ImGui::TreePop();
  }

  if (processHistory)
  {
    processDataSets();
  }

  if (ImGui::TreeNodeEx("Textures##Resource-Usage-History-Textures", ImGuiTreeNodeFlags_Framed))
  {
    if (begin_sub_section("Texture-Usage-Segment", "Textures", ImGui::GetFrameHeightWithSpacing() * 10))
    {
      if (ImGui::BeginTable("Texture-Usage-Segment-Table", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
      {
        make_table_header({"Name", "Subresource", "Object", "Entries"});
        for (auto &e : textures)
        {
          begin_selectable_row(e.name.c_str());
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%u", e.subRes.index());
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%p", e.texture);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%u", e.entries);
        }
        ImGui::EndTable();
      }
    }
    end_sub_section();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Buffers##Resource-Usage-History-Buffers", ImGuiTreeNodeFlags_Framed))
  {
    if (begin_sub_section("Buffer-Usage-Segment", "Buffers", ImGui::GetFrameHeightWithSpacing() * 10))
    {
      if (ImGui::BeginTable("Buffer-Usage-Segment-Table", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
      {
        make_table_header({"Name", "Object", "ID", "Entries"});
        for (auto &e : buffers)
        {
          begin_selectable_row(e.name.c_str());
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%p", e.buffer.buffer);
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%u", e.buffer.resourceId.index());
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%u", e.entries);
        }
        ImGui::EndTable();
      }
    }
    end_sub_section();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Texture Barrier Analyzer##Usage-Analyzer-Texture", ImGuiTreeNodeFlags_Framed))
  {
    auto nextMode = drawAnalyzerModeSelector(inspectinTextureMode);
    if (nextMode != inspectinTextureMode)
    {
      inspectinTextureMode = nextMode;
      analyzedTextureEntries.clear();
      inspectingTextureProgress = 0;
    }
    const bool isUniqueMode =
      AnalyzerMode::UNIQUE_MISSING_BARRIERS == inspectinTextureMode || AnalyzerMode::UNIQUE_USES == inspectinTextureMode;
    const bool isBarrierMode =
      AnalyzerMode::UNIQUE_MISSING_BARRIERS == inspectinTextureMode || AnalyzerMode::ALL_MISSING_BARRIERS == inspectinTextureMode;
    char strBuf[4096];
    auto labelOf = [&](auto &e) //
    {
      sprintf_s(strBuf, "%s[%u] %p", e.name.c_str(), e.subRes, e.texture);
      return strBuf;
    };
    const char *label = "<nothing>";
    if (inspectingTexture < textures.size())
    {
      label = labelOf(textures[inspectingTexture]);
    }
    if (ImGui::BeginCombo("Texture To Analyze", label, ImGuiComboFlags_HeightRegular))
    {
      auto oldIndex = inspectingTexture;
      if (ImGui::Selectable("<nothing>", inspectingTexture >= textures.size()))
      {
        inspectingTexture = ~size_t(0);
      }
      if (oldIndex >= textures.size())
      {
        ImGui::SetItemDefaultFocus();
      }
      for (size_t i = 0; i < textures.size(); ++i)
      {
        if (i != inspectingTexture && isBarrierMode && 0 == textures[i].neededTransitionCount)
        {
          continue;
        }
        if (ImGui::Selectable(labelOf(textures[i]), i == inspectingTexture))
        {
          if (inspectingTexture != i)
          {
            analyzedTextureEntries.clear();
            inspectingTextureProgress = 0;
            inspectingTexture = i;
          }
        }
        if (oldIndex == i)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    if (inspectingTexture < textures.size())
    {
      auto &toInspect = textures[inspectingTexture];
      constexpr size_t maxSteps = 10;
      for (size_t i = 0; i < maxSteps && inspectingTextureProgress < dataSets.size(); ++i, ++inspectingTextureProgress)
      {
        auto &dataSet = dataSets[inspectingTextureProgress];
        // TODO:
        // - detect unused user barriers
        for (auto &ds : dataSet.dataSet)
        {
          ds.iterateTextures([this, isUniqueMode, isBarrierMode, &toInspect](auto texture, auto name, auto fmt, auto sub_res,
                               auto current_state, auto expected_state, auto segment, auto what, auto is_automatic_transition,
                               auto what_string) //
            {
              G_UNUSED(fmt);
              if (texture != toInspect.texture)
              {
                return true;
              }
              if (sub_res != toInspect.subRes)
              {
                return true;
              }
              if (name != eastl::string_view(toInspect.name))
              {
                return true;
              }

              if (is_automatic_transition && isBarrierMode)
              {
                return true;
              }
              if (D3D12_RESOURCE_STATE_COMMON == current_state && isBarrierMode)
              {
                return true;
              }
              bool needsTransition =
                needs_transition(current_state, translate_texture_barrier_to_state(expected_state, !toInspect.fmt.isColor()));
              if (!needsTransition && isBarrierMode)
              {
                return true;
              }
              AnalyzedIssueType issueType = AnalyzedIssueType::OKAY;
              if (!is_automatic_transition && (D3D12_RESOURCE_STATE_COMMON != current_state) && needsTransition)
              {
                issueType = AnalyzedIssueType::MISSING_BARRIER;
              }
              if (isUniqueMode)
              {
                addUniqueAnalyzedTextureInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string,
                  issueType);
              }
              else
              {
                addAnalyzedTextureInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string, issueType);
              }
              return true;
            });
        }
      }
      if (begin_sub_section("Texture-Analyzer-Result-Segment", "Results", ImGui::GetFrameHeightWithSpacing() * 20))
      {
        bool isColorFormat = toInspect.fmt.isColor();
        if (ImGui::BeginTable("Texture-Analyzer-Result-Table", 5,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
          make_table_header({"Use", "Current State", "Used State", "Expected Barrier", "Segment"});
          for (auto &e : analyzedTextureEntries)
          {
            // TODO:
            // - make wrong / unused user barrier a unique color (red?)
            bool doPopStyleColor = false;
            if (AnalyzedIssueType::OKAY != e.issueType)
            {
              doPopStyleColor = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.75f, .75f, 0.f, 1.f));
            }
            else if (ResourceUsageHistoryDataSet::UsageEntryType::USER_BARRIER == e.what)
            {
              doPopStyleColor = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.0f, .75f, 0.f, 1.f));
            }
            begin_selectable_row(e.whatString.data());
            bool rowIsHovered = ImGui::IsItemHovered();
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%08X", uint32_t(e.currentState));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%08X", uint32_t(translate_texture_barrier_to_state(e.expectedState, !isColorFormat)));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%08X", uint32_t(e.expectedState));
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(e.segment.data());
            if (doPopStyleColor)
            {
              ImGui::PopStyleColor();
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(1) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, e.currentState));
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(2) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, translate_texture_barrier_to_state(e.expectedState, !isColorFormat)));
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(3) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, e.expectedState));
            }
          }
          ImGui::EndTable();
        }
        if (ImGui::Button("Report to debug log"))
        {
          logdbg("Texture usage analysis %s of <%s>", to_string(inspectinTextureMode), toInspect.name);
          logdbg("Use | Current State | Used State | Expected Barrier | Segment");
          for (auto &e : analyzedTextureEntries)
          {
            logdbg("%s | %08X | %08X | | %08X | %s", e.whatString, uint32_t(e.currentState),
              uint32_t(translate_texture_barrier_to_state(e.expectedState, !isColorFormat)), uint32_t(e.expectedState), e.segment);
          }
          logdbg("Listed %u entries", analyzedTextureEntries.size());
        }
      }
      end_sub_section();
      if (inspectingTextureProgress < dataSets.size())
      {
        sprintf_s(strBuf, "%u / %u - %.2f%%", uint32_t(inspectingTextureProgress), uint32_t(dataSets.size()),
          100.0 * inspectingTextureProgress / dataSets.size());
        ImGui::ProgressBar(float(inspectingTextureProgress) / dataSets.size(), ImVec2(-FLT_MIN, 0), strBuf);
      }
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Buffer Barrier Analyzer##Usage-Analyzer-Buffer", ImGuiTreeNodeFlags_Framed))
  {
    auto nextMode = drawAnalyzerModeSelector(inspectinBufferMode);
    if (nextMode != inspectinBufferMode)
    {
      inspectinBufferMode = nextMode;
      analyzedBufferEntries.clear();
      inspectingBufferProgress = 0;
    }
    const bool isUniqueMode =
      AnalyzerMode::UNIQUE_MISSING_BARRIERS == inspectinBufferMode || AnalyzerMode::UNIQUE_USES == inspectinBufferMode;
    const bool isBarrierMode =
      AnalyzerMode::UNIQUE_MISSING_BARRIERS == inspectinBufferMode || AnalyzerMode::ALL_MISSING_BARRIERS == inspectinBufferMode;
    char strBuf[4096];
    auto labelOf = [&](auto &e) //
    {
      sprintf_s(strBuf, "%s %p", e.name.c_str(), e.buffer.buffer);
      return strBuf;
    };
    const char *label = "<nothing>";
    if (inspectingBuffer < buffers.size())
    {
      label = labelOf(buffers[inspectingBuffer]);
    }
    if (ImGui::BeginCombo("Buffer To Analyze", label, ImGuiComboFlags_HeightRegular))
    {
      auto oldIndex = inspectingBuffer;
      if (ImGui::Selectable("<nothing>", inspectingBuffer >= buffers.size()))
      {
        inspectingBuffer = ~size_t(0);
      }
      if (oldIndex >= buffers.size())
      {
        ImGui::SetItemDefaultFocus();
      }
      for (size_t i = 0; i < buffers.size(); ++i)
      {
        if (i != inspectingBuffer && isBarrierMode && 0 == buffers[i].neededTransitionCount)
        {
          continue;
        }
        if (ImGui::Selectable(labelOf(buffers[i]), i == inspectingBuffer))
        {
          if (inspectingBuffer != i)
          {
            analyzedBufferEntries.clear();
            inspectingBufferProgress = 0;
            inspectingBuffer = i;
          }
        }
        if (oldIndex == i)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    if (inspectingBuffer < buffers.size())
    {
      auto &toInspect = buffers[inspectingBuffer];
      constexpr size_t maxSteps = 10;
      for (size_t i = 0; i < maxSteps && inspectingBufferProgress < dataSets.size(); ++i, ++inspectingBufferProgress)
      {
        auto &dataSet = dataSets[inspectingBufferProgress];
        for (auto &ds : dataSet.dataSet)
        {
          ds.iterateBuffers([this, isUniqueMode, isBarrierMode, &toInspect](auto buffer, auto name, auto current_state, // -V657
                              auto expected_state, auto segment, auto what, bool is_automatic_transition, auto what_string) {
            if (buffer != toInspect.buffer)
            {
              return true;
            }
            if (name != eastl::string_view(toInspect.name))
            {
              return true;
            }

            if (is_automatic_transition && isBarrierMode)
            {
              return true;
            }
            if (D3D12_RESOURCE_STATE_COMMON == current_state && isBarrierMode)
            {
              return true;
            }
            bool needsTransition = needs_transition(current_state, translate_buffer_barrier_to_state(expected_state));
            if (!needsTransition && isBarrierMode)
            {
              return true;
            }
            AnalyzedIssueType issueType = AnalyzedIssueType::OKAY;
            if (!is_automatic_transition && (D3D12_RESOURCE_STATE_COMMON != current_state) && needsTransition)
            {
              issueType = AnalyzedIssueType::MISSING_BARRIER;
            }
            if (isUniqueMode)
            {
              addUniqueAnalyzedBufferInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string,
                issueType);
            }
            else
            {
              addAnalyzedBufferInfo(current_state, expected_state, segment, what, is_automatic_transition, what_string, issueType);
            }
            return true;
          }); // -V657
        }
      }
      if (begin_sub_section("Buffer-Analyzer-Result-Segment", "Results", ImGui::GetFrameHeightWithSpacing() * 20))
      {
        if (ImGui::BeginTable("Texture-Analyzer-Result-Table", 5,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
          make_table_header({"Use", "Current State", "Used State", "Expected Barrier", "Segment"});
          for (auto &e : analyzedBufferEntries)
          {
            // TODO:
            // - make wrong / unused user barrier a unique color (red?)
            bool doPopStyleColor = false;
            if (AnalyzedIssueType::OKAY != e.issueType)
            {
              doPopStyleColor = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.75f, .75f, 0.f, 1.f));
            }
            else if (ResourceUsageHistoryDataSet::UsageEntryType::USER_BARRIER == e.what)
            {
              doPopStyleColor = true;
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.0f, .75f, 0.f, 1.f));
            }
            begin_selectable_row(e.whatString.data());
            bool rowIsHovered = ImGui::IsItemHovered();
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%08X", uint32_t(e.currentState));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%08X", uint32_t(translate_buffer_barrier_to_state(e.expectedState)));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%08X", uint32_t(e.expectedState));
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(e.segment.data());
            if (doPopStyleColor)
            {
              ImGui::PopStyleColor();
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(1) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, e.currentState));
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(2) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, translate_buffer_barrier_to_state(e.expectedState)));
            }
            if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(3) && rowIsHovered)
            {
              ScopedTooltip tooltip;
              ImGui::TextUnformatted(translate_to_string(strBuf, e.expectedState));
            }
          }
          ImGui::EndTable();
        }
        if (ImGui::Button("Report to debug log"))
        {
          logdbg("Buffer usage analysis %s of <%s>", to_string(inspectinTextureMode), toInspect.name);
          logdbg("Use | Current State | Used State | Expected Barrier | Segment");
          for (auto &e : analyzedBufferEntries)
          {
            logdbg("%s | %08X | %08X | | %08X | %s", e.whatString, uint32_t(e.currentState),
              uint32_t(translate_buffer_barrier_to_state(e.expectedState)), uint32_t(e.expectedState), e.segment);
          }
          logdbg("Listed %u entries", analyzedBufferEntries.size());
        }
      }
      end_sub_section();
      if (inspectingBufferProgress < dataSets.size())
      {
        sprintf_s(strBuf, "%u / %u - %.2f%%", uint32_t(inspectingBufferProgress), uint32_t(dataSets.size()),
          100.0 * inspectingBufferProgress / dataSets.size());
        ImGui::ProgressBar(float(inspectingBufferProgress) / dataSets.size(), ImVec2(-FLT_MIN, 0), strBuf);
      }
    }
    ImGui::TreePop();
  }
}
#endif
