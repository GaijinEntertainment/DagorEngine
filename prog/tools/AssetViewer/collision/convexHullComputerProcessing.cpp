#include "convexHullComputerProcessing.h"
#include "propPanelPids.h"
#include "collisionUtils.h"
#include <debug/dag_debug3d.h>

void ConvexHullComputerProcessing::init(CollisionResource *collision_res, PropertyContainerControlBase *prop_panel)
{
  collisionRes = collision_res;
  panel = prop_panel;
}

void ConvexHullComputerProcessing::calcSelectedComputer()
{
  selectedSettings.shrink = panel->getFloat(PID_CONVEX_COMPUTER_SHRINK);
  calcComputer(selectedSettings);
}

void ConvexHullComputerProcessing::calcComputer(const ConvexComputerSettings &settings)
{
  dag::Vector<Point3_vec4> verts;
  dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
  float shrink = settings.shrink;
  for (const auto &refNode : settings.selectedNodes.refNodes)
  {
    G_ASSERT_LOG(add_verts_from_node(nodes, refNode, verts), "Collision node not found: %s", refNode);
  }

  if (!verts.empty())
    selectedComputer.compute(reinterpret_cast<float *>(verts.data()), sizeof(Point3_vec4), verts.size(), shrink, 0.0f);
}

void ConvexHullComputerProcessing::setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes)
{
  selectedSettings.selectedNodes = eastl::move(selected_nodes);
}

void ConvexHullComputerProcessing::addSelectedComputer() { computers.push_back(eastl::move(selectedComputer)); }

void ConvexHullComputerProcessing::setConvexComputerParams(const ConvexComputerSettings &settings)
{
  selectedSettings.shrink = settings.shrink;
}

void ConvexHullComputerProcessing::updatePanelParams() { panel->setInt(PID_CONVEX_COMPUTER_SHRINK, selectedSettings.shrink); }

void ConvexHullComputerProcessing::renderComputedConvex(bool is_faded)
{
  const auto drawConvex = [&](const ConvexHullComputer &computer, bool is_faded = false,
                            E3DCOLOR color = E3DCOLOR_MAKE(153, 255, 153, 255)) {
    if (is_faded)
      color.a = 30;
    for (int i = 0; i < computer.edges.size(); ++i)
    {
      const vec4f &p1Raw = computer.vertices[computer.edges[i].getSourceVertex()];
      const vec4f &p2Raw = computer.vertices[computer.edges[i].getTargetVertex()];
      Point3 p1 = {v_extract_x(p1Raw), v_extract_y(p1Raw), v_extract_z(p1Raw)};
      Point3 p2 = {v_extract_x(p2Raw), v_extract_y(p2Raw), v_extract_z(p2Raw)};
      draw_cached_debug_line(p1, p2, color);
    }
  };

  begin_draw_cached_debug_lines();

  for (int i = 0; i < computers.size(); ++i)
  {
    drawConvex(computers[i], is_faded, E3DCOLOR(colors[(i) % (sizeof(colors) / sizeof(colors[0]))]));
  }

  if (showConvexComputed && !selectedComputer.vertices.empty())
  {
    drawConvex(selectedComputer);
  }

  end_draw_cached_debug_lines();
}

void ConvexHullComputerProcessing::fillConvexComputerPanel()
{
  PropertyContainerControlBase &convexComputerGroup = *panel->createGroup(PID_CONVEX_COMPUTER_GROUP, "convex computer options");
  convexComputerGroup.createCheckBox(PID_SHOW_CONVEX_COMPUTER, "Show computed convex", showConvexComputed);
  convexComputerGroup.createTrackFloat(PID_CONVEX_COMPUTER_SHRINK, "Shrink", 0.f, 0.f, 10.f, 0.01f);
  convexComputerGroup.createSeparator();
  convexComputerGroup.createButton(PID_SAVE_CONVEX_COMPUTER, "Save settings");
  convexComputerGroup.createButton(PID_CANCEL_CONVEX_COMPUTER, "Cancel");
}
