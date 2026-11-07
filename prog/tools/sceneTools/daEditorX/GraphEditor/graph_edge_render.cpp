// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_edge_render.h"

#include "graph_canvas_cull.h"
#include "graph_document.h"
#include "graph_edge_reconnect.h"
#include "graph_edit_types.h"
#include "graph_pin_colors.h"
#include "graph_pin_jump_menu.h"
#include "graph_validation.h"

#include <graphEditor/graph_dead_paths.h>
#include <libTools/util/hdpiUtil.h>

#include <EASTL/algorithm.h>
#include <EASTL/vector.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

#include <math.h>


namespace ne = ax::NodeEditor;

namespace
{
constexpr int EDGE_THICKNESS = 2;
constexpr int EDGE_THICKNESS_ACTIVE = 4;
constexpr int EDGE_MUTED_DASH = 4; // dash and gap length, passed to ne::SetNextLinkDashSize
constexpr float EDGE_MIN_COVERAGE = 0.2f;
} // namespace

GraphEdgeFrame::GraphEdgeFrame(const GraphData &graph_data, const GraphCanvasCull &canvas_cull, const DeadPaths &dead_paths,
  const GraphSelection &graph_selection, const GraphPinJumpMenu &pin_jump_menu) :
  graph(graph_data), canvasCull(canvas_cull), deadPaths(dead_paths), selection(graph_selection), pinJumpMenu(pin_jump_menu)
{
  thickness = static_cast<float>(hdpi::_pxS(EDGE_THICKNESS));
  thicknessActive = static_cast<float>(hdpi::_pxS(EDGE_THICKNESS_ACTIVE));
  mutedDash = static_cast<float>(hdpi::_pxS(EDGE_MUTED_DASH));
}

PinType pin_type_at(const GraphData &graph, int node_index, int pin_index)
{
  if (node_index < 0 || node_index >= static_cast<int>(graph.nodes.size()) || pin_index < 0 ||
      pin_index >= static_cast<int>(graph.nodes[node_index].pins.size()))
  {
    return PinType::Unknown;
  }
  return graph.nodes[node_index].pins[pin_index].type;
}

void draw_graph_edges(const GraphEdgeFrame &frame)
{
  const ne::LinkId hoveredLink = ne::GetHoveredLink();
  const int jumpMenuEdgeId = frame.pinJumpMenu.highlightedEdgeId();
  const float invZoom = ne::GetCurrentZoom(); // canvas units per screen pixel
  const ImVec4 canvasBg = ne::GetStyle().Colors[ne::StyleColor_Bg];

  for (int ei = 0; ei < static_cast<int>(frame.graph.edges.size()); ++ei)
  {
    const GraphData::Edge &e = frame.graph.edges[ei];
    // Skip a link whose endpoint node was culled this frame: ne::Link would no-op anyway (the pin
    // isn't live) and this avoids the per-edge FindPin lookups. Endpoints kept "reduced" still
    // declared their pins, so links reaching into the visible area survive.
    const int ia = frame.canvasCull.indexOf(e.elemA);
    const int ib = frame.canvasCull.indexOf(e.elemB);
    if ((ia >= 0 && !frame.canvasCull.needed(ia)) || (ib >= 0 && !frame.canvasCull.needed(ib)))
    {
      continue;
    }

    // Color follows the data the edge carries, so it reads as one piece with the pin dots it joins.
    // elemA/pinA is the source (out) pin by the stored-edge convention.
    PinType type = pin_type_at(frame.graph, ia, e.pinA);
    if (type == PinType::Unknown)
    {
      type = pin_type_at(frame.graph, ib, e.pinB);
    }

    const bool active = e.id == jumpMenuEdgeId || hoveredLink == ne::LinkId(make_link_id(e.id)) ||
                        eastl::binary_search(frame.selection.links.begin(), frame.selection.links.end(), e.id) ||
                        eastl::binary_search(frame.selection.nodes.begin(), frame.selection.nodes.end(), e.elemA) ||
                        eastl::binary_search(frame.selection.nodes.begin(), frame.selection.nodes.end(), e.elemB);
    const float thickness = active ? frame.thicknessActive : frame.thickness;

    const bool dead = frame.deadPaths.isDeadEdge(ei);
    const ImVec4 edgeColor = ImColor(dead ? dead_color_for_type(type) : pin_color_for_type(type));
    // ImGui draws any line under one screen pixel as an opaque 1 px line, so mix it toward the canvas by its
    // coverage, and by its own alpha too: drawn translucent, overlapping edges would add up.
    ImVec4 color = ImLerp(canvasBg, edgeColor, edgeColor.w * eastl::clamp(thickness / invZoom, EDGE_MIN_COVERAGE, 1.0f));
    color.w = 1.0f;

    if (e.muted)
    {
      ne::SetNextLinkDashSize(frame.mutedDash);
    }
    ne::Link(ne::LinkId(make_link_id(e.id)), ne::PinId(make_pin_id(e.elemA, e.pinA)), ne::PinId(make_pin_id(e.elemB, e.pinB)), color,
      thickness);
  }
}

void handle_link_create(const GraphEdgeFrame &frame, GraphDocument &doc, const GraphEdgeReconnect &edge_reconnect,
  LinkDropOnCanvas &out_drop)
{
  out_drop.overEmptyCanvas = false;
  out_drop.createInFlight = false;

  // QueryNewLink hands us the pin where the user grabbed the drag and where they dropped it; the user
  // can grab either end first, so we re-orient to (output, input) before storing -- mirrors the JS
  // editor's convention that elemA/pinA is the source. The dangling drag has no target pin yet, so it
  // keeps the neutral color and only matches the edge weight; once a valid target is under the cursor,
  // AcceptNewItem repaints it in its type color.
  ne::BeginCreate(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), frame.thickness);
  {
    ne::PinId startId, endId;
    if (ne::QueryNewLink(&startId, &endId) && startId && endId)
    {
      out_drop.createInFlight = true;
      if (edge_reconnect.isActive())
      {
        // A "modify edge" reconnect (A) owns the pin interaction; swallow ne's own link creation so
        // the two don't both fire and leave a stray edge to whichever pin ne's drag started from.
        ne::RejectNewItem();
      }
      else
      {
        int aNode = -1, aPin = -1, bNode = -1, bPin = -1;
        decode_pin_id(startId.Get(), aNode, aPin);
        decode_pin_id(endId.Get(), bNode, bPin);

        // Re-orient so the stored edge always goes (Out -> In). Validator accepts either order
        // but persisted edges match the convention used elsewhere (load path, JS editor).
        const GraphData::Pin *const aEnd = bNode >= 0 ? find_pin(frame.graph, aNode, aPin) : nullptr;
        const bool aIsOut = aEnd && aEnd->role == PinRole::Out;
        const int srcNode = aIsOut ? aNode : bNode;
        const int srcPin = aIsOut ? aPin : bPin;
        const int dstNode = aIsOut ? bNode : aNode;
        const int dstPin = aIsOut ? bPin : aPin;

        if (validate_new_edge(frame.graph, srcNode, srcPin, dstNode, dstPin))
        {
          const PinType srcType = pin_type_at(frame.graph, frame.canvasCull.indexOf(srcNode), srcPin);
          if (ne::AcceptNewItem(ImColor(pin_color_for_type(srcType)), frame.thicknessActive))
          {
            // The validator's hideSameConnection pass treats an edge already on this pin pair as
            // replaced by the candidate, but nothing here ever removed it -- so the drag used to add
            // a second copy. Target the existing edge instead: reviving a muted one is what the
            // gesture means, and a live one already is what the user asked for. Stored orientation
            // is not guaranteed, so match both.
            const auto existing = eastl::find_if(frame.graph.edges.begin(), frame.graph.edges.end(), [&](const GraphData::Edge &e) {
              return (e.elemA == srcNode && e.pinA == srcPin && e.elemB == dstNode && e.pinB == dstPin) ||
                     (e.elemA == dstNode && e.pinA == dstPin && e.elemB == srcNode && e.pinB == srcPin);
            });
            if (existing != frame.graph.edges.end())
            {
              if (existing->muted)
              {
                doc.toggleEdgeMuted(existing->id);
              }
            }
            else
            {
              GraphData::Edge edge;
              edge.id = next_edge_id(frame.graph);
              edge.elemA = srcNode;
              edge.pinA = srcPin;
              edge.elemB = dstNode;
              edge.pinB = dstPin;
              doc.recordConnectEdge(eastl::move(edge));
            }
          }
        }
        else
        {
          ne::RejectNewItem(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), frame.thickness);
        }
      }
    }

    // A link dropped on empty canvas asks for a node there. ne only confirms it a frame after the
    // button comes up, and only if AcceptNewItem was called on the dragging frames too -- that call
    // is what promotes the action, so it cannot wait until this looks like a drop.
    ne::PinId dropPinId;
    if (ne::QueryNewNode(&dropPinId))
    {
      // A reroute (A) owns its own pin interaction. Declining without RejectNewItem, which leaves the
      // stage set: QueryNewNode would then fire every frame, while DragEnd retires it.
      if (!edge_reconnect.isActive() && dropPinId)
      {
        int dropNode = -1;
        int dropPin = -1;
        decode_pin_id(dropPinId.Get(), dropNode, dropPin);

        out_drop.overEmptyCanvas = true;
        out_drop.sourceNode = dropNode;
        out_drop.sourcePin = dropPin;
        const GraphData::Pin *const src = find_pin(frame.graph, dropNode, dropPin);
        out_drop.sourceIsOutput = src && src->role == PinRole::Out;
        // ne suspends inside QueryNewNode, so the mouse is back in screen space here. Release
        // included, but not the accepting frame -- the cursor has moved on by then.
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
          out_drop.dropCanvasPos = ne::ScreenToCanvas(ImGui::GetMousePos());
        }

        if (ne::AcceptNewItem())
        {
          out_drop.spawnRequested = true;
        }
      }
    }
  }
  ne::EndCreate();
}

void draw_dangling_link(ImDrawList *draw_list, const ImVec2 &from, const ImVec2 &to, bool from_is_output, uint32_t color,
  float thickness)
{
  const float dirX = from_is_output ? 1.0f : -1.0f;
  const float strength = fabsf(to.x - from.x) * 0.5f + 25.0f;
  const ImVec2 c1(from.x + dirX * strength, from.y);
  const ImVec2 c2(to.x - dirX * strength, to.y);
  draw_list->AddBezierCubic(from, c1, c2, to, color, thickness);
}

void handle_deleted_links(GraphDocument &doc)
{
  ne::LinkId deletedLinkId;
  eastl::vector<int> deletedEdgeIds;
  while (ne::QueryDeletedLink(&deletedLinkId))
  {
    if (ne::AcceptDeletedItem())
    {
      deletedEdgeIds.push_back(decode_link_id(deletedLinkId.Get()));
    }
  }
  // One undo step for the links removed in this Delete; the actual erase happens here. Node deletes
  // are recorded separately (deferred to actObjects); undoing replays nodes-first, then these edges.
  if (!deletedEdgeIds.empty())
  {
    doc.deleteEdges(deletedEdgeIds);
  }
}
