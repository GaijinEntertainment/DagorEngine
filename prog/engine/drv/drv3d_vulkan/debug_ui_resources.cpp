// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0
#include "debug_ui.h"
#include <gui/dag_imgui.h>
#include "imgui.h"
#include "globals.h"
#include "resource_manager.h"

using namespace drv3d_vulkan;

struct UiResListData
{
  const char *type;
  String name;
  String size;
  String overhead;
};

namespace
{
dag::Vector<UiResListData> uiResList;
struct SummaryInfo
{
  uint64_t size;
  uint64_t overhead;
  uint64_t count;
};
SummaryInfo summary[(int)ResourceType::COUNT];
} // namespace

void listUpdate()
{
  if (!ImGui::Button("Update") && uiResList.size() > 0)
    return;

  for (int i = 0; i < (int)ResourceType::COUNT; ++i)
    summary[i] = {};

  uiResList.clear();
  auto statPrintCb = [](Resource *i) {
    if (i->isManaged())
    {
      if (i->isResident())
      {
        const ResourceMemory &rm = i->getMemory();
        uiResList.push_back(
          {i->resTypeString(), String(i->getDebugName()), byte_size_unit(rm.originalSize), byte_size_unit(rm.size - rm.originalSize)});
        summary[(int)i->getResType()].size += rm.originalSize;
        summary[(int)i->getResType()].overhead += rm.size - rm.originalSize;
      }
      else
      {
        uiResList.push_back({i->resTypeString(), String(i->getDebugName()), String("Evicted"), String("Evicted")});
      }
    }
    else
    {
      uiResList.push_back({i->resTypeString(), String(i->getDebugName()), String("N/A"), String("N/A")});
    }
    ++summary[(int)i->getResType()].count;
  };

  WinAutoLock lk(Globals::Mem::mutex);
  Globals::Mem::res.iterateAllocated<Buffer>(statPrintCb);
  Globals::Mem::res.iterateAllocated<Image>(statPrintCb);
  Globals::Mem::res.iterateAllocated<RenderPassResource>(statPrintCb);
#if D3D_HAS_RAY_TRACING
  Globals::Mem::res.iterateAllocated<RaytraceAccelerationStructure>(statPrintCb);
#endif
}

void drawInfo()
{
  String szStr;
  String ovStr;
  for (int i = 0; i < (int)ResourceType::COUNT; ++i)
  {
    szStr = byte_size_unit(summary[i].size);
    ovStr = byte_size_unit(summary[i].overhead);
    ImGui::Text("%s: total %llu items | %s size | %s overhead", Resource::resTypeString((ResourceType)i), summary[i].count + 0ull,
      szStr.c_str(), ovStr.c_str());
  }
}

void drawList()
{
  if (!ImGui::BeginTable("VULK-res-list", 4))
    return;

  ImGui::TableSetupColumn("Type");
  ImGui::TableSetupColumn("Name");
  ImGui::TableSetupColumn("Size");
  ImGui::TableSetupColumn("Overhead");
  ImGui::TableHeadersRow();
  ImGui::TableNextRow();

  for (const UiResListData &i : uiResList)
  {
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(i.type);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(i.name.begin(), i.name.end());
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted(i.size.begin(), i.size.end());
    ImGui::TableSetColumnIndex(3);
    ImGui::TextUnformatted(i.overhead.begin(), i.overhead.end());
    ImGui::TableNextRow();
  }
  ImGui::EndTable();
}

void drv3d_vulkan::debug_ui_resources()
{
  listUpdate();
  drawInfo();

  if (ImGui::TreeNode("List##1"))
  {
    drawList();
    ImGui::TreePop();
  }
}

#endif