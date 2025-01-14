// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0

#include <gui/dag_imgui.h>
#include <imgui.h>
#include "debug_ui.h"

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
#include <imgui_node_editor.h>
#include "globals.h"
#include "backend.h"
#include "frontend.h"
#include "backend_interop.h"
#include "execution_sync_capture.h"
#include "device_resource.h"
#include "vk_to_string.h"
#include <ska_hash_map/flat_hash_map2.hpp>

using namespace drv3d_vulkan;
using namespace ax;

namespace
{

NodeEditor::EditorContext *ectx = nullptr;

enum class ResSortModes
{
  ORIGINAL,
  UNIQ_RES,
  SORTED_RES
};

ResSortModes resSortMode = ResSortModes::UNIQ_RES;

enum class StepLinksMode
{
  NO,
  ALL,
  CHAIN
};

StepLinksMode showStepLinks = StepLinksMode::NO;
bool swapCoord = false;
bool requestCapture = false;
bool settleNodes = false;
bool requestedCapture = false;
bool resetView = false;
bool moveInputsToOutputs = true;

struct GraphLayout
{
  uint32_t opPosStep;
  uint32_t opSyncStepSpacing;
  uint32_t opSyncStepLeft;
  uint32_t opSyncStepYOfs;
  uint32_t opPosStepSameRes;
  uint32_t opNormalYBase;
  uint32_t opNormalXBase;
  uint32_t indepSeqSpacing;
  uint32_t sequencesYSpacing;
  uint32_t opSyncStepSpacingX;
  uint32_t opOrderSpacingX;
};

const GraphLayout yTimeLayout = {
  700, // opPosStep
  120, // opSyncStepSpacing
  600, // opSyncStepLeft
  60,  // opSyncStepYOfs
  10,  // opPosStepSameRes
  300, // opNormalYBase
  600, // opNormalXBase
  300, // indepSeqSpacing
  300, // sequencesYSpacing
  0,   // opSyncStepSpacingX
  310  // opOrderSpacingX
};

const GraphLayout xTimeLayout = {
  100, // opPosStep
  430, // opSyncStepSpacing
  600, // opSyncStepLeft
  60,  // opSyncStepYOfs
  150, // opPosStepSameRes
  300, // opNormalYBase
  600, // opNormalXBase
  70,  // indepSeqSpacing
  300, // sequencesYSpacing
  0,   // opSyncStepSpacingX
  150  // opOrderSpacingX
};

GraphLayout activeLayout = yTimeLayout;

void wnd_header()
{
  resetView = ImGui::Button("Reset view");
  ImGui::SameLine();
  requestCapture |= ImGui::Button("Capture");

  if (ImGui::Button("ResSort: original order"))
  {
    resSortMode = ResSortModes::ORIGINAL;
    settleNodes |= true;
  }
  ImGui::SameLine();
  if (ImGui::Button("ResSort: uniq res order"))
  {
    resSortMode = ResSortModes::UNIQ_RES;
    settleNodes |= true;
  }
  ImGui::SameLine();
  if (ImGui::Button("ResSort: sorted order"))
  {
    resSortMode = ResSortModes::SORTED_RES;
    settleNodes |= true;
  }

  if (ImGui::Button("Links: NO"))
    showStepLinks = StepLinksMode::NO;
  ImGui::SameLine();
  if (ImGui::Button("Links: ALL"))
    showStepLinks = StepLinksMode::ALL;
  ImGui::SameLine();
  if (ImGui::Button("Links: chain"))
    showStepLinks = StepLinksMode::CHAIN;

  if (ImGui::Button("Swap coord"))
  {
    swapCoord = swapCoord ? 0 : 1;
    activeLayout = swapCoord ? xTimeLayout : yTimeLayout;
    settleNodes = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Toggle input-to-output move"))
  {
    moveInputsToOutputs = !moveInputsToOutputs;
    settleNodes = true;
  }
}

bool request_capture_data_and_wait()
{
  if (Backend::interop.syncCaptureRequest.load(std::memory_order_acquire))
    return false;

  // request before settling if not
  if (!requestCapture && settleNodes)
    requestCapture |= true;

  if (requestCapture)
  {
    if (!requestedCapture)
    {
      requestedCapture = true;
      ++Backend::interop.syncCaptureRequest;
      return false;
    }
    else
    {
      requestedCapture = false;
      requestCapture = false;
      settleNodes = true;
    }
  }
  return true;
}

void push_indep_node(ExecutionSyncCapture::SyncStep &indep_step)
{
  ExecutionSyncCapture &capt = Frontend::syncCapture;
  capt.indepSeqs.push_back({capt.currentVisNode++, capt.currentVisPin++, 0});
  indep_step.visIndepSeq = capt.indepSeqs.size();
  indep_step.orderPos = indep_step.visIndepSeq - 1;
}

void settle_nodes()
{
  ExecutionSyncCapture &capt = Frontend::syncCapture;

  // prepare storage for node positions
  capt.reformedPositions.clear();
  capt.reformedPositions.resize(capt.currentVisNode + 1);

  uint32_t uniqueResId = 0;
  struct UniqResStat
  {
    uint32_t id;
    uint32_t count;
    uint32_t links;
    uint32_t localCounter;
    Tab<uint32_t> stepPos;
  };
  ska::flat_hash_map<Resource *, UniqResStat> uniqueResources;

  // sort on unique resources, count conflcting links for resources
  {
    for (ExecutionSyncCapture::SyncOp &i : capt.ops)
    {
      if (!uniqueResources.count(i.res))
        uniqueResources[i.res] = {uniqueResId++, 1, 0, 0};
      else
        ++uniqueResources[i.res].count;
    }
    // count links for resource
    for (ExecutionSyncCapture::SyncOpLink &i : capt.links)
    {
      ++uniqueResources[capt.ops[i.srcOpUid].res].links;
      ++uniqueResources[capt.ops[i.dstOpUid].res].links;
    }
  }

  // detect no link status and patch source data for following logic
  {
    uniqueResId = 0;
    for (auto &iter : uniqueResources)
    {
      if (iter.second.links)
        iter.second.id = uniqueResId++;
      else
      {
        for (ExecutionSyncCapture::SyncOp &i : capt.ops)
        {
          if (i.res == iter.first)
            i.noLink = 1;
        }
      }
    }

    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      uint32_t stepCounter = 0;
      for (auto &iter : uniqueResources)
      {
        for (ExecutionSyncCapture::SyncOp &j : capt.ops)
        {
          if (j.res != iter.first || i.idx != j.step)
            continue;

          iter.second.stepPos.push_back(stepCounter++);
        }
      }
    }
  }

  static const uint32_t INVALID_ORDER_LEVEL = 999999;
  // find "input" nodes, nodes that has no prev dep
  {
    capt.indepSeqs.clear();
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      bool hasLink = false;
      for (ExecutionSyncCapture::SyncOp &j : capt.ops)
      {
        if (j.step != i.idx)
          continue;
        // no dep for resource, no point in finding relations to it
        if (j.noLink)
          continue;
        for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
        {
          if (k.dstOpUid != j.uid)
            continue;
          hasLink = true;
          break;
        }
        if (hasLink)
          break;
      }
      if (!hasLink)
      {
        push_indep_node(i);
        i.orderLevel = 0;
      }
      else
        i.orderLevel = INVALID_ORDER_LEVEL;
    }
  }

  // find "output" nodes, nodes that has no follow deps
  {
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      if (i.visIndepSeq)
        continue;
      bool hasLink = false;
      for (ExecutionSyncCapture::SyncOp &j : capt.ops)
      {
        if (j.step != i.idx)
          continue;
        // no dep for resource, no point in finding relations to it
        if (j.noLink)
          continue;
        for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
        {
          if (k.srcOpUid != j.uid)
            continue;
          hasLink = true;
          break;
        }
        if (hasLink)
          break;
      }
      if (!hasLink)
        push_indep_node(i);
    }
  }

  // generate order levels for sync steps from input nodes to output nodes following dependencies
  {
    for (uint32_t l = 0; l < capt.steps.size() * 2; ++l)
    {
      // start level from min order pos of deps
      uint32_t ordPos = 0;
      for (ExecutionSyncCapture::SyncStep &i : capt.steps)
      {
        if (!i.orderLevel)
          continue;

        uint32_t minDepPos = ~0; // minimal initial
        bool skip = false;
        bool found = false;
        for (ExecutionSyncCapture::SyncOp &j : capt.ops)
        {
          if (j.step != i.idx)
            continue;
          for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
          {
            if (k.dstOpUid != j.uid)
              continue;
            if (capt.steps[capt.ops[k.srcOpUid].step].orderLevel > l)
            {
              skip = true;
              continue;
            }
            if (capt.steps[capt.ops[k.srcOpUid].step].orderLevel == l)
            {
              found = true;
              uint32_t depPos = capt.steps[capt.ops[k.srcOpUid].step].orderPos;
              if (minDepPos > depPos)
                minDepPos = depPos;
            }
          }
        }
        if (!skip && found)
        {
          if (minDepPos > ordPos)
            ordPos = minDepPos;
          i.orderLevel = l + 1;
          i.orderPos = ordPos;
          ++ordPos;
        }
      }
    }
  }

  // move inputs closer to outputs when possible (independant branches, branch join to bigger branch)
  if (moveInputsToOutputs)
  {
    for (uint32_t l = 0; l < capt.steps.size() * 2; ++l)
    {
      for (ExecutionSyncCapture::SyncStep &i : capt.steps)
      {
        uint32_t minDepLevel = INVALID_ORDER_LEVEL;
        for (ExecutionSyncCapture::SyncOp &j : capt.ops)
        {
          if (j.step != i.idx)
            continue;
          for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
          {
            if (k.srcOpUid != j.uid)
              continue;
            uint32_t depLevel = capt.steps[capt.ops[k.dstOpUid].step].orderLevel;
            if (minDepLevel > depLevel)
              minDepLevel = depLevel;
          }
        }
        if (minDepLevel != INVALID_ORDER_LEVEL && minDepLevel)
          i.orderLevel = minDepLevel - 1;
      }
    }
  }

  // do not generate invalid graph
  // if something goes haywire throw this to corner for analisis
  {
    uint32_t maxOrd = capt.steps.back().idx;
    uint32_t ordPosInvalidBase = 50;
    const uint32_t orderOffsetForSelfDep = 10;
    const uint32_t orderOffsetForUnkErr = 20;
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      if (i.orderLevel == INVALID_ORDER_LEVEL)
      {
        i.invalidOrdering = 1;
        i.orderLevel = 0;
        bool selfDep = false;
        for (ExecutionSyncCapture::SyncOp &j : capt.ops)
        {
          if (j.step != i.idx)
            continue;
          for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
          {
            if (k.dstOpUid != j.uid)
              continue;
            selfDep |= capt.ops[k.srcOpUid].step == i.idx;
          }
        }
        if (selfDep)
        {
          i.invalidOrdering = 2;
          i.orderLevel = maxOrd + orderOffsetForSelfDep;
        }
        else
          i.orderLevel = maxOrd + orderOffsetForUnkErr;
        i.orderPos = ordPosInvalidBase++;
      }
    }
  }

  // position ops nodes
  {
    uint32_t maxOrdPos = 0;
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
      if (maxOrdPos < i.orderPos)
        maxOrdPos = i.orderPos;

    uint32_t nonConflictingLocalIdx = 0;
    for (ExecutionSyncCapture::SyncOp &i : capt.ops)
    {
      if (i.noLink)
        capt.reformedPositions[i.visNode] = {0, nonConflictingLocalIdx++ * activeLayout.opSyncStepSpacing};
      else
      {
        uint32_t reorderedStepIdx = resSortMode == ResSortModes::UNIQ_RES ? capt.steps[i.step].orderLevel : capt.steps[i.step].idx;
        uint32_t sortingXOffset = 0;
        if (resSortMode == ResSortModes::ORIGINAL)
          sortingXOffset = i.localIdx * activeLayout.opPosStep;
        else if (resSortMode == ResSortModes::UNIQ_RES)
        {
          sortingXOffset += uniqueResources[i.res].localCounter++ * activeLayout.opPosStepSameRes;
          sortingXOffset += uniqueResources[i.res].id * activeLayout.opPosStep;
        }
        else
          sortingXOffset = uniqueResources[i.res].stepPos[uniqueResources[i.res].localCounter++] * activeLayout.opPosStep;

        capt.reformedPositions[i.visNode] = {                    //
          (maxOrdPos + 1) * activeLayout.opOrderSpacingX +       //
            reorderedStepIdx * activeLayout.opSyncStepSpacingX + //
            activeLayout.opNormalXBase +                         //
            activeLayout.opSyncStepLeft +                        //
            sortingXOffset,                                      //
          activeLayout.opSyncStepYOfs +                          //
            reorderedStepIdx * activeLayout.opSyncStepSpacing +  //
            activeLayout.opNormalYBase};
      }
    }
  }

  // position step nodes
  {
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      capt.reformedPositions[i.visNode] = {              //
        i.orderLevel * activeLayout.opSyncStepSpacingX + //
          activeLayout.opNormalXBase +                   //
          i.orderPos * activeLayout.opOrderSpacingX,     //
        i.orderLevel * activeLayout.opSyncStepSpacing};
    }
  }

  // position indep sequences nodes
  {
    for (ExecutionSyncCapture::NodePos &i : capt.reformedPositions)
    {
      i.y += activeLayout.sequencesYSpacing;
    }

    capt.reformedPositions.resize(capt.currentVisNode + 1);
    int indepSeqUid = 0;
    for (ExecutionSyncCapture::IndepSeq &i : capt.indepSeqs)
    {
      capt.reformedPositions[i.visIndepSeq] = {
        activeLayout.opNormalXBase + activeLayout.opSyncStepLeft + indepSeqUid++ * activeLayout.indepSeqSpacing, 0};
    }
  }
}

} // anonymous namespace

void drv3d_vulkan::debug_ui_sync()
{
  wnd_header();
  if (!request_capture_data_and_wait())
    return;

  if (!ectx)
  {
    NodeEditor::Config cfg{};
    ectx = NodeEditor::CreateEditor(&cfg);
  }

  NodeEditor::SetCurrentEditor(ectx);
  NodeEditor::Begin("sync_ngraph", ImVec2(0, 0));

  if (settleNodes)
    settle_nodes();

  uint32_t linkId = 1;

  ExecutionSyncCapture &capt = Frontend::syncCapture;

  {
    int indepSeqUid = 0;
    for (ExecutionSyncCapture::IndepSeq &i : capt.indepSeqs)
    {
      NodeEditor::BeginNode(i.visIndepSeq);
      ImGui::Text("IndepSeq %u %s", indepSeqUid++, i.cat ? "end" : "start");
      NodeEditor::BeginPin(i.visBufPin, NodeEditor::PinKind::Output);
      ImGui::Text("Step");
      NodeEditor::EndPin();
      NodeEditor::EndNode();
    }
  }

  for (ExecutionSyncCapture::SyncStep &i : capt.steps)
  {
    NodeEditor::BeginNode(i.visNode);
    ImGui::Text("Step %u [buffer %u, queue %u]", i.idx, i.dstBuffer, i.dstQueue);
    if (i.invalidOrdering)
      ImGui::Text("Ordering sorting failed %u", i.invalidOrdering);
    NodeEditor::BeginPin(i.visBufPin, NodeEditor::PinKind::Output);
    ImGui::Text("buffer");
    NodeEditor::EndPin();
    NodeEditor::BeginPin(i.visIdepSeqPin, NodeEditor::PinKind::Input);
    ImGui::Text("indepSeq");
    NodeEditor::EndPin();
    NodeEditor::BeginPin(i.visPrevDepPin, NodeEditor::PinKind::Input);
    ImGui::Text("prevdep");
    NodeEditor::EndPin();
    NodeEditor::BeginPin(i.visNextDepPin, NodeEditor::PinKind::Input);
    ImGui::Text("nextdep");
    NodeEditor::EndPin();
    ImGui::SameLine();
    NodeEditor::BeginPin(i.visOpsPin, NodeEditor::PinKind::Output);
    ImGui::Text("ops");
    NodeEditor::EndPin();
    if (NodeEditor::IsNodeSelected(i.visNode))
    {
      ImGui::Text("External: %s", i.caller.getExternal().c_str());
      ImGui::Text("Internal: %s", i.caller.getInternal().c_str());
      NodeEditor::SetNodeZPosition(i.visNode, 10);
    }
    else if (NodeEditor::HasSelectionChanged())
      NodeEditor::SetNodeZPosition(i.visNode, 0);
    NodeEditor::EndNode();
    if (i.visIndepSeq)
      NodeEditor::Link(linkId++, capt.indepSeqs[i.visIndepSeq - 1].visBufPin, i.visIdepSeqPin);
  }

  for (ExecutionSyncCapture::SyncOp &i : capt.ops)
  {
    NodeEditor::BeginNode(i.visNode);

    NodeEditor::BeginPin(i.visStepPin, NodeEditor::PinKind::Input);
    ImGui::Text("step");
    NodeEditor::EndPin();
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(activeLayout.opPosStep / 2, 0));
    ImGui::SameLine();
    NodeEditor::BeginPin(i.visDstPin, NodeEditor::PinKind::Output);
    ImGui::Text("dst");
    NodeEditor::EndPin();


    ImGui::Text("Op %u:%p %s", i.uid, i.res, i.resName.c_str());
    ImGui::Text("%s", formatPipelineStageFlags(i.laddr.stage).c_str());
    ImGui::Text("%s", formatMemoryAccessFlags(i.laddr.access).c_str());
    NodeEditor::BeginPin(i.visSrcPin, NodeEditor::PinKind::Input);
    ImGui::Text("src");
    NodeEditor::EndPin();
    if (NodeEditor::IsNodeSelected(i.visNode))
    {
      ImGui::Text("External: %s", i.caller.getExternal().c_str());
      ImGui::Text("Internal: %s", i.caller.getInternal().c_str());
      NodeEditor::SetNodeZPosition(i.visNode, 10);
    }
    else if (NodeEditor::HasSelectionChanged())
      NodeEditor::SetNodeZPosition(i.visNode, 0);
    NodeEditor::EndNode();
    if (showStepLinks == StepLinksMode::NO)
      continue;
    if (i.localIdx == 0 || i.noLink || showStepLinks == StepLinksMode::ALL)
      NodeEditor::Link(linkId++, capt.steps[i.step].visOpsPin, i.visStepPin);
    else
    {
      ExecutionSyncCapture::SyncOp *prevOp = &i;
      while (prevOp > capt.ops.begin() && prevOp->step == i.step)
      {
        --prevOp;
        if (!prevOp->noLink)
          break;
      }
      if (prevOp >= capt.ops.begin() && prevOp->step == i.step)
        NodeEditor::Link(linkId++, i.visStepPin, prevOp->visStepPin);
      else
        NodeEditor::Link(linkId++, capt.steps[i.step].visOpsPin, i.visStepPin);
    }
  }

  // setup links between sync steps
  {
    ska::flat_hash_map<uint32_t, uint32_t> uniqueSrc; // src for step = follows
    for (ExecutionSyncCapture::SyncStep &i : capt.steps)
    {
      uniqueSrc.clear();
      for (ExecutionSyncCapture::SyncOp &j : capt.ops)
      {
        if (j.step != i.idx)
          continue;
        if (j.noLink)
          continue;
        for (ExecutionSyncCapture::SyncOpLink &k : capt.links)
        {
          if (k.dstOpUid == j.uid)
            uniqueSrc[capt.ops[k.srcOpUid].step] = 1;
        }
      }
      for (auto j : uniqueSrc)
        NodeEditor::Link(linkId++, capt.steps[j.first].visNextDepPin, i.visPrevDepPin, ImVec4(0, 1, 0, 1));
    }
  }

  for (ExecutionSyncCapture::SyncOpLink &i : capt.links)
    NodeEditor::Link(linkId++, capt.ops[i.srcOpUid].visSrcPin, capt.ops[i.dstOpUid].visDstPin, ImVec4(0, 0, 1, 1));

  for (ExecutionSyncCapture::SyncBufferLink &i : capt.bufferLinks)
  {
    uint32_t lastStepOnSrcBuffer = ~0;
    uint32_t firstStepOnDstBuffer = ~0;
    for (ExecutionSyncCapture::SyncStep &j : capt.steps)
    {
      if (j.dstBuffer == i.srcBuffer && j.dstQueue == i.srcQueue)
        lastStepOnSrcBuffer = j.idx;
      if (j.dstBuffer == i.dstBuffer && j.dstQueue == i.dstQueue && firstStepOnDstBuffer == ~0)
        firstStepOnDstBuffer = j.idx;
    }
    if (lastStepOnSrcBuffer == ~0 || firstStepOnDstBuffer == ~0)
      continue;
    NodeEditor::Link(linkId++, capt.steps[lastStepOnSrcBuffer].visNextDepPin, capt.steps[firstStepOnDstBuffer].visPrevDepPin,
      ImVec4(1, 0, 0, 1));
  }

  // set positions for nodes after all nodes are "drawn"
  if (settleNodes)
  {
    for (int i = 1; i < capt.currentVisNode; ++i)
    {
      if (swapCoord)
        NodeEditor::SetNodePosition(i, ImVec2(capt.reformedPositions[i].y, capt.reformedPositions[i].x));
      else
        NodeEditor::SetNodePosition(i, ImVec2(capt.reformedPositions[i].x, capt.reformedPositions[i].y));
    }
  }

  NodeEditor::End();

  if (resetView)
    NodeEditor::NavigateToContent(0.0f);
  NodeEditor::SetCurrentEditor(nullptr);

  settleNodes = false;
}

#else // EXECUTION_SYNC_DEBUG_CAPTURE > 0

void drv3d_vulkan::debug_ui_sync() { ImGui::Text("Compile with EXECUTION_SYNC_DEBUG_CAPTURE > 0 to enable this feature"); }

#endif // EXECUTION_SYNC_DEBUG_CAPTURE > 0
#endif // DAGOR_DBGLEVEL > 0
