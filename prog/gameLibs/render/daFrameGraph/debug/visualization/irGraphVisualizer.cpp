// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "irGraphVisualizer.h"

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <generic/dag_sort.h>
#include <generic/dag_reverseView.h>


constexpr float NODE_MIN_X_SIZE = 300.f;
constexpr float NODE_MIN_Y_SIZE = 200.f;
constexpr ImVec2 NODE_BORDER_PADDING = ImVec2{10.f, 10.f};

constexpr float NODE_BLOCK_WIDTH = 300.f;
constexpr float NODE_BLOCK_BORDER_WIDTH = 2.f;
constexpr ImVec2 NODE_BLOCK_BORDER_PADDING = ImVec2{10.f, 10.f};

constexpr float NODE_IN_PASS_MARGIN = 150.f;
constexpr float NODE_BETWEEN_PASS_MARGIN = 600.f;

constexpr float PASS_BRACKET_OFFSET = 50.f;
constexpr float PASS_BRACKET_WIDTH = 10.f;
constexpr float PASS_BLOCK_WIDTH = 300.f;
constexpr ImVec2 PASS_BLOCK_BORDER_PADDING = ImVec2{PASS_BRACKET_WIDTH, PASS_BRACKET_WIDTH};

constexpr float STATE_DELTA_BLOCK_WIDTH = 300.f;
constexpr ImVec2 STATE_DELTA_BLOCK_BORDER_PADDING = ImVec2{10.f, 10.f};
constexpr float STATE_DELTA_BLOCK_MARGIN = 50.f;


constexpr float RES_BLOCK_OFFSCREEN_PADDING = 20.f;
constexpr ImVec2 RES_BORDER_PADDING = ImVec2{10.f, 10.f};

constexpr float RES_BLOCK_WIDTH = 500.f;
constexpr float RES_BLOCK_BORDER_WIDTH = 2.f;
constexpr ImVec2 RES_BLOCK_BORDER_PADDING = ImVec2{10.f, 10.f};

constexpr float RES_LINE_OFFSET = 300.f;
constexpr float RES_LINE_Y_SIZE = 150.f;
constexpr float RES_LINE_MARGIN = 100.f;


constexpr ImVec2 TAG_BORDER_PADDING = ImVec2{5.f, 5.f};
constexpr float FOCUS_BORDER_WIDTH = 5.f;

constexpr float ORDERING_WIDTH = 5.f;
constexpr float ORDERINGS_MARGIN = 5.f;

constexpr float USAGE_WIDTH = 10.f;
constexpr float USAGES_MARGIN = 15.f;


constexpr ImU32 NODE_BASE_COLOR = IM_COL32(128, 128, 128, 255); // rgb(128, 128, 128)
constexpr ImU32 NODE_BLOCK_COLOR = IM_COL32(64, 64, 64, 255);   // rgb(64, 64, 64)

constexpr ImU32 NODE_START_PASS_COLOR = IM_COL32(64, 196, 64, 255);        // rgb(64, 196, 64)
constexpr ImU32 NODE_END_PASS_COLOR = IM_COL32(196, 64, 64, 255);          // rgb(196, 64, 64)
constexpr ImU32 NODE_START_LEGACY_PASS_COLOR = IM_COL32(64, 64, 196, 255); // rgb(64, 64, 196)
constexpr ImU32 NODE_END_LEGACY_PASS_COLOR = IM_COL32(196, 64, 196, 255);  // rgb(196, 64, 196)
constexpr ImU32 NODE_NEXT_SUBPASS_COLOR = IM_COL32(196, 196, 64, 255);     // rgb(196, 196, 64)

constexpr ImU32 RES_BASE_COLOR = IM_COL32(64, 64, 64, 255);  // rgb(64, 64, 64)
constexpr ImU32 RES_BLOCK_COLOR = IM_COL32(32, 32, 32, 255); // rgb(32, 32, 32)

constexpr ImU32 RES_TEX_TYPE_COLOR = IM_COL32(16, 192, 16, 255);   // rgb(16, 192, 16)
constexpr ImU32 RES_TEX_TAG_COLOR = IM_COL32(16, 96, 16, 255);     // rgb(16, 96, 16)
constexpr ImU32 RES_BUF_TYPE_COLOR = IM_COL32(16, 16, 192, 255);   // rgb(16, 16, 192)
constexpr ImU32 RES_BUF_TAG_COLOR = IM_COL32(16, 16, 96, 255);     // rgb(16, 16, 96)
constexpr ImU32 RES_BLOB_TYPE_COLOR = IM_COL32(192, 128, 16, 255); // rgb(192, 128, 16)
constexpr ImU32 RES_BLOB_TAG_COLOR = IM_COL32(96, 64, 16, 255);    // rgb(96, 64, 16)

constexpr ImU32 RES_SHC_TYPE_COLOR = IM_COL32(64, 192, 192, 255); // rgb(64, 192, 192)
constexpr ImU32 RES_SHC_TAG_COLOR = IM_COL32(64, 96, 96, 255);    // rgb(64, 96, 96)
constexpr ImU32 RES_EXT_TYPE_COLOR = IM_COL32(192, 64, 192, 255); // rgb(192, 64, 192)
constexpr ImU32 RES_EXT_TAG_COLOR = IM_COL32(96, 64, 96, 255);    // rgb(96, 64, 96)
constexpr ImU32 RES_DEF_TYPE_COLOR = IM_COL32(0, 128, 64, 255);   // rgb(0, 128, 64)
constexpr ImU32 RES_DEF_TAG_COLOR = IM_COL32(0, 64, 64, 255);     // rgb(0, 64, 64)
constexpr ImU32 RES_HIS_TAG_COLOR = IM_COL32(128, 128, 0, 255);   // rgb(128, 128, 0)

constexpr ImU32 ORDERING_BASE_COLOR = IM_COL32(128, 255, 128, 255); // rgb(128, 192, 128)
constexpr ImU32 USAGE_BASE_COLOR = IM_COL32(255, 128, 255, 255);    // rgb(192, 128, 192)

constexpr ImU32 POS_COLOR = IM_COL32(64, 192, 64, 255); // rgb(64, 192, 64)
constexpr ImU32 NEG_COLOR = IM_COL32(192, 64, 64, 255); // rgb(192, 64, 64)


constexpr const char *NODE_NOOPT_LABEL = " - NONE";
constexpr const char *NODE_NORESREF_LABEL = "  NULL";
constexpr const char *NODE_VRS_MAX_LABEL = "VRS state:\n  rates X/Y: x x\n  vtx combiner: PASSTHROUGH\n  pix combiner: PASSTHROUGH\n";

constexpr const char *NODE_START_PASS_TAG = "START PASS";
constexpr const char *NODE_END_PASS_TAG = "END PASS";
constexpr const char *NODE_START_LEGACY_PASS_TAG = "START PASS (LEGACY)";
constexpr const char *NODE_END_LEGACY_PASS_TAG = "END PASS (LEGACY)";
constexpr const char *NODE_NEXT_SUBPASS_TAG = "NEXT SUBPASS";


constexpr const char *RES_INV_TYPE_TAG = "INVALID TYPE";
constexpr const char *RES_TEXTURE_TAG = "TEXTURE";
constexpr const char *RES_BUFFER_TAG = "BUFFER";
constexpr const char *RES_BLOB_TAG = "BLOB";

constexpr const char *RES_UNKNOWN_STATUS_TAG = "Unknown resource status !(Scheduled || External || Backbuffer)";
constexpr const char *RES_SHCEDULE_TAG = "SCHEDULED";
constexpr const char *RES_EXTERNAL_TAG = "EXTERNAL";
constexpr const char *RES_DEFERRED_TAG = "BACKBUFFER";

constexpr const char *RES_HISTORY_CLEAR_TAG = "HISTORY : CLEAR";
constexpr const char *RES_HISTORY_DISCARD_TAG = "HISTORY : DISCARD";

constexpr const char *RES_FRONTEND_POPUP = "res_frontend_resources";


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

namespace visualization::irgraph
{

Visualizer::Visualizer(const InternalRegistry &int_registry, const intermediate::Graph &ir_graph, const PassColoring &coloring,
  const sd::NodeStateDeltas &state_deltas) :
  registry{int_registry}, intermediateGraph{ir_graph}, passColoring(coloring), stateDeltas(state_deltas)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_IRG_WIN_NAME, [&]() { this->draw(); });
}

void Visualizer::draw()
{
  if (ImGui::IsWindowCollapsed())
    return;

  if (updateNeeded)
    updateVisualization();

  hoverState.reset();
  hoverState.window = ImGui::IsWindowHovered();


  drawUI();

  drawCanvas();

  processInput();

  processPopup();
}


void Visualizer::drawUI()
{
  // 1 line
  {
    if (ImGui::BeginCombo("##node_search",
          focusState.nodeValid() ? intermediateGraph.nodeNames[nodes[focusState.node].irIndex].c_str() : "Node Search"))
    {
      for (auto [nodeId, node] : nodes.enumerate())
      {
        const bool selected = nodeId == focusState.node;
        if (ImGui::Selectable(intermediateGraph.nodeNames[node.irIndex].c_str(), selected))
          focusState.set(nodeId);
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    hoverState.searchBox |= ImGui::IsItemHovered();
  }

  // 2 line
  {
    if (ImGui::BeginCombo("##resource_search",
          focusState.resValid() ? intermediateGraph.resourceNames[resources[focusState.resource].irIndex].c_str() : "Resource Search"))
    {
      for (auto [resId, resource] : resources.enumerate())
      {
        const bool selected = resId == focusState.resource;
        if (ImGui::Selectable(intermediateGraph.resourceNames[resource.irIndex].c_str(), selected))
          focusState.set(resId);
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    hoverState.searchBox |= ImGui::IsItemHovered();
  }

  // 3 line
  {
    if (ImGui::Button("Recompile"))
      recompile_graph.set(true);

    ImGui::SameLine();
    if (ImGui::Button("Reset view"))
    {
      canvasCamera.reset();
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset focus"))
      focusState.reset();
  }

  // 4 line
  {
    ImGui::TextUnformatted("double left click on element to set focus");
  }
}

void Visualizer::drawCanvas()
{
  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  hoverState.canvas = canvas.ViewRect().Contains(ImGui::GetMousePos());
  checkHovering(generalLayout);


  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->ChannelsSplit(CanvasChannels::COUNT);
  {
    drawNodes(drawList, generalLayout);
    drawResources(drawList, generalLayout);
    drawOrderings(drawList, generalLayout);
    drawUsages(drawList, generalLayout);
  }
  drawList->ChannelsMerge();

  canvas.End();
}

void Visualizer::drawNodes(ImDrawList *draw_list, const CanvasLayout &layout)
{
  for (const auto [nodeId, rect] : layout.nodes.enumerate())
  {
    const auto nodeStart = rect.offset;
    const auto nodeEnd = nodeStart + rect.size;


    draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);

    const auto &node = nodes[nodeId];
    const auto &irNode = intermediateGraph.nodes[node.irIndex];
    const auto &state = intermediateGraph.nodeStates[node.irIndex];
    const auto &nodeName = intermediateGraph.nodeNames[node.irIndex];

    static String label;
    ImVec2 selectibleMaxSize = {NODE_BLOCK_WIDTH, 0.f};
    const ImVec2 contentsStart = nodeStart + NODE_BORDER_PADDING;

    const ImVec2 nameBlockStart = contentsStart;
    ImGui::SetCursorScreenPos(nameBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::TextUnformatted(nodeName.c_str());

      if (irNode.frontendNode)
      {
        label.printf(0, "frontend node##%u", static_cast<uint32_t>(nodeId));

        ImGui::SameLine();
        if (ImGui::Button(label, selectibleMaxSize / 2.5f))
        {
          request_user_node_focus(*irNode.frontendNode);
          imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true);
        }
      }

      ImGui::Text("priority: %d  multiplex: %d  side effects: %c", irNode.priority, irNode.multiplexingIndex,
        irNode.hasSideEffects ? 'Y' : 'N');
    }
    ImGui::EndGroup();
    const ImVec2 nameBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 passBlockStart = contentsStart + ImVec2{0.f, nameBlockSize.y};
    ImGui::SetCursorScreenPos(passBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::Text("RENDER PASS%s", state.pass ? (state.pass->isLegacyPass ? " (LEGACY)" : "") : NODE_NOOPT_LABEL);
      if (state.pass)
      {
        const auto rp = *state.pass;

        if (!rp.colorAttachments.empty())
        {
          ImGui::TextUnformatted("Color attachments");
          for (const auto &colAtt : rp.colorAttachments)
            if (colAtt)
            {
              const auto irResIndex = colAtt->resource;
              label.printf(0, "  (%d,%d) %s##%u", colAtt->mipLevel, colAtt->layer, intermediateGraph.resourceNames[irResIndex],
                static_cast<uint32_t>(nodeId));

              if (ImGui::Selectable(label, focusState.resource == irResRepresent[irResIndex], 0, selectibleMaxSize))
                focusState.set(irResRepresent[irResIndex]);
              if (ImGui::IsItemHovered())
              {
                hoverState.node = NodeId::Invalid;
                hoverState.resource = irResRepresent[irResIndex];
              }
            }
            else
            {
              ImGui::TextUnformatted(NODE_NORESREF_LABEL);
            }
        }

        if (rp.depthAttachment)
        {
          ImGui::Text("Depth attachment (%s)", rp.depthReadOnly ? "RO" : "RW");

          const auto irResIndex = rp.depthAttachment->resource;
          label.printf(0, "  (%d,%d) %s##%u", rp.depthAttachment->mipLevel, rp.depthAttachment->layer,
            intermediateGraph.resourceNames[irResIndex], static_cast<uint32_t>(nodeId));

          if (ImGui::Selectable(label, focusState.resource == irResRepresent[irResIndex], 0, selectibleMaxSize))
            focusState.set(irResRepresent[irResIndex]);
          if (ImGui::IsItemHovered())
          {
            hoverState.node = NodeId::Invalid;
            hoverState.resource = irResRepresent[irResIndex];
          }
        }

        if (rp.vrsRateAttachment)
        {
          ImGui::TextUnformatted("VRS texture");

          const auto irResIndex = rp.vrsRateAttachment->resource;
          label.printf(0, "  (%d,%d) %s##%u", rp.vrsRateAttachment->mipLevel, rp.vrsRateAttachment->layer,
            intermediateGraph.resourceNames[irResIndex], static_cast<uint32_t>(nodeId));

          if (ImGui::Selectable(label, focusState.resource == irResRepresent[irResIndex], 0, selectibleMaxSize))
            focusState.set(irResRepresent[irResIndex]);
          if (ImGui::IsItemHovered())
          {
            hoverState.node = NodeId::Invalid;
            hoverState.resource = irResRepresent[irResIndex];
          }
        }

        if (!rp.resolves.empty())
        {
          ImGui::TextUnformatted("MSAA resolves");
          for (const auto &pair : rp.resolves)
          {
            label.printf(0, "%s##%u", intermediateGraph.resourceNames[pair.first], static_cast<uint32_t>(nodeId));
            if (ImGui::Button(label, selectibleMaxSize / 2.5f))
              focusState.set(irResRepresent[pair.first]);
            ImGui::SameLine();
            ImGui::TextUnformatted(" -> ");
            ImGui::SameLine();
            label.printf(0, "%s##%u", intermediateGraph.resourceNames[pair.second], static_cast<uint32_t>(nodeId));
            if (ImGui::Button(label, selectibleMaxSize / 2.5f))
              focusState.set(irResRepresent[pair.second]);
          }
        }
      }
    }
    ImGui::EndGroup();
    const ImVec2 passBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 ivbBlockStart = contentsStart + ImVec2{0.f, nameBlockSize.y + passBlockSize.y};
    ImGui::SetCursorScreenPos(ivbBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      bool hasIBuf = bool(state.indexSource);
      bool hasVBuf = false;
      for (const auto &vtxBuf : state.vertexSources)
        hasVBuf |= bool(vtxBuf);

      ImGui::Text("IDX/VTX BUF%s", hasIBuf || hasVBuf ? "" : NODE_NOOPT_LABEL);

      if (hasIBuf)
      {
        ImGui::TextUnformatted("Index buffer");

        if (state.indexSource->buffer)
        {
          const auto irBufIndex = *(state.indexSource->buffer);
          label.printf(0, "  %s##%u", intermediateGraph.resourceNames[irBufIndex], static_cast<uint32_t>(nodeId));

          if (ImGui::Selectable(label, focusState.resource == irResRepresent[irBufIndex], 0, selectibleMaxSize))
            focusState.set(irResRepresent[irBufIndex]);
          if (ImGui::IsItemHovered())
          {
            hoverState.node = NodeId::Invalid;
            hoverState.resource = irResRepresent[irBufIndex];
          }
        }
        else
        {
          ImGui::TextUnformatted(NODE_NORESREF_LABEL);
        }
      }

      if (hasVBuf)
      {
        ImGui::TextUnformatted("Vertex buffer");

        for (const auto &bufSrc : state.vertexSources)
        {
          if (bufSrc && bufSrc->buffer)
          {
            const auto irBufIndex = *(bufSrc->buffer);
            label.printf(0, "  (%d) %s##%u", bufSrc->stride, intermediateGraph.resourceNames[irBufIndex],
              static_cast<uint32_t>(nodeId));

            if (ImGui::Selectable(label, focusState.resource == irResRepresent[irBufIndex], 0, selectibleMaxSize))
              focusState.set(irResRepresent[irBufIndex]);
            if (ImGui::IsItemHovered())
            {
              hoverState.node = NodeId::Invalid;
              hoverState.resource = irResRepresent[irBufIndex];
            }
          }
          else
          {
            ImGui::TextUnformatted(NODE_NORESREF_LABEL);
          }
        }
      }
    }
    ImGui::EndGroup();
    const ImVec2 ivbBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 bindBlockStart = contentsStart + ImVec2{max(passBlockSize.x, ivbBlockSize.x), nameBlockSize.y};
    ImGui::SetCursorScreenPos(bindBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::Text("BINDINGS%s", !state.bindings.empty() ? "" : NODE_NOOPT_LABEL);
      if (!state.bindings.empty())
      {
        ImGui::TextUnformatted("idx: type (hist?)(reset?)(opt?) name");
        if (ImGui::IsItemHovered())
          hoverState.tooltip.printf(0, "SV - ShaderVar\nVM - ViewMatrix\nPM - ProjMatrix\nIN - Invalid\n");
        for (const auto &[index, binding] : state.bindings)
        {
          if (binding.resource)
          {
            label.printf(0, "  %d: %s %c%c%c %s##%u", index, bind_type_name(binding.type), binding.history ? '+' : '-',
              binding.reset ? '+' : '-', binding.optional ? '+' : '-', intermediateGraph.resourceNames[*binding.resource],
              static_cast<uint32_t>(nodeId));

            const auto bindResource = irResRepresent[*binding.resource];
            if (ImGui::Selectable(label, focusState.resource == bindResource, 0, selectibleMaxSize))
              focusState.set(bindResource);
            if (ImGui::IsItemHovered())
            {
              hoverState.node = NodeId::Invalid;
              hoverState.resource = bindResource;
            }
          }
          else
          {
            ImGui::Text("  %d: %s", index, NODE_NORESREF_LABEL);
          }
        }
      }
    }
    ImGui::EndGroup();
    const ImVec2 bindBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 overrideBlockStart = contentsStart + ImVec2{max(passBlockSize.x, ivbBlockSize.x) + bindBlockSize.x, nameBlockSize.y};
    ImGui::SetCursorScreenPos(overrideBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::Text("STATE OVERRIDES%s", state.shaderOverrides ? "" : NODE_NOOPT_LABEL);
      if (state.shaderOverrides)
        ImGui::TextUnformatted(override_state_descr(*state.shaderOverrides));
    }
    ImGui::EndGroup();
    const ImVec2 overrideBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 vrsBlockStart =
      contentsStart + ImVec2{max(passBlockSize.x, ivbBlockSize.x) + bindBlockSize.x, nameBlockSize.y + overrideBlockSize.y};
    ImGui::SetCursorScreenPos(vrsBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::Text("VRS state:\n  rates X/Y: %d %d\n  vtx combiner: %s\n  pix combiner: %s\n", state.vrs.rateX, state.vrs.rateY,
        vrs_combiner_name(state.vrs.vertexCombiner), vrs_combiner_name(state.vrs.pixelCombiner));
    }
    ImGui::EndGroup();
    const ImVec2 vrsBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    const ImVec2 miscBlockStart = contentsStart + ImVec2{max(passBlockSize.x, ivbBlockSize.x) + bindBlockSize.x,
                                                    nameBlockSize.y + overrideBlockSize.y + vrsBlockSize.y};
    ImGui::SetCursorScreenPos(miscBlockStart + NODE_BLOCK_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::Text("async pipelines: %c", state.asyncPipelines ? 'Y' : 'N');
      ImGui::Text("wireframe: %c", state.wire ? 'Y' : 'N');
      ImGui::Text("sh block layers (ob,fr,sc): %d %d %d", state.shaderBlockLayers.objectLayer, state.shaderBlockLayers.frameLayer,
        state.shaderBlockLayers.sceneLayer);
    }
    ImGui::EndGroup();
    const ImVec2 miscBlockSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;


    {
      const auto &stateDelta = stateDeltas[node.irIndex];

      ImVec2 deltaBlockStart = {(nodeStart.x + nodeEnd.x) / 2.f - STATE_DELTA_BLOCK_WIDTH / 2.f, nodeStart.y};
      {
        selectibleMaxSize = {STATE_DELTA_BLOCK_WIDTH - 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.x, 0.f};
        const float lineHeight = ImGui::GetFrameHeight();
        const auto startSDBlock = [=, &deltaBlockStart](const float block_height) {
          deltaBlockStart.y -= STATE_DELTA_BLOCK_MARGIN + block_height;
          draw_list->ChannelsSetCurrent(CanvasChannels::NODES);
          draw_list->AddRectFilled(deltaBlockStart, deltaBlockStart + ImVec2{STATE_DELTA_BLOCK_WIDTH, block_height}, NODE_BASE_COLOR,
            2.f, 0);
          draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);
          ImGui::SetCursorScreenPos(deltaBlockStart + STATE_DELTA_BLOCK_BORDER_PADDING);
          ImGui::BeginGroup();
        };
        const auto endSDBlock = []() { ImGui::EndGroup(); };

        if (stateDelta.shaderBlockLayers.objectLayer || stateDelta.shaderBlockLayers.frameLayer ||
            stateDelta.shaderBlockLayers.sceneLayer)
        {
          startSDBlock(lineHeight + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Shader Block Layers:");
            ImGui::SameLine();
            if (stateDelta.shaderBlockLayers.objectLayer)
              ImGui::Text("%d", *stateDelta.shaderBlockLayers.objectLayer);
            else
              ImGui::TextUnformatted("_");
            ImGui::SameLine();
            if (stateDelta.shaderBlockLayers.frameLayer)
              ImGui::Text("%d", *stateDelta.shaderBlockLayers.frameLayer);
            else
              ImGui::TextUnformatted("_");
            ImGui::SameLine();
            if (stateDelta.shaderBlockLayers.sceneLayer)
              ImGui::Text("%d", *stateDelta.shaderBlockLayers.sceneLayer);
            else
              ImGui::TextUnformatted("_");
          }
          endSDBlock();
        }
        if (!stateDelta.bindings.empty())
        {
          startSDBlock(lineHeight * (1 + stateDelta.bindings.size()) + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Set Bindings:");
            if (ImGui::IsItemHovered())
              hoverState.tooltip.printf(0,
                "idx: type (hist?)(reset?)(opt?) name\nSV - ShaderVar\nVM - ViewMatrix\nPM - ProjMatrix\nIN - Invalid\n");

            for (const auto &[index, binding] : stateDelta.bindings)
            {
              if (binding.resource)
              {
                label.printf(0, "  %d: %s %s%s%s %s##sd%u", index, bind_type_name(binding.type), binding.history ? "+" : "-",
                  binding.reset ? "+" : "-", binding.optional ? "+" : "-", intermediateGraph.resourceNames[*binding.resource],
                  static_cast<uint32_t>(nodeId));

                const auto bindResource = irResRepresent[*binding.resource];
                if (ImGui::Selectable(label, focusState.resource == bindResource, 0, selectibleMaxSize))
                  focusState.set(bindResource);
                if (ImGui::IsItemHovered())
                {
                  hoverState.node = NodeId::Invalid;
                  hoverState.resource = bindResource;
                }
              }
              else
              {
                ImGui::Text("  %d: %s", index, NODE_NORESREF_LABEL);
              }
            }
          }
          endSDBlock();
        }

        uint32_t vtxBufDeltas = 0;
        for (const auto optBuf : stateDelta.vertexSources)
          vtxBufDeltas += bool(optBuf) ? 1 : 0;
        if (vtxBufDeltas > 0)
        {
          startSDBlock(lineHeight * (1 + vtxBufDeltas) + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Set Vervex Buffers:");
            for (const auto optBuf : stateDelta.vertexSources)
              if (optBuf && optBuf->buffer)
              {
                const auto irBufIndex = *(optBuf->buffer);
                label.printf(0, "  (%d) %s##%u", intermediateGraph.resourceNames[irBufIndex], optBuf->stride,
                  static_cast<uint32_t>(nodeId));

                if (ImGui::Selectable(label, focusState.resource == irResRepresent[irBufIndex], 0, selectibleMaxSize))
                  focusState.set(irResRepresent[irBufIndex]);
                if (ImGui::IsItemHovered())
                {
                  hoverState.node = NodeId::Invalid;
                  hoverState.resource = irResRepresent[irBufIndex];
                }
              }
              else
              {
                ImGui::TextUnformatted(NODE_NORESREF_LABEL);
              }
          }
          endSDBlock();
        }

        if (stateDelta.indexSource)
        {
          startSDBlock(2.f * lineHeight + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Set Index Buffer:");
            if (stateDelta.indexSource->buffer)
            {
              const auto irBufIndex = *(stateDelta.indexSource->buffer);
              label.printf(0, "  %s##%u", intermediateGraph.resourceNames[irBufIndex], static_cast<uint32_t>(nodeId));

              if (ImGui::Selectable(label, focusState.resource == irResRepresent[irBufIndex], 0, selectibleMaxSize))
                focusState.set(irResRepresent[irBufIndex]);
              if (ImGui::IsItemHovered())
              {
                hoverState.node = NodeId::Invalid;
                hoverState.resource = irResRepresent[irBufIndex];
              }
            }
            else
            {
              ImGui::TextUnformatted(NODE_NORESREF_LABEL);
            }
          }
          endSDBlock();
        }

        if (stateDelta.shaderOverrides)
        {
          const char *soLabel = override_state_descr(shaders::overrides::get(*stateDelta.shaderOverrides));
          startSDBlock(lineHeight + ImGui::CalcTextSize(soLabel).y + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Set State Overrides:");
            ImGui::TextUnformatted(soLabel);
          }
          endSDBlock();
        }

        if (stateDelta.vrs)
        {
          const auto &vrsState = *stateDelta.vrs;
          startSDBlock(lineHeight + ImGui::CalcTextSize(NODE_VRS_MAX_LABEL).y + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          {
            ImGui::TextUnformatted("Set VRS state:");
            ImGui::Text("  rates X/Y: %d %d\n  vtx combiner: %s\n  pix combiner: %s\n", vrsState.rateX, vrsState.rateY,
              vrs_combiner_name(vrsState.vertexCombiner), vrs_combiner_name(vrsState.pixelCombiner));
          }
          endSDBlock();
        }

        if (stateDelta.wire)
        {
          startSDBlock(lineHeight + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          ImGui::Text("Set wireframe: %s", *stateDelta.wire ? "Y" : "N");
          endSDBlock();
        }

        if (stateDelta.asyncPipelines)
        {
          startSDBlock(lineHeight + 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y);
          ImGui::Text("Set async pipelines: %s", *stateDelta.asyncPipelines ? "Y" : "N");
          endSDBlock();
        }
      }

      {
        ImU32 passTagColor = 0;
        const char *passLabel = nullptr;

        const float leftPos = nodeStart.x - PASS_BRACKET_OFFSET;
        const float rightPos = nodeEnd.x + PASS_BRACKET_OFFSET;
        float topPos = 0.f;
        float middlePos = 0.f;
        float bottomPos = 0.f;

        const auto drawBracket = [=, &passTagColor, &passLabel, &topPos, &middlePos, &bottomPos]() {
          const ImVec2 bracketP0 = ImVec2{leftPos, bottomPos};
          const ImVec2 bracketP1 = ImVec2{leftPos, topPos};
          const ImVec2 bracketP2 = ImVec2{leftPos, middlePos};
          const ImVec2 bracketP3 = ImVec2{rightPos, middlePos};
          const ImVec2 bracketP4 = ImVec2{rightPos, topPos};
          const ImVec2 bracketP5 = ImVec2{rightPos, bottomPos};

          const ImVec2 passTagSize = ImGui::CalcTextSize(passLabel) + 2.f * TAG_BORDER_PADDING;
          const ImVec2 passTagStart = ImVec2{leftPos - TAG_BORDER_PADDING.x, middlePos - passTagSize.y / 2.f};
          const ImVec2 passTagEnd = passTagStart + passTagSize;

          draw_list->AddLine(bracketP0, bracketP1, passTagColor, PASS_BRACKET_WIDTH);
          draw_list->AddLine(bracketP2, bracketP3, passTagColor, PASS_BRACKET_WIDTH);
          draw_list->AddLine(bracketP4, bracketP5, passTagColor, PASS_BRACKET_WIDTH);

          draw_list->AddRectFilled(passTagStart, passTagEnd, passTagColor, passTagSize.y / 2.f);
          ImGui::SetCursorScreenPos(passTagStart + TAG_BORDER_PADDING);
          ImGui::TextUnformatted(passLabel);
        };

        if (stateDelta.pass)
        {
          bool firstInPass = false;

          if (eastl::holds_alternative<sd::PassChange>(*stateDelta.pass))
          {
            firstInPass = true;
            bottomPos = deltaBlockStart.y;
            middlePos = deltaBlockStart.y - PASS_BRACKET_OFFSET;
            topPos = middlePos - PASS_BRACKET_WIDTH / 2.f;

            const auto &passChange = eastl::get<sd::PassChange>(*stateDelta.pass);
            if (eastl::holds_alternative<sd::BeginRenderPass>(passChange.beginAction))
            {
              {
                const auto &beginRenderPass = eastl::get<sd::BeginRenderPass>(passChange.beginAction);

                const ImVec2 passInfoStart = ImVec2{leftPos - PASS_BLOCK_WIDTH, topPos};

                selectibleMaxSize = {PASS_BLOCK_WIDTH - 2.f * PASS_BLOCK_BORDER_PADDING.x, 0.f};
                ImGui::SetCursorScreenPos(passInfoStart + PASS_BLOCK_BORDER_PADDING);
                ImGui::BeginGroup();
                {
                  ImGui::TextUnformatted("Attachments:");
                  if (ImGui::IsItemHovered())
                    hoverState.tooltip.printf(0, "(mip,layer) name");
                  for (const auto &attachment : beginRenderPass.attachments)
                  {
                    label.printf(0, "  (%d,%d) %s##sd%u", attachment.mipLevel, attachment.layer,
                      intermediateGraph.resourceNames[attachment.res], static_cast<uint32_t>(nodeId));
                    if (ImGui::Selectable(label, focusState.resource == irResRepresent[attachment.res], 0, selectibleMaxSize))
                      focusState.set(irResRepresent[attachment.res]);
                    if (ImGui::IsItemHovered())
                    {
                      hoverState.node = NodeId::Invalid;
                      hoverState.resource = irResRepresent[attachment.res];
                    }

                    ImGui::TextUnformatted("    clear value:");
                    ImGui::SameLine();
                    if (eastl::holds_alternative<ResourceClearValue>(attachment.clearValue))
                    {
                      const auto &clearVal = eastl::get<ResourceClearValue>(attachment.clearValue);
                      ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
                      ImGui::TextUnformatted("[...]");
                      if (ImGui::IsItemHovered())
                        hoverState.tooltip.printf(0, clear_value_descr(clearVal));
                      ImGui::PopStyleColor();
                    }
                    else
                    {
                      ImVec2 selectSize = ImGui::GetItemRectSize();
                      selectSize = ImVec2{PASS_BLOCK_WIDTH - 2.f * PASS_BLOCK_BORDER_PADDING.x - selectSize.x, selectSize.y};

                      const auto irResIndex = eastl::get<intermediate::DynamicParameter>(attachment.clearValue).resource;
                      label.printf(0, "%s##%u", intermediateGraph.resourceNames[irResIndex], static_cast<uint32_t>(nodeId));
                      if (ImGui::Selectable(label, focusState.resource == irResRepresent[irResIndex], 0, selectSize))
                        focusState.set(irResRepresent[irResIndex]);
                    }
                  }

                  ImGui::TextUnformatted("Use VRS:");
                  ImGui::SameLine();
                  if (beginRenderPass.useVrs)
                  {
                    ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
                    ImGui::TextUnformatted("YES");
                  }
                  else
                  {
                    ImGui::PushStyleColor(ImGuiCol_Text, NEG_COLOR);
                    ImGui::TextUnformatted("NO");
                  }
                  ImGui::PopStyleColor();
                }
                ImGui::EndGroup();
                ImVec2 passInfoSize = ImGui::GetItemRectSize() + 2.f * NODE_BLOCK_BORDER_PADDING;
                passInfoSize = ImVec2{PASS_BLOCK_WIDTH, passInfoSize.y};

                draw_list->ChannelsSetCurrent(CanvasChannels::NODES);
                draw_list->AddRectFilled(passInfoStart, passInfoStart + passInfoSize, NODE_BASE_COLOR, 0.f, 0);
                draw_list->AddRect(passInfoStart, passInfoStart + passInfoSize, NODE_START_PASS_COLOR, 0.f, 0, PASS_BRACKET_WIDTH);
                draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);
              }

              passTagColor = NODE_START_PASS_COLOR;
              passLabel = NODE_START_PASS_TAG;
            }
            else if (eastl::holds_alternative<sd::LegacyBackbufferPass>(passChange.beginAction))
            {
              passTagColor = NODE_START_LEGACY_PASS_COLOR;
              passLabel = NODE_START_LEGACY_PASS_TAG;
            }
          }
          else
          {
            firstInPass = true;
            bottomPos = deltaBlockStart.y;
            middlePos = deltaBlockStart.y - PASS_BRACKET_OFFSET;
            topPos = middlePos - PASS_BRACKET_OFFSET;

            passTagColor = NODE_NEXT_SUBPASS_COLOR;
            passLabel = NODE_NEXT_SUBPASS_TAG;
          }

          if (firstInPass)
            drawBracket();
        }

        {
          bool lastInPass = false;

          const auto nextNodeIndex = intermediateGraph.nodes.getNextUsed(node.irIndex);
          if (nextNodeIndex < intermediateGraph.nodes.totalKeys() && stateDeltas[nextNodeIndex].pass &&
              eastl::holds_alternative<sd::PassChange>(*stateDeltas[nextNodeIndex].pass))
          {
            const auto &passChange = eastl::get<sd::PassChange>(*stateDeltas[nextNodeIndex].pass);
            if (eastl::holds_alternative<sd::FinishRenderPass>(passChange.endAction))
            {
              lastInPass = true;
              passTagColor = NODE_END_PASS_COLOR;
              passLabel = NODE_END_PASS_TAG;
            }
            else if (eastl::holds_alternative<sd::LegacyBackbufferPass>(passChange.endAction))
            {
              lastInPass = true;
              passTagColor = NODE_END_LEGACY_PASS_COLOR;
              passLabel = NODE_END_LEGACY_PASS_TAG;
            }
          }

          if (lastInPass)
          {
            topPos = nodeEnd.y;
            middlePos = nodeEnd.y + PASS_BRACKET_OFFSET;
            bottomPos = middlePos + PASS_BRACKET_WIDTH / 2.f;

            drawBracket();
          }
        }
      }
    }


    draw_list->ChannelsSetCurrent(CanvasChannels::NODES);

    if (focusState.node == nodeId)
      draw_list->AddRect(nodeStart - ImVec2{FOCUS_BORDER_WIDTH, FOCUS_BORDER_WIDTH},
        nodeEnd + ImVec2{FOCUS_BORDER_WIDTH, FOCUS_BORDER_WIDTH}, IM_COL32_WHITE, 2.f, 0, FOCUS_BORDER_WIDTH);
    draw_list->AddRectFilled(nodeStart, nodeEnd, NODE_BASE_COLOR, 2.f);

    draw_list->AddRect(nodeStart, nodeEnd, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(nameBlockStart, nameBlockStart + nameBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(passBlockStart, passBlockStart + passBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(ivbBlockStart, ivbBlockStart + ivbBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(bindBlockStart, bindBlockStart + bindBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(overrideBlockStart, overrideBlockStart + overrideBlockSize, NODE_BLOCK_COLOR, 0.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(vrsBlockStart, vrsBlockStart + vrsBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
    draw_list->AddRect(miscBlockStart, miscBlockStart + miscBlockSize, NODE_BLOCK_COLOR, 2.f, 0, NODE_BLOCK_BORDER_WIDTH);
  }
}

void Visualizer::drawResources(ImDrawList *draw_list, const CanvasLayout &layout)
{
  static String label;
  const ImRect &canvasView = canvas.ViewRect();
  ImVec2 resNameMaxSize = {RES_BLOCK_WIDTH - 2.f * RES_BLOCK_BORDER_PADDING.x - ImGui::CalcTextSize("x000k").x, 0.f};

  for (const auto [resId, rect] : layout.resources.enumerate())
  {
    const auto &resource = resources[resId];
    const auto &irResource = intermediateGraph.resources[resource.irIndex];
    const char *irResName = intermediateGraph.resourceNames[resource.irIndex].c_str();
    const auto irResType = irResource.getResType();

    const ImVec2 resourceStart = rect.offset;
    const ImVec2 resourceEnd = resourceStart + rect.size;
    const ImVec2 visibleResStart = ImVec2{
      clamp(resourceStart.x, canvasView.Min.x + RES_BLOCK_OFFSCREEN_PADDING, resourceEnd.x - (RES_BLOCK_WIDTH + RES_BORDER_PADDING.x)),
      resourceStart.y};

    const ImVec2 contentsStart = visibleResStart + RES_BORDER_PADDING;
    ImVec2 contentsEnd = contentsStart;

    ImU32 typeFillColor = RES_BASE_COLOR;
    ImU32 typeTagColor = RES_BASE_COLOR;
    const char *typeLabel = RES_INV_TYPE_TAG;
    switch (irResType)
    {
      case ResourceType::Texture:
        typeFillColor = RES_TEX_TYPE_COLOR;
        typeTagColor = RES_TEX_TAG_COLOR;
        typeLabel = RES_TEXTURE_TAG;
        break;
      case ResourceType::Buffer:
        typeFillColor = RES_BUF_TYPE_COLOR;
        typeTagColor = RES_BUF_TAG_COLOR;
        typeLabel = RES_BUFFER_TAG;
        break;
      case ResourceType::Blob:
        typeFillColor = RES_BLOB_TYPE_COLOR;
        typeTagColor = RES_BLOB_TAG_COLOR;
        typeLabel = RES_BLOB_TAG;
        break;
      default: break;
    }

    ImU32 statusFillColor = RES_BASE_COLOR;
    ImU32 statusTagColor = RES_BASE_COLOR;
    const char *statusLabel = RES_UNKNOWN_STATUS_TAG;
    if (irResource.isScheduled())
    {
      statusFillColor = RES_SHC_TYPE_COLOR;
      statusTagColor = RES_SHC_TAG_COLOR;
      statusLabel = RES_SHCEDULE_TAG;
    }
    else if (irResource.isExternal())
    {
      statusFillColor = RES_EXT_TYPE_COLOR;
      statusTagColor = RES_EXT_TAG_COLOR;
      statusLabel = RES_EXTERNAL_TAG;
    }
    else if (irResource.isDriverDeferredTexture())
    {
      statusFillColor = RES_DEF_TYPE_COLOR;
      statusTagColor = RES_DEF_TAG_COLOR;
      statusLabel = RES_DEFERRED_TAG;
    }

    const ImVec2 typeTagSize = ImGui::CalcTextSize(typeLabel) + 2.f * TAG_BORDER_PADDING;
    const ImVec2 typeTagStart = ImVec2{visibleResStart.x - TAG_BORDER_PADDING.x, visibleResStart.y - typeTagSize.y / 2.f};
    const ImVec2 typeTagEnd = typeTagStart + typeTagSize;

    const ImVec2 statusTagSize = ImGui::CalcTextSize(statusLabel) + 2.f * TAG_BORDER_PADDING;
    const ImVec2 statusTagStart = ImVec2{visibleResStart.x - TAG_BORDER_PADDING.x, resourceEnd.y - statusTagSize.y / 2.f};
    const ImVec2 statusTagEnd = statusTagStart + statusTagSize;


    draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);
    {
      ImGui::SetCursorScreenPos(contentsStart + RES_BLOCK_BORDER_PADDING);
      ImGui::BeginGroup();
      {
        if (ImGui::Button(irResName, resNameMaxSize) && !irResource.frontendResources.empty())
        {
          popupResource = resId;
          draw_list->ChannelsSetCurrent(CanvasChannels::SUSPEND);
          canvas.Suspend();
          ImGui::OpenPopup(RES_FRONTEND_POPUP);
          canvas.Resume();
          draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);
        }
        if (ImGui::IsItemHovered())
        {
          if (!irResource.frontendResources.empty())
            hoverState.tooltip.printf(0, "see frontend resources...\n");
          else
            hoverState.tooltip.printf(0, "no frontend resources\n");
        }

        ImVec2 subBlockStart =
          contentsStart + RES_BLOCK_BORDER_PADDING + ImVec2{0.f, ImGui::GetItemRectSize().y + RES_BLOCK_BORDER_PADDING.y};

        ImGui::SameLine();
        ImGui::Text("x%d", irResource.multiplexingIndex);
        if (ImGui::IsItemHovered())
          hoverState.tooltip.printf(0, "multiplex iteration");


        struct SubBlockHandler
        {
          ImDrawList *drawList;
          ImVec2 &startPos;

          SubBlockHandler(ImDrawList *draw_list, ImVec2 &start) : drawList(draw_list), startPos(start)
          {
            ImGui::SetCursorScreenPos(startPos);
            ImGui::BeginGroup();
          }

          ~SubBlockHandler()
          {
            ImGui::EndGroup();
            const ImVec2 blockSize = ImGui::GetItemRectSize();
            drawList->AddRect(startPos - RES_BLOCK_BORDER_PADDING / 2.f, startPos + blockSize + RES_BLOCK_BORDER_PADDING / 2.f,
              RES_BLOCK_COLOR, 2.f, 0, RES_BLOCK_BORDER_WIDTH);
            startPos += ImVec2{blockSize.x + RES_BLOCK_BORDER_PADDING.x, 0.f};
          }
        };

#define SUBBLOCK_SCOPED auto sbh = SubBlockHandler(draw_list, subBlockStart);

        if (irResource.isScheduled())
        {
          const auto &scheduledRes = irResource.asScheduled();

          if (irResType == ResourceType::Texture)
          {
            const auto &resDescr = eastl::get<ResourceDescription>(scheduledRes.description);

            {
              SUBBLOCK_SCOPED

              format_pretty(draw_list, resDescr.asBasicRes.cFlags & TEXFMT_MASK);
              ImGui::Text("%s - %s", texture_type_name(resDescr.type),
                (resDescr.asBasicRes.cFlags & TEXCF_RTARGET) ? "RTARGET" : "UAV");
              {
                if (resDescr.type == D3DResourceType::TEX)
                  ImGui::Text("%dx%d", resDescr.asTexRes.width, resDescr.asTexRes.height);
                else if (resDescr.type == D3DResourceType::VOLTEX)
                  ImGui::Text("%dx%dx%d", resDescr.asVolTexRes.width, resDescr.asVolTexRes.height, resDescr.asVolTexRes.depth);
                else if (resDescr.type == D3DResourceType::ARRTEX)
                  ImGui::Text("%dx%d [%d]", resDescr.asArrayTexRes.width, resDescr.asArrayTexRes.height,
                    resDescr.asArrayTexRes.arrayLayers);
                else if (resDescr.type == D3DResourceType::CUBETEX)
                  ImGui::Text("extent: %d", resDescr.asCubeTexRes.extent);
                else if (resDescr.type == D3DResourceType::CUBEARRTEX)
                  ImGui::Text("extent: %d [%d]", resDescr.asArrayCubeTexRes.extent, resDescr.asArrayCubeTexRes.cubes);
              }
              {
                ImGui::Text("mips: %d%s, flags:", resDescr.asBasicTexRes.mipLevels, scheduledRes.autoMipCount ? "(auto)" : "");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
                ImGui::TextUnformatted("[...]");
                if (ImGui::IsItemHovered())
                  hoverState.tooltip.printf(0, texture_flags_descr(resDescr.asBasicRes.cFlags));
                ImGui::PopStyleColor();
              }
            }

            if (scheduledRes.resolutionType)
            {
              SUBBLOCK_SCOPED

              const auto autoResData = registry.autoResTypes[scheduledRes.resolutionType->id];
              ImGui::TextUnformatted(auto_res_data_descr(autoResData, scheduledRes.resolutionType->multiplier));
              if (ImGui::IsItemHovered())
                hoverState.tooltip.printf(0, "Static\nDynamic\nLast applied dynamic\nMultiplier, Countdown");
            }
          }
          else if (irResType == ResourceType::Buffer)
          {
            SUBBLOCK_SCOPED

            const auto &bufferDescr = eastl::get<ResourceDescription>(scheduledRes.description).asBufferRes;

            ImGui::Text("%d byte x %d", bufferDescr.elementSizeInBytes, bufferDescr.elementCount);
            ImGui::Text("size: %d byte", bufferDescr.elementSizeInBytes * bufferDescr.elementCount);
            ImGui::Text("view: %u", bufferDescr.viewFormat);
            ImGui::TextUnformatted("flags:");
            ImGui::SameLine();
            {
              ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
              ImGui::TextUnformatted("[...]");
              if (ImGui::IsItemHovered())
                hoverState.tooltip.printf(0, buffer_flags_descr(bufferDescr.cFlags));
              ImGui::PopStyleColor();
            }
          }
          else if (irResType == ResourceType::Blob)
          {
            SUBBLOCK_SCOPED

            const auto &blobDescr = eastl::get<intermediate::BlobDescription>(scheduledRes.description);
            ImGui::Text("size: %zu byte", blobDescr.size);
            ImGui::Text("align: %zu byte", blobDescr.alignment);
          }


          {
            SUBBLOCK_SCOPED

            if (irResType == ResourceType::Texture || irResType == ResourceType::Buffer)
              ImGui::Text("activation: %s",
                activation_action_name(eastl::get<ResourceDescription>(scheduledRes.description).asBasicRes.activation));
            ImGui::Text("clear stage: %s", clear_stage_name(scheduledRes.clearStage));
            if (scheduledRes.clearStage != intermediate::ClearStage::None)
            {
              ImGui::Text("clear flags: %s", res_clear_flag_name(scheduledRes.clearFlags));
              ImGui::TextUnformatted("clear value:");
              ImVec2 selectSize = ImGui::GetItemRectSize();
              ImGui::SameLine();
              if (eastl::holds_alternative<ResourceClearValue>(scheduledRes.clearValue))
              {
                const auto &clearVal = eastl::get<ResourceClearValue>(scheduledRes.clearValue);
                ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
                ImGui::TextUnformatted("[...]");
                if (ImGui::IsItemHovered())
                  hoverState.tooltip.printf(0, clear_value_descr(clearVal));
                ImGui::PopStyleColor();
              }
              else
              {
                const auto irResIndex = eastl::get<intermediate::DynamicParameter>(scheduledRes.clearValue).resource;
                label.printf(0, "%s##%u", intermediateGraph.resourceNames[irResIndex], static_cast<uint32_t>(resId));
                if (ImGui::Selectable(label, focusState.resource == irResRepresent[irResIndex], 0, selectSize))
                  focusState.set(irResRepresent[irResIndex]);
              }
            }
          }

          if (scheduledRes.history != History::No)
          {
            const char *historyLabel =
              scheduledRes.history == History::ClearZeroOnFirstFrame ? RES_HISTORY_CLEAR_TAG : RES_HISTORY_DISCARD_TAG;
            const ImVec2 historyTagSize = ImGui::CalcTextSize(historyLabel) + 2.f * TAG_BORDER_PADDING;
            const ImVec2 historyTagStart = ImVec2{typeTagEnd.x, typeTagStart.y};
            const ImVec2 historyTagEnd = historyTagStart + historyTagSize;

            draw_list->AddRectFilled(historyTagStart, historyTagEnd, RES_HIS_TAG_COLOR, historyTagSize.y / 2.f);
            ImGui::SetCursorScreenPos(historyTagStart + TAG_BORDER_PADDING);
            ImGui::TextUnformatted(historyLabel);
          }
        }
        else if (irResource.isExternal())
        {
          SUBBLOCK_SCOPED

          if (irResType == ResourceType::Texture)
          {
            const auto &extTexInfo = eastl::get<TextureInfo>(irResource.asExternal().info);

            format_pretty(draw_list, extTexInfo.cflg & TEXFMT_MASK & TEXFMT_MASK);
            ImGui::Text("%s - %s", texture_type_name(extTexInfo.type), (extTexInfo.cflg & TEXCF_RTARGET) ? "RTARGET" : "UAV");
            if (extTexInfo.type == D3DResourceType::VOLTEX)
              ImGui::Text("%dx%dx%d", extTexInfo.w, extTexInfo.h, extTexInfo.d);
            else
              ImGui::Text("%dx%d", extTexInfo.w, extTexInfo.h);

            if (extTexInfo.type == D3DResourceType::ARRTEX)
            {
              ImGui::SameLine();
              ImGui::Text("[%d]", extTexInfo.a);
            }
            else if (extTexInfo.type == D3DResourceType::CUBEARRTEX)
            {
              ImGui::SameLine();
              ImGui::Text("[6 * %d]", extTexInfo.a / 6);
            }

            ImGui::Text("mips: %d, flags:", extTexInfo.mipLevels);
            ImGui::SameLine();
            {
              ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
              ImGui::TextUnformatted("[...]");
              if (ImGui::IsItemHovered())
                hoverState.tooltip.printf(0, texture_flags_descr(extTexInfo.cflg));
              ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            {
              if (extTexInfo.isCommitted)
              {
                ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
                ImGui::TextUnformatted("commited");
              }
              else
              {
                ImGui::PushStyleColor(ImGuiCol_Text, NEG_COLOR);
                ImGui::TextUnformatted("NOT commited");
              }
              ImGui::PopStyleColor();
            }
          }
          else if (irResType == ResourceType::Buffer)
          {
            const auto extBufInfo = eastl::get<intermediate::BufferInfo>(irResource.asExternal().info);

            ImGui::TextUnformatted("Creation flags:");
            ImGui::SameLine();
            {
              ImGui::PushStyleColor(ImGuiCol_Text, POS_COLOR);
              ImGui::TextUnformatted("[...]");
              if (ImGui::IsItemHovered())
                hoverState.tooltip.printf(0, buffer_flags_descr(extBufInfo.flags));
              ImGui::PopStyleColor();
            }
          }
        }
#undef SUBBLOCK_SCOPED
      }
      ImGui::EndGroup();
      const ImVec2 resBlockSize = ImGui::GetItemRectSize() + 2.f * RES_BLOCK_BORDER_PADDING;
      contentsEnd += resBlockSize;


      draw_list->AddRectFilled(typeTagStart, typeTagEnd, typeTagColor, typeTagSize.y / 2.f);
      ImGui::SetCursorScreenPos(typeTagStart + TAG_BORDER_PADDING);
      ImGui::TextUnformatted(typeLabel);

      draw_list->AddRectFilled(statusTagStart, statusTagEnd, statusTagColor, statusTagSize.y / 2.f);
      ImGui::SetCursorScreenPos(statusTagStart + TAG_BORDER_PADDING);
      ImGui::TextUnformatted(statusLabel);
    }


    draw_list->ChannelsSetCurrent(CanvasChannels::RESOURCES);
    {
      if (focusState.resource == resId)
        draw_list->AddRect(resourceStart - ImVec2{FOCUS_BORDER_WIDTH, FOCUS_BORDER_WIDTH},
          resourceEnd + ImVec2{FOCUS_BORDER_WIDTH, FOCUS_BORDER_WIDTH}, IM_COL32_WHITE, 2.f, 0, FOCUS_BORDER_WIDTH);

      draw_list->AddRectFilled(resourceStart, resourceEnd, typeFillColor, 2.f);
      draw_list->AddRectFilled(ImVec2{resourceStart.x, resourceStart.y + 0.75f * (resourceEnd.y - resourceStart.y)}, resourceEnd,
        statusFillColor, 2.f);
      draw_list->AddRectFilled(contentsStart, contentsEnd, RES_BASE_COLOR, 2.f);
    }
  }
}

void Visualizer::drawOrderings(ImDrawList *draw_list, const CanvasLayout &layout)
{
  draw_list->ChannelsSetCurrent(CanvasChannels::ORDERINGS);

  for (const auto [orderingId, rect] : layout.orderings.enumerate())
  {
    const auto &ordering = orderings[orderingId];
    const auto [start, end] = rect;

    const bool highlight = focusState.nodeValid() && (focusState.node == ordering.from || focusState.node == ordering.to) ||
                           ordering.from != NodeId::Invalid && hoverState.node == ordering.from ||
                           ordering.to != NodeId::Invalid && hoverState.node == ordering.to;

    ImColor color = ORDERING_BASE_COLOR;
    if (!highlight)
      color = apply_alpha_coeff(color, 0.1f);

    draw_list->AddLine(start, end, color, ORDERING_WIDTH);
  }
}

void Visualizer::drawUsages(ImDrawList *draw_list, const CanvasLayout &layout)
{
  draw_list->ChannelsSetCurrent(CanvasChannels::USAGES);

  for (const auto [usageId, rect] : layout.usages.enumerate())
  {
    const auto &usage = usages[usageId];
    const auto [start, end] = rect;
    const bool highlight = focusState.nodeValid() && focusState.node == usage.node ||
                           focusState.resValid() && focusState.resource == usage.resource ||
                           hoverState.node != NodeId::Invalid && hoverState.node == usage.node ||
                           hoverState.resource != ResourceId::Invalid && hoverState.resource == usage.resource;

    ImColor color = USAGE_BASE_COLOR;
    if (!highlight)
      color = apply_alpha_coeff(color, 0.1f);

    draw_list->AddLine(start, end, color, USAGE_WIDTH);
    draw_list->AddCircleFilled(start, 1.5f * USAGE_WIDTH, color);
    draw_list->AddCircleFilled(end, 1.5f * USAGE_WIDTH, color);
  }
}


void Visualizer::checkHovering(const CanvasLayout &layout)
{
  const ImVec2 mouseGridPosition = ImGui::GetIO().MousePos;

  for (const auto [nodeId, rect] : layout.nodes.enumerate())
    if (ImRect{rect.offset, rect.offset + rect.size}.Contains(mouseGridPosition))
      hoverState.node = nodeId;

  for (const auto [resId, rect] : layout.resources.enumerate())
    if (ImRect{rect.offset, rect.offset + rect.size}.Contains(mouseGridPosition))
      hoverState.resource = resId;
}

void Visualizer::processInput()
{
  if (hoverState.window && hoverState.canvas)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
      canvasCamera.canvasOffset += ImGui::GetIO().MouseDelta;
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
    if (abs(ImGui::GetIO().MouseWheel) > 0.1 && !hoverState.searchBox)
    {
      ImGui::GetIO().MouseWheel > 0 ? canvasCamera.zoomIn() : canvasCamera.zoomOut();

      ImVec2 oldPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
      ImVec2 newPos = canvas.ToLocal(ImGui::GetIO().MousePos);

      canvasCamera.canvasOffset += canvas.FromLocalV(newPos - oldPos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
  }

  if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
  {
    if (hoverState.node != NodeId::Invalid)
      focusState.set(hoverState.node);
    else if (hoverState.resource != ResourceId::Invalid)
      focusState.set(hoverState.resource);
    else
      focusState.reset();
  }
}

void Visualizer::processPopup()
{
  if (ImGui::BeginPopup(RES_FRONTEND_POPUP))
  {
    static String label;

    const auto resource = resources[popupResource];
    const auto irResource = intermediateGraph.resources[resource.irIndex];
    for (const auto resNameId : irResource.frontendResources)
    {
      label.printf(0, "%s", registry.knownNames.getName(resNameId));
      if (ImGui::Selectable(label))
      {
        request_user_resource_focus(resNameId);
        imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true);
      }
    }

    ImGui::EndPopup();
  }

  if (!hoverState.tooltip.empty() && !hoverState.searchBox)
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(hoverState.tooltip);
    ImGui::EndTooltip();
  }
}


void Visualizer::updateVisualization()
{
  TIME_PROFILE(update_intermediate_visualization);

  updateNeeded = true;
  focusState.reset();

  if (!imgui_window_is_visible(nullptr, IMGUI_IRG_WIN_NAME) || imgui_window_is_collapsed(nullptr, IMGUI_IRG_WIN_NAME))
    return;

  updateNodes();
  updateResourses();
  updateConnections();

  performLayout();

  updateNeeded = false;
}

void Visualizer::updateNodes()
{
  TIME_PROFILE(update_nodes);

  const uint32_t irNodesCount = intermediateGraph.nodes.used();
  const uint32_t irNodesRange = intermediateGraph.nodes.totalKeys();

  nodes.clear();
  irNodeRepresent.clear();

  nodes.reserve(irNodesCount);
  irNodeRepresent.resize(irNodesRange, NodeId::Invalid);


  int currentPassNumber = -1;
  PassColor previousPassColor = UNKNOWN_PASS_COLOR;
  uint32_t curExecTime = 0;

  for (const auto [irNodeIndex, irNode] : intermediateGraph.nodes.enumerate())
    if (intermediateGraph.nodes.isMapped(irNodeIndex))
    {
      auto [newNodeId, newNode] = nodes.appendNew();
      irNodeRepresent[irNodeIndex] = newNodeId;
      newNode.irIndex = irNodeIndex;
      newNode.executionTime = curExecTime;

      if (previousPassColor != passColoring[irNodeIndex])
      {
        ++currentPassNumber;
        previousPassColor = passColoring[irNodeIndex];
      }
      newNode.renderPassNumber = currentPassNumber;

      ++curExecTime;
    }
}

void Visualizer::updateResourses()
{
  TIME_PROFILE(update_resources);

  const uint32_t irResCount = intermediateGraph.resources.used();
  const uint32_t irResRange = intermediateGraph.resources.totalKeys();

  resources.clear();
  irResRepresent.clear();

  resources.reserve(irResCount);
  irResRepresent.resize(irResRange, ResourceId::Invalid);


  for (const auto [irResourceIndex, irResource] : intermediateGraph.resources.enumerate())
    if (intermediateGraph.resources.isMapped(irResourceIndex))
    {
      auto [newResId, newResourse] = resources.appendNew();
      irResRepresent[irResourceIndex] = newResId;
      newResourse.irIndex = irResourceIndex;
    }
}

void Visualizer::updateConnections()
{
  TIME_PROFILE(update_connections);

  orderings.clear();
  usages.clear();

  for (auto [nodeId, node] : nodes.enumerate())
  {
    for (const auto prevIrNodeIndex : intermediateGraph.nodes[node.irIndex].predecessors)
    {
      const auto prevNodeId = irNodeRepresent[prevIrNodeIndex];
      auto &prevNode = nodes[prevNodeId];

      const auto newOrderingId = orderings.appendNew(Ordering{prevNodeId, nodeId}).first;

      node.previous.push_back(newOrderingId);
      prevNode.following.push_back(newOrderingId);
    }

    for (const auto &request : intermediateGraph.nodes[node.irIndex].resourceRequests)
    {
      const auto requestedResId = irResRepresent[request.resource];
      auto &requestedRes = resources[requestedResId];

      const auto newUsageId = usages.appendNew(Usage{nodeId, requestedResId, request}).first;

      node.resourceUsages.push_back(newUsageId);
      requestedRes.usages.push_back(newUsageId);
    }
  }

  for (auto &node : nodes)
  {
    eastl::sort(node.previous.begin(), node.previous.end(), [&](const OrderingId a, const OrderingId b) {
      return nodes[orderings[a].from].executionTime < nodes[orderings[b].from].executionTime;
    });

    eastl::sort(node.following.begin(), node.following.end(), [&](const OrderingId a, const OrderingId b) {
      return nodes[orderings[a].to].executionTime < nodes[orderings[b].to].executionTime;
    });
  }

  for (auto &resource : resources)
  {
    eastl::sort(resource.usages.begin(), resource.usages.end(),
      [&](const UsageId a, const UsageId b) { return nodes[usages[a].node].executionTime < nodes[usages[b].node].executionTime; });

    resource.firstUser = usages[resource.usages.front()].node;
    resource.lastUser = usages[resource.usages.back()].node;
  }
}


void Visualizer::performLayout()
{
  TIME_PROFILE(layout);

  generalLayout = {};

  placeNodesByPasses(generalLayout);
  placeResourcesByLines(generalLayout);
  placeOrderings(generalLayout);
  placeUsages(generalLayout);
}

void Visualizer::placeNodesByPasses(CanvasLayout &layout)
{
  TIME_PROFILE(place_nodes);

  const uint32_t nodesCount = nodes.size();
  if (nodesCount == 0)
    return;


  layout.nodes.resize(nodesCount, {});

  const auto &style = ImGui::GetStyle();
  const float lineHeight = ImGui::GetFrameHeight();
  for (auto [nodeId, node] : nodes.enumerate())
  {
    const auto &state = intermediateGraph.nodeStates[node.irIndex];

    ImVec2 nameBlockSize = ImVec2{ImGui::CalcTextSize(intermediateGraph.nodeNames[node.irIndex].c_str()).x, 2.f * lineHeight} +
                           2.f * NODE_BLOCK_BORDER_PADDING;

    ImVec2 rpBlockSize = ImVec2{NODE_BLOCK_WIDTH, lineHeight} + 2.f * NODE_BLOCK_BORDER_PADDING;
    if (state.pass)
    {
      const auto rp = *state.pass;
      if (!rp.colorAttachments.empty())
        rpBlockSize.y += (1 + rp.colorAttachments.size()) * lineHeight;

      if (rp.depthAttachment)
        rpBlockSize.y += 2.f * lineHeight;

      if (rp.vrsRateAttachment)
        rpBlockSize.y += 2.f * lineHeight;

      if (!rp.resolves.empty())
        rpBlockSize.y += (1 + rp.resolves.size()) * lineHeight;
    }

    ImVec2 ivbBlockSize = ImVec2{NODE_BLOCK_WIDTH, lineHeight} + 2.f * NODE_BLOCK_BORDER_PADDING;
    {
      bool hasIBuf = bool(state.indexSource);
      bool hasVBuf = false;
      for (const auto &vtxBuf : state.vertexSources)
        hasVBuf |= bool(vtxBuf);

      if (hasIBuf)
        ivbBlockSize.y += 2.f * lineHeight;

      if (hasVBuf)
        ivbBlockSize.y += (1 + state.vertexSources.size()) * lineHeight;
    }

    ImVec2 bindBlockSize = ImVec2{NODE_BLOCK_WIDTH, (2 + state.bindings.size()) * lineHeight} + 2.f * NODE_BLOCK_BORDER_PADDING;

    ImVec2 overrideBlockSize = ImVec2{NODE_BLOCK_WIDTH, lineHeight} + 2.f * NODE_BLOCK_BORDER_PADDING;
    if (state.shaderOverrides)
      overrideBlockSize.y += ImGui::CalcTextSize(override_state_descr(*state.shaderOverrides)).y + 2.f * style.FramePadding.y;


    ImVec2 vrsBlockSize = ImGui::CalcTextSize(NODE_VRS_MAX_LABEL) + 2.f * NODE_BLOCK_BORDER_PADDING;

    ImVec2 miscBlockSize = ImVec2{NODE_BLOCK_WIDTH, 3.f * lineHeight} + 2.f * NODE_BLOCK_BORDER_PADDING;


    const float nodeContentsSizeX =
      max(rpBlockSize.x, ivbBlockSize.x) + bindBlockSize.x + max(overrideBlockSize.x, max(vrsBlockSize.x, miscBlockSize.x));
    const float nodeContentsSizeY = nameBlockSize.y + max(rpBlockSize.y + ivbBlockSize.y,
                                                        max(bindBlockSize.y, overrideBlockSize.y + vrsBlockSize.y + miscBlockSize.y));

    const float orderingsHeight = max(2.f * ORDERINGS_MARGIN + 3.f * ORDERING_WIDTH * node.previous.size(),
      2.f * ORDERINGS_MARGIN + 3.f * ORDERING_WIDTH * node.following.size());

    layout.nodes[nodeId].size =
      ImVec2{
        max(nodeContentsSizeX, NODE_MIN_X_SIZE),
        max(max(nodeContentsSizeY, orderingsHeight), NODE_MIN_Y_SIZE),
      } +
      2.f * NODE_BORDER_PADDING;
  }

  const uint32_t passesCount = nodes.back().renderPassNumber + 1;
  dag::Vector<float, framemem_allocator> passesWidths(passesCount, 0.f);
  for (const auto &node : nodes)
    passesWidths[node.renderPassNumber] += 2.f * USAGES_MARGIN + 3.f * USAGE_WIDTH * node.resourceUsages.size();
  for (auto [nodeId, node] : nodes.enumerate())
    passesWidths[node.renderPassNumber] = max(passesWidths[node.renderPassNumber], layout.nodes[nodeId].size.x);


  float horOffset = 0.f;
  float vertOffset = 0.f;
  uint32_t currentPassNumber = nodes.front().renderPassNumber;
  for (auto [nodeId, node] : nodes.enumerate())
  {
    if (node.renderPassNumber != currentPassNumber)
    {
      horOffset += passesWidths[currentPassNumber] + NODE_BETWEEN_PASS_MARGIN;
      vertOffset = 0.f;
      currentPassNumber = node.renderPassNumber;
    }

    const float nodeWidth = passesWidths[node.renderPassNumber];
    const float nodeHeight = layout.nodes[nodeId].size.y;

    {
      const float betweenLines = 2.f * STATE_DELTA_BLOCK_BORDER_PADDING.y + STATE_DELTA_BLOCK_MARGIN;
      float deltaBlocksHeight = 0.f;

      const auto &stateDelta = stateDeltas[node.irIndex];

      if (stateDelta.asyncPipelines)
        deltaBlocksHeight += lineHeight + betweenLines;
      if (stateDelta.wire)
        deltaBlocksHeight += lineHeight + betweenLines;

      if (stateDelta.vrs)
        deltaBlocksHeight += lineHeight + ImGui::CalcTextSize(NODE_VRS_MAX_LABEL).y + betweenLines;

      if (stateDelta.shaderOverrides)
        deltaBlocksHeight += lineHeight +
                             ImGui::CalcTextSize(override_state_descr(shaders::overrides::get(*stateDelta.shaderOverrides))).y +
                             betweenLines;

      if (stateDelta.indexSource)
        deltaBlocksHeight += 2.f * lineHeight + betweenLines;

      uint32_t vtxBufDeltas = 0;
      for (const auto optBuf : stateDelta.vertexSources)
        vtxBufDeltas += bool(optBuf) ? 1 : 0;
      if (vtxBufDeltas > 0)
        deltaBlocksHeight += lineHeight * (1 + vtxBufDeltas) + betweenLines;

      if (!stateDelta.bindings.empty())
        deltaBlocksHeight += lineHeight * (1 + stateDelta.bindings.size()) + betweenLines;

      if (
        stateDelta.shaderBlockLayers.objectLayer || stateDelta.shaderBlockLayers.frameLayer || stateDelta.shaderBlockLayers.sceneLayer)
        deltaBlocksHeight += lineHeight + betweenLines;

      if (vertOffset != 0.f)
        vertOffset += deltaBlocksHeight + NODE_IN_PASS_MARGIN;
      else
        vertOffset += deltaBlocksHeight;
    }

    layout.nodes[nodeId] = Rectangle{ImVec2{horOffset, vertOffset}, ImVec2{nodeWidth, nodeHeight}};

    vertOffset += nodeHeight;
  }
}

void Visualizer::placeResourcesByLines(CanvasLayout &layout)
{
  TIME_PROFILE(place_resources);

  const uint32_t resCount = resources.size();
  if (resCount == 0)
    return;


  layout.resources.resize(resCount, {});

  /*
  we represent each resource as a segment on an integer line, where the ends are the first and last use
  (the location in memory is not taken into account yet, only in time)
  then we sort these segments and begin to lay them out along lines,
  choosing for each segment the smallest line in order where it can be placed
  so we get the layout of resources along lines without intersections and with the smallest number of lines
  */

  IdIndexedMapping<ResourceId, uint32_t, framemem_allocator> resourcesLines(resCount);
  {
    struct Lifetime
    {
      uint32_t startPass;
      uint32_t endPass;
      ResourceId resId;
    };

    dag::Vector<Lifetime, framemem_allocator> times;
    times.reserve(resCount);
    for (auto [resId, resource] : resources.enumerate())
      times.push_back(Lifetime{nodes[resource.firstUser].renderPassNumber, nodes[resource.lastUser].renderPassNumber, resId});

    eastl::sort(times.begin(), times.end(), [](const Lifetime &a, const Lifetime &b) {
      return a.startPass == b.startPass ? a.endPass < b.endPass : a.startPass < b.startPass;
    });

    dag::Vector<uint32_t, framemem_allocator> lastTimeInLine;
    for (const Lifetime &time : times)
    {
      uint32_t lineToPlace = 0;

      while (lineToPlace < lastTimeInLine.size() && lastTimeInLine[lineToPlace] >= time.startPass)
        ++lineToPlace;

      if (lineToPlace >= lastTimeInLine.size())
        lastTimeInLine.push_back(0);

      resourcesLines[time.resId] = lineToPlace;
      lastTimeInLine[lineToPlace] = time.endPass;
    }
  }

  for (auto [resId, resource] : resources.enumerate())
  {
    const auto &firstNodeRect = layout.nodes[resource.firstUser];
    const auto &lastNodeRect = layout.nodes[resource.lastUser];

    const float leftBorder = firstNodeRect.offset.x;
    const float rightBorder = lastNodeRect.offset.x + lastNodeRect.size.x;
    const float bottomBorder = -(RES_LINE_OFFSET + resourcesLines[resId] * (RES_LINE_Y_SIZE + RES_LINE_MARGIN));

    layout.resources[resId] =
      Rectangle{ImVec2{leftBorder, bottomBorder - RES_LINE_Y_SIZE}, ImVec2{rightBorder - leftBorder, RES_LINE_Y_SIZE}};
  }

  for (auto &node : nodes)
    eastl::sort(node.resourceUsages.begin(), node.resourceUsages.end(),
      [&](const UsageId a, const UsageId b) { return resourcesLines[usages[a].resource] < resourcesLines[usages[b].resource]; });
}

void Visualizer::placeOrderings(CanvasLayout &layout)
{
  TIME_PROFILE(place_orderings);

  const uint32_t orderingsCount = orderings.size();
  if (orderingsCount == 0)
    return;


  layout.orderings.resize(orderingsCount, {});

  for (auto [orderingId, ordering] : orderings.enumerate())
  {
    const auto &fromNode = nodes[ordering.from];
    const auto &toNode = nodes[ordering.to];

    const float startOrderingAreaHeight = 3.f * ORDERING_WIDTH * fromNode.following.size();
    const float endOrderingAreaHeight = 3.f * ORDERING_WIDTH * toNode.previous.size();
    const uint32_t startOrderingPosition =
      eastl::find(fromNode.following.begin(), fromNode.following.end(), orderingId) - fromNode.following.begin();
    const uint32_t endOrderingPosition =
      eastl::find(toNode.previous.begin(), toNode.previous.end(), orderingId) - toNode.previous.begin();

    const auto &fromNodeRect = layout.nodes[ordering.from];
    const auto &toNodeRect = layout.nodes[ordering.to];

    const ImVec2 fromNodeCenter = fromNodeRect.offset + ImVec2{fromNodeRect.size.x, fromNodeRect.size.y / 2.f};
    const ImVec2 toNodeCenter = toNodeRect.offset + ImVec2{0.f, toNodeRect.size.y / 2.f};

    const ImVec2 fromAnchor =
      fromNodeCenter + ImVec2{0.f, -startOrderingAreaHeight / 2.f + (1.5f + 3.f * startOrderingPosition) * ORDERING_WIDTH};
    const ImVec2 toAnchor =
      toNodeCenter + ImVec2{0.f, -endOrderingAreaHeight / 2.f + (1.5f + 3.f * endOrderingPosition) * ORDERING_WIDTH};

    layout.orderings[orderingId] = Rectangle{fromAnchor, toAnchor};
  }
}

void Visualizer::placeUsages(CanvasLayout &layout)
{
  TIME_PROFILE(place_usages);

  const uint32_t usagesCount = usages.size();
  if (usagesCount == 0)
    return;


  layout.usages.resize(usagesCount, {});

  const uint32_t passesCount = nodes.back().renderPassNumber + 1;
  dag::Vector<dag::Vector<NodeId, framemem_allocator>, framemem_allocator> nodesByPasses(passesCount);
  for (auto [nodeId, node] : dag::ReverseView{nodes.enumerate()})
    nodesByPasses[node.renderPassNumber].push_back(nodeId);


  float horOffset = 0.f;
  for (const auto &column : nodesByPasses)
  {
    horOffset = 0.f;

    for (const auto nodeId : dag::ReverseView{column})
    {
      horOffset += USAGES_MARGIN;
      for (const auto usageId : nodes[nodeId].resourceUsages)
      {
        horOffset += 1.5f * USAGE_WIDTH;

        const auto &usage = usages[usageId];
        const auto &fromNodeRect = layout.nodes[usage.node];
        const auto &toResourceRect = layout.resources[usage.resource];

        const ImVec2 fromAnchor = fromNodeRect.offset + ImVec2{horOffset, 0.f};
        const ImVec2 toAnchor = ImVec2{fromAnchor.x, toResourceRect.offset.y + toResourceRect.size.y};

        layout.usages[usageId] = Rectangle{fromAnchor, toAnchor};

        horOffset += 1.5f * USAGE_WIDTH;
      }
      horOffset += USAGES_MARGIN;
    }
  }
}

} // namespace visualization::irgraph

} // namespace dafg