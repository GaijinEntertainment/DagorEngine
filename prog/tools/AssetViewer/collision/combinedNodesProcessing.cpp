// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "combinedNodesProcessing.h"
#include "collisionUtils.h"
#include "propPanelPids.h"
#include <debug/dag_debug3d.h>
#include <render/debug3dSolid.h>
#include <math/dag_color.h>

void CombinedNodesProcessing::init(CollisionResource *collision_res, PropPanel::ContainerPropertyControl *prop_panel)
{
  collisionRes = collision_res;
  panel = prop_panel;
}

void CombinedNodesProcessing::calcSelectedCombinedNode() { calcCombinedNode(selectedSettings); }

void CombinedNodesProcessing::calcCombinedNode(const SelectedNodesSettings &settings)
{
  dag::ConstSpan<CollisionNode> collisionNodes = collisionRes->getAllNodes();
  clearSelectedNode();
  switch (settings.type)
  {
    case ExportCollisionNodeType::MESH:
    case ExportCollisionNodeType::KDOP:
    case ExportCollisionNodeType::CONVEX_COMPUTER:
    case ExportCollisionNodeType::CONVEX_VHACD:
      for (const auto &refNode : settings.refNodes)
      {
        add_verts_and_indices_from_node(collisionNodes, refNode, selectedNode.vertices, selectedNode.indices);
      }
      break;

    case ExportCollisionNodeType::BOX:
      for (const auto &refNode : settings.refNodes)
      {
        for (const auto &node : collisionNodes)
        {
          if (refNode == node.name.c_str())
          {
            if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CAPSULE ||
                node.type == COLLISION_NODE_TYPE_CONVEX)
              selectedNode.boundingBox += node.tm * node.modelBBox;
            else
              selectedNode.boundingBox += node.modelBBox;
          }
        }
      }
      break;

    case ExportCollisionNodeType::SPHERE:
      for (const auto &refNode : settings.refNodes)
      {
        for (const auto &node : collisionNodes)
        {
          if (refNode == node.name.c_str())
          {
            selectedNode.boundingSphere += node.tm * node.boundingSphere;
          }
        }
      }
      break;

    // to prevent the unhandled switch case error
    case ExportCollisionNodeType::UNKNOWN_TYPE:
    case ExportCollisionNodeType::NODE_TYPES_COUNT: break;
  }
}

void CombinedNodesProcessing::setSelectedNodesSettings(SelectedNodesSettings &&selected_nodes)
{
  selectedSettings = eastl::move(selected_nodes);
}

void CombinedNodesProcessing::addSelectedCombinedNode() { nodes.push_back(eastl::move(selectedNode)); }

void CombinedNodesProcessing::clearSelectedNode()
{
  selectedNode.boundingBox.setempty();
  selectedNode.boundingSphere.setempty();
  selectedNode.vertices.clear();
  selectedNode.indices.clear();
}

void CombinedNodesProcessing::renderCombinedNodes(bool is_faded, bool draw_solid,
  const dag::Vector<SelectedNodesSettings> &combined_settings)
{
  const auto drawCombinedNode = [&](const SelectedNodesSettings &settings, const CombinedNode &node, bool is_faded = false,
                                  E3DCOLOR color = E3DCOLOR_MAKE(153, 255, 153, 255)) {
    if (is_faded)
      color.a = 30;
    switch (settings.type)
    {
      case ExportCollisionNodeType::MESH:
      {
        if (draw_solid)
        {
          draw_debug_solid_mesh(node.indices.data(), node.indices.size() / 3, &node.vertices.data()->x, elem_size(node.vertices),
            node.vertices.size(), TMatrix::IDENT, color, false, DrawSolidMeshCull::FLIP);
        }
        for (int i = 0; i < node.indices.size(); i += 3)
        {
          Point3 p1 = node.vertices[node.indices[i + 0]];
          Point3 p2 = node.vertices[node.indices[i + 1]];
          Point3 p3 = node.vertices[node.indices[i + 2]];

          draw_cached_debug_line(p1, p2, color);
          draw_cached_debug_line(p2, p3, color);
          draw_cached_debug_line(p3, p1, color);
        }
        break;
      }
      case ExportCollisionNodeType::BOX:
        if (draw_solid)
          draw_debug_solid_cube(node.boundingBox, TMatrix::IDENT, color);
        draw_cached_debug_box(node.boundingBox, color);
        break;

      case ExportCollisionNodeType::SPHERE:
        if (draw_solid)
          draw_debug_solid_sphere(node.boundingSphere.c, node.boundingSphere.r, TMatrix::IDENT, color);
        draw_cached_debug_sphere(node.boundingSphere.c, node.boundingSphere.r, color);
        break;

      default: debug("Wrong node for rendering: %s", settings.nodeName); break;
    }
  };

  begin_draw_cached_debug_lines();

  set_cached_debug_lines_wtm(TMatrix::IDENT);

  for (int i = 0; i < nodes.size(); ++i)
  {
    drawCombinedNode(combined_settings[i], nodes[i], is_faded, E3DCOLOR(colors[(i) % (sizeof(colors) / sizeof(colors[0]))]));
  }

  drawCombinedNode(selectedSettings, selectedNode);

  end_draw_cached_debug_lines();
}

void CombinedNodesProcessing::updateSeletectedSettings()
{
  selectedSettings.nodeName = panel->getText(PID_NEW_NODE_NAME);
  selectedSettings.nodeName.toLower();
  selectedSettings.isTraceable = panel->getBool(PID_TRACEABLE_FLAG);
  selectedSettings.isPhysCollidable = panel->getBool(PID_PHYS_COLLIDABLE_FLAG);
  selectedSettings.replaceNodes = panel->getBool(PID_REPLACE_NODE);
  selectedSettings.physMat = panel->getText(PID_MATERIAL_TYPE);
}
