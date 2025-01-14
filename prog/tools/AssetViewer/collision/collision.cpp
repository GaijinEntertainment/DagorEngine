// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision.h"
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_collisionResource.h>
#include <generic/dag_sort.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <math/dag_capsule.h>
#include <math/dag_geomTree.h>
#include <osApiWrappers/dag_clipboard.h>
#include <scene/dag_physMat.h>
#include <libTools/util/makeBindump.h>
#include <EASTL/optional.h>
#include <winGuiWrapper/wgw_input.h>
#include <render/debug3dSolid.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>

#include <gui/dag_stdGuiRenderEx.h>
#include "../av_appwnd.h"
#include "../av_viewportWindow.h"
#include "../entity/assetStatsFiller.h"
#include "../../sceneTools/assetExp/exporters/getSkeleton.h"
#include "propPanelPids.h"
#include "collisionUtils.h"
#include <drv/3d/dag_renderTarget.h>

const unsigned int phys_collidable_color = 0xFF00FF00;
const unsigned int traceable_color = 0xFFFF0000;

static int debug_ri_face_orientationVarId = -1;

enum
{
  CM_HIDE = 1,
  CM_UNHIDE,
  CM_ISOLATE,
  CM_UNHIDE_ALL,
  CM_COPY_NAME,
};

struct CollisionNodesData
{
  int nodeId;
  Point2 pos;
  real z;
};

static inline int cmpNodesData(const CollisionNodesData *n1, const CollisionNodesData *n2) { return n2->z - n1->z; }

static inline real getMinP3(const Point3 &p3, real clamp)
{
  real m = p3.x < p3.y ? p3.x : p3.y;
  m = m < p3.z ? m : p3.z;
  return m < clamp ? clamp : m;
}

enum CollisionNodeVisibility
{
  INVISIBLE,
  SHOW_AS_PHYS_COLLIDABLE,
  SHOW_AS_TRACEABLE,
  SHOW_AS_USUAL
};

static inline CollisionNodeVisibility getNodeVisibility(const CollisionNode &node, const bool showPhysCollidable,
  const bool showTraceable)
{
  const bool isPhysCollidable = node.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
  const bool isTraceable = node.checkBehaviorFlags(CollisionNode::TRACEABLE);
  const bool showAsPhysCollidable = showPhysCollidable && isPhysCollidable;
  const bool showAsTraceable = showTraceable && isTraceable;

  if ((showPhysCollidable || showTraceable) && !(showAsPhysCollidable || showAsTraceable))
    return CollisionNodeVisibility::INVISIBLE;
  else if (showAsPhysCollidable)
    return CollisionNodeVisibility::SHOW_AS_PHYS_COLLIDABLE;
  else if (showAsTraceable)
    return CollisionNodeVisibility::SHOW_AS_TRACEABLE;
  else
    return CollisionNodeVisibility::SHOW_AS_USUAL;
}

CollisionPlugin::CollisionPlugin()
{
  self = this;
  drawNodeAnotate = true;
  showBbox = false;
  showPhysCollidable = false;
  showTraceable = false;
  drawSolid = false;
  showFaceOrientation = false;
  collisionRes = NULL;
  nodeTree = NULL;
  selectedNodeId = -1;

  MaterialData matNull;
  matNull.className = "debug_ri";
  Ptr<ShaderMaterial> debugCollisionMat = new_shader_material(matNull, false, false);
  isSolidMatValid = debugCollisionMat != nullptr;

  initScriptPanelEditor("collision.scheme.nut", "collision by scheme");
  debug_ri_face_orientationVarId = get_shader_variable_id("debug_ri_face_orientation", true);
  if (VariableMap::isVariablePresent(debug_ri_face_orientationVarId) && !::dgs_get_game_params()->getStr("debugRiTexture", nullptr))
  {
    debug_ri_face_orientationVarId = -1;
    logerr("debugRiTexture:t= not set in gameParams, \"Show face orientation\" is disabled");
  }
}

void CollisionPlugin::onSaveLibrary() { nodesProcessing.saveCollisionNodes(); }

bool CollisionPlugin::begin(DagorAsset *asset)
{
  if (spEditor && asset)
    spEditor->load(asset);

  if (asset)
  {
    InitCollisionResource(*asset, &collisionRes, &nodeTree);
    nodesProcessing.init(asset, collisionRes, this);
  }

  return true;
}

bool CollisionPlugin::end()
{
  if (!nodesProcessing.canChangeAsset())
    return false;

  if (spEditor)
    spEditor->destroyPanel();

  nodesProcessing.saveExportedCollisionNodes();
  selectedNodeId = -1;

  clearAssetStats();
  ReleaseCollisionResource(&collisionRes, &nodeTree);
  return true;
}

bool CollisionPlugin::getSelectionBox(BBox3 &box) const
{
  if (!collisionRes)
    return false;

  box = collisionRes->boundingBox;
  return true;
}

bool CollisionPlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "collision") == 0; }

void CollisionPlugin::renderTransObjects()
{
  if (!collisionRes)
    return;

  if (showFaceOrientation)
  {
    updateFaceOrientationRenderDepthFromCurRT();
    d3d::set_depth(faceOrientationRenderDepth.getTex2D(), DepthAccess::RW);
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
  }

  RenderCollisionResource(*collisionRes, nodeTree, showBbox, showPhysCollidable, showTraceable, drawSolid, showFaceOrientation,
    selectedNodeId, nodesProcessing.editMode, nodesProcessing.selectionNodesProcessing.hiddenNodes);
  nodesProcessing.renderNodes(selectedNodeId, drawSolid);

  fillAssetStats();
}

void CollisionPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  panel.setEventHandler(this);

  panel.createCheckBox(PID_DRAW_NODE_ANOTATE, "draw node anotate", drawNodeAnotate);
  panel.createCheckBox(PID_SHOW_BBOX, "Show bounding box", showBbox);
  panel.createCheckBox(PID_SHOW_PHYS_COLLIDABLE, "Show Phys Collidable (green)", showPhysCollidable);
  panel.createCheckBox(PID_SHOW_TRACEABLE, "Show Traceable (red)", showTraceable);
  panel.createCheckBox(PID_DRAW_SOLID, "Draw collision solid", drawSolid, isSolidMatValid);
  if (VariableMap::isVariablePresent(debug_ri_face_orientationVarId))
  {
    panel.createCheckBox(PID_SHOW_FACE_ORIENTATION, "Show face orientation", showFaceOrientation, isSolidMatValid);
    panel.setTooltipId(PID_SHOW_FACE_ORIENTATION, "The front side of triangles is filled by blue color, and back side is red.");
  }

  nodesProcessing.setPropPanel(&panel);
  nodesProcessing.fillCollisionInfoPanel();
  nodesProcessing.setPanelAfterReject();
}

void CollisionPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_DRAW_NODE_ANOTATE:
      drawNodeAnotate = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_SHOW_BBOX:
      showBbox = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_SHOW_PHYS_COLLIDABLE:
      showPhysCollidable = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_SHOW_TRACEABLE:
      showTraceable = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_DRAW_SOLID:
      drawSolid = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_SHOW_FACE_ORIENTATION:
      showFaceOrientation = panel->getBool(pcb_id);
      repaintView();
      break;

    case PID_PRINT_KDOP_LOG: printKdopLog(); break;
    case PID_NEXT_EDIT_NODE: selectedNodeId = -1; break;
  }
  nodesProcessing.onClick(pcb_id);
}

void CollisionPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_SELECTABLE_NODES_LIST)
    selectedNodeId = -1;
  else if (pcb_id == PID_COLLISION_NODES_TREE)
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_COLLISION_NODES_TREE)->getContainer();
    PropPanel::TLeafHandle leaf = tree->getSelLeaf();
    selectedNodeId = getNodeIdx(tree, leaf);
    PropPanel::TLeafHandle rootLeaf = tree->getRootLeaf();
    for (PropPanel::TLeafHandle leaf = rootLeaf; leaf;)
    {
      int idx = getNodeIdx(tree, leaf);
      if (idx != -1)
        nodesProcessing.selectionNodesProcessing.hiddenNodes[idx] = !tree->getCheckboxValue(leaf);

      leaf = tree->getNextLeaf(leaf);
      if (leaf == rootLeaf)
        break;
    }
  }
  nodesProcessing.onChange(pcb_id);
}

static bool trace_ray_through_nodes(CollisionResource *collision_res, IGenViewportWnd *wnd, int x, int y,
  CollResIntersectionsType &intersected_nodes_list)
{
  CollResIntersectionsType physIntersectedNodesList;
  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);
  const float t = 1000.f;
  collision_res->traceRay(TMatrix::IDENT, NULL, world, dir, t, intersected_nodes_list, false, CollisionNode::TRACEABLE);
  collision_res->traceRay(TMatrix::IDENT, NULL, world, dir, t, physIntersectedNodesList, false, CollisionNode::PHYS_COLLIDABLE);
  intersected_nodes_list.insert(intersected_nodes_list.end(), physIntersectedNodesList.begin(), physIntersectedNodesList.end());
  stlsort::sort_branchless(intersected_nodes_list.begin(), intersected_nodes_list.end());

  auto compare = [](const IntersectedNode &lhs, const IntersectedNode &rhs) { return lhs.collisionNodeId == rhs.collisionNodeId; };

  intersected_nodes_list.erase(eastl::unique(intersected_nodes_list.begin(), intersected_nodes_list.end(), compare),
    intersected_nodes_list.end());
  return !intersected_nodes_list.empty();
}

bool CollisionPlugin::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (!collisionRes)
    return false;

  CollResIntersectionsType sortedIntersectedNodesList;
  if (trace_ray_through_nodes(collisionRes, wnd, x, y, sortedIntersectedNodesList))
  {
    dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
    for (int i = 0; i < (int)sortedIntersectedNodesList.size(); ++i)
    {
      const int candidateId = sortedIntersectedNodesList[i].collisionNodeId;
      dag::Vector<bool> &hiddenNodes = nodesProcessing.selectionNodesProcessing.hiddenNodes;
      const bool isVisible =
        getNodeVisibility(nodes[candidateId], showPhysCollidable, showTraceable) != CollisionNodeVisibility::INVISIBLE;
      if (isVisible && !hiddenNodes[candidateId])
      {
        if (selectedNodeId != candidateId)
        {
          selectedNodeId = candidateId;
          nodesProcessing.selectNode(nodes[selectedNodeId].name, key_modif == wingw::M_CTRL);
          return false;
        }
      }
    }
  }
  selectedNodeId = -1;
  nodesProcessing.selectNode(nullptr, key_modif == wingw::M_CTRL);
  return false;
}

bool CollisionPlugin::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  PropPanel::TLeafHandle selection = tree.getSelLeaf();
  PropPanel::TLeafHandle parent = tree.getParentLeaf(selection);
  // We want check only top level nodes
  if (!parent)
  {
    const int selectedIdx = getNodeIdx(&tree, selection);
    if (selectedIdx < 0)
    {
      logerr("Can't find selected collision node in resource with name <%s>", tree.getCaption().str());
      return false;
    }

    PropPanel::IMenu &menu = tree_interface.createContextMenu();
    menu.setEventHandler(this);
    dag::Vector<bool> &hiddenNodes = nodesProcessing.selectionNodesProcessing.hiddenNodes;
    if (hiddenNodes[selectedIdx])
      menu.addItem(ROOT_MENU_ITEM, CM_UNHIDE, "Unhide");
    else
      menu.addItem(ROOT_MENU_ITEM, CM_HIDE, "Hide");

    menu.addItem(ROOT_MENU_ITEM, CM_ISOLATE, "Isolate");
    menu.addItem(ROOT_MENU_ITEM, CM_UNHIDE_ALL, "Unhide all");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_NAME, "Copy name");
    return true;
  }

  return false;
}

int CollisionPlugin::onMenuItemClick(unsigned id)
{
  PropPanel::ContainerPropertyControl *panel = getPluginPanel();
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_COLLISION_NODES_TREE)->getContainer();

  dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
  tree->getSelectedLeafs(selectedLeafs);

  dag::Vector<int> selectedIndices;
  for (PropPanel::TLeafHandle selLeaf : selectedLeafs)
  {
    const int selectedIdx = getNodeIdx(tree, selLeaf);
    if (selectedIdx < 0)
    {
      logerr("Can't find selected collision node in resource with name <%s>", tree->getCaption(selLeaf).str());
      return 1;
    }

    selectedIndices.push_back(selectedIdx);
  }

  dag::Vector<bool> &hiddenNodes = nodesProcessing.selectionNodesProcessing.hiddenNodes;
  switch (id)
  {
    case CM_HIDE:
      for (int selectedIdx : selectedIndices)
        hiddenNodes[selectedIdx] = true;
      for (PropPanel::TLeafHandle selLeaf : selectedLeafs)
        tree->setCheckboxValue(selLeaf, false);
      break;
    case CM_UNHIDE:
      for (int selectedIdx : selectedIndices)
        hiddenNodes[selectedIdx] = false;
      for (PropPanel::TLeafHandle selLeaf : selectedLeafs)
        tree->setCheckboxValue(selLeaf, true);
      break;
    case CM_ISOLATE:
    {
      eastl::fill(hiddenNodes.begin(), hiddenNodes.end(), true);
      for (int selectedIdx : selectedIndices)
        hiddenNodes[selectedIdx] = false;
      PropPanel::TLeafHandle rootLeaf = tree->getRootLeaf();
      for (PropPanel::TLeafHandle leaf = rootLeaf; leaf;)
      {
        if (tree->isCheckboxEnable(leaf))
        {
          const bool selected = eastl::find(selectedLeafs.begin(), selectedLeafs.end(), leaf) != selectedLeafs.end();
          tree->setCheckboxValue(leaf, selected);
        }

        leaf = tree->getNextLeaf(leaf);
        if (leaf == rootLeaf)
          break;
      }
      break;
    }
    case CM_UNHIDE_ALL:
    {
      nodesProcessing.selectionNodesProcessing.updateHiddenNodes();
      PropPanel::TLeafHandle rootLeaf = tree->getRootLeaf();
      for (PropPanel::TLeafHandle leaf = rootLeaf; leaf;)
      {
        if (tree->isCheckboxEnable(leaf))
          tree->setCheckboxValue(leaf, true);

        leaf = tree->getNextLeaf(leaf);
        if (leaf == rootLeaf)
          break;
      }
      break;
    }
    case CM_COPY_NAME:
    {
      String text;

      for (PropPanel::TLeafHandle leaf : selectedLeafs)
      {
        String name = tree->getCaption(leaf);
        NodesProcessing::delete_flags_prefix(name);
        if (!text.empty())
          text += '\n';
        text += name;
      }

      if (!text.empty())
        clipboard::set_clipboard_ansi_text(text);

      break;
    }
  }

  return 0;
}

void CollisionPlugin::drawObjects(IGenViewportWnd *wnd)
{
  if (!drawNodeAnotate || !collisionRes || !collisionRes->getAllNodes().size())
    return;

  static Tab<CollisionNodesData> nodesData(midmem);
  nodesData.clear();

  const auto allNodes = collisionRes->getAllNodes();
  int nodesCnt = allNodes.size();
  for (int i = 0; i < nodesCnt; i++)
  {
    Point2 pos;
    real z;

    Point3 pos3 = allNodes[i].tm * allNodes[i].boundingSphere.c;

    if (!wnd->worldToClient(pos3, pos, &z) || (z < 0.001))
      continue;

    append_items(nodesData, 1);

    nodesData.back().pos = pos;
    nodesData.back().z = z;
    nodesData.back().nodeId = i;
  }

  if (!nodesData.size())
    return;

  sort(nodesData, &cmpNodesData);

  StdGuiRender::set_font(0);
  for (int i = nodesData.size() - 1; i >= 0; i--)
  {
    int id = nodesData[i].nodeId;
    const CollisionNode &node = allNodes[id];
    const char *name = node.name;
    if (!name || (selectedNodeId >= 0 && selectedNodeId != id))
      continue;

    Point2 &screen = nodesData[i].pos;

    String selectedName(128, name);
    if (selectedNodeId >= 0)
    {
      selectedName.aprintf(128, " | triangles %d, phmat '%s'", node.indices.size() / 3,
        node.physMatId != PHYSMAT_INVALID ? PhysMat::getMaterial(node.physMatId).name : "none");
      if (node.behaviorFlags >> 3)
      {
        selectedName.aprintf(128, " Flags : %s %s %s %s",
          (node.behaviorFlags & CollisionNode::FLAG_ALLOW_HOLE) ? "" : "noOverlapHoles |",
          (node.behaviorFlags & CollisionNode::FLAG_CUT_REQUIRED) ? "noOverlapHolesIfNoCut |" : "",
          (node.behaviorFlags & CollisionNode::FLAG_CHECK_SIDE) ? "noOverlapEdgeHoles |" : "",
          (node.behaviorFlags & CollisionNode::FLAG_ALLOW_BULLET_DECAL) ? "" : "noBullets |");
      }
    }
    const bool isHidden = nodesProcessing.selectionNodesProcessing.hiddenNodes[i];
    if (!isHidden)
    {
      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_strf_to(screen.x + 1, screen.y + 1, selectedName.str());
      StdGuiRender::set_color(COLOR_LTGREEN);
      StdGuiRender::draw_strf_to(screen.x, screen.y, selectedName.str());
    }
  }
}

void CollisionPlugin::printKdopLog()
{
  const Kdop &kdop = nodesProcessing.kdopSetupProcessing.selectedKdop;
  getMainConsole().addMessage(ILogWriter::NOTE, "K-dop center (%f %f %f)", kdop.center.x, kdop.center.y, kdop.center.z);
  for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
  {
    getMainConsole().addMessage(ILogWriter::NOTE, "K-dop direction #%d %f %f %f with max %f", i,
      kdop.planeDefinitions[i].planeNormal.x, kdop.planeDefinitions[i].planeNormal.y, kdop.planeDefinitions[i].planeNormal.z,
      kdop.planeDefinitions[i].limit);
  }
  for (int i = 0; i < kdop.vertices.size(); ++i)
  {
    getMainConsole().addMessage(ILogWriter::NOTE, "Vertex #%d (%f %f %f)", i, kdop.vertices[i].x, kdop.vertices[i].y,
      kdop.vertices[i].z);
  }
  for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
  {
    for (int j = 0; j < kdop.planeDefinitions[i].vertices.size(); ++j)
    {
      getMainConsole().addMessage(ILogWriter::NOTE, "Plane #%d, Vertex #%d (%f %f %f)", i, j, kdop.planeDefinitions[i].vertices[j].x,
        kdop.planeDefinitions[i].vertices[j].y, kdop.planeDefinitions[i].vertices[j].z);
    }
    for (int j = 0; j < kdop.planeDefinitions[i].indices.size(); ++j)
    {
      getMainConsole().addMessage(ILogWriter::NOTE, "Index #%d, Vertex #%d", i, kdop.planeDefinitions[i].indices[j]);
    }
  }
  for (int i = 0; i < kdop.indices.size(); i += 3)
  {
    getMainConsole().addMessage(ILogWriter::NOTE, "Face #%d (%d %d %d)", i, kdop.indices[i], kdop.indices[i + 1], kdop.indices[i + 2]);
  }

  Point3 c0 = kdop.rm.getcol(0);
  Point3 c1 = kdop.rm.getcol(1);
  Point3 c2 = kdop.rm.getcol(2);

  getMainConsole().addMessage(ILogWriter::NOTE, "Rot matrix:");
  getMainConsole().addMessage(ILogWriter::NOTE, "(%f, %f, %f)", c0.x, c1.x, c2.x);
  getMainConsole().addMessage(ILogWriter::NOTE, "(%f, %f, %f)", c0.y, c1.y, c2.y);
  getMainConsole().addMessage(ILogWriter::NOTE, "(%f, %f, %f)", c0.z, c1.z, c2.z);

  getMainConsole().addMessage(ILogWriter::NOTE, "Planes area:");
  for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
  {
    getMainConsole().addMessage(ILogWriter::NOTE, "%d: %f", i, kdop.planeDefinitions[i].area);
  }
}

void CollisionPlugin::clearAssetStats()
{
  AssetViewerViewportWindow *viewport = static_cast<AssetViewerViewportWindow *>(EDITORCORE->getCurrentViewport());
  if (viewport)
    viewport->getAssetStats().clear();
}

void CollisionPlugin::fillAssetStats()
{
  AssetViewerViewportWindow *viewport = static_cast<AssetViewerViewportWindow *>(EDITORCORE->getCurrentViewport());
  if (!viewport || !viewport->needShowAssetStats())
    return;

  AssetStats &stats = viewport->getAssetStats();
  stats.clear();
  stats.assetType = AssetStats::AssetType::Collision;

  if (collisionRes)
  {
    AssetStatsFiller assetStatsFiller(stats);
    assetStatsFiller.fillAssetCollisionStats(*collisionRes);
    assetStatsFiller.finalizeStats();
  }
}

void CollisionPlugin::updateFaceOrientationRenderDepthFromCurRT()
{
  int targetW, targetH;
  d3d::get_target_size(targetW, targetH);

  if (faceOrientationRenderDepth)
  {
    TextureInfo depthInfo;
    faceOrientationRenderDepth.getTex2D()->getinfo(depthInfo);
    if (depthInfo.w == targetW && depthInfo.h == targetH)
      return;
  }

  faceOrientationRenderDepth.close();
  faceOrientationRenderDepth =
    dag::create_tex(nullptr, targetW, targetH, TEXCF_RTARGET | TEXFMT_DEPTH32, 1, "face_orient_render_depth");
}

int CollisionPlugin::getNodeIdx(PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle leaf)
{
  if (leaf)
  {
    String sel_name = tree->getCaption(leaf);
    NodesProcessing::delete_flags_prefix(sel_name);
    for (const auto &n : collisionRes->getAllNodes())
      if (sel_name == n.name.str())
        return &n - collisionRes->getAllNodes().data();
  }
  return -1;
}

static void reset_nodes_tm(CollisionResource *collision_res, GeomNodeTree *node_tree)
{
  if (node_tree)
    return;

  for (auto &node : collision_res->getAllNodes())
  {
    if (
      (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CAPSULE || node.type == COLLISION_NODE_TYPE_CONVEX) &&
      node.tm.det() < 0.0f)
    {
      for (int i = 0; i < node.indices.size(); i += 3)
        eastl::swap(node.indices[i], node.indices[i + 1]);

      for (auto &vert : node.vertices)
        vert = node.tm * vert;

      node.modelBBox = node.tm * node.modelBBox;
      node.boundingSphere = node.tm * node.boundingSphere;

      node.tm = TMatrix::IDENT;
    }
  }
}

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTree **node_tree)
{
  *collision_res = (CollisionResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(asset.getName()), CollisionGameResClassId);

  const DataBlock &props = asset.getProfileTargetProps(_MAKE4C('PC'), NULL);
  if (const char *skeletonName = props.getStr("ref_skeleton", nullptr))
  {
    *node_tree = new GeomNodeTree;

    if (getSkeleton(**node_tree, asset.getMgr(), skeletonName, ::get_app().getConsole()))
    {
      (*node_tree)->invalidateWtm();
      (*node_tree)->calcWtm();
      (*collision_res)->initializeWithGeomNodeTree(**node_tree);
    }
    else
    {
      delete *node_tree;
      *node_tree = nullptr;
    }
  }
  reset_nodes_tm(*collision_res, *node_tree);
}


void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTree **node_tree)
{
  if (*collision_res)
  {
    ::release_game_resource((GameResource *)*collision_res);
    *collision_res = NULL;
  }

  if (*node_tree)
  {
    delete *node_tree;
    *node_tree = nullptr;
  }
}

static void draw_collision_mesh(const CollisionNode &node, const TMatrix &tm, const E3DCOLOR &color, bool draw_solid,
  bool show_face_orientation)
{
  if (show_face_orientation)
  {
    ShaderGlobal::set_int(debug_ri_face_orientationVarId, 1);

    draw_debug_solid_mesh(node.indices.data(), node.indices.size() / 3, &node.vertices.data()->x, elem_size(node.vertices),
      node.vertices.size(), tm, E3DCOLOR(255, 255, 255), true, DrawSolidMeshCull::FLIP);

    ShaderGlobal::set_int(debug_ri_face_orientationVarId, 0);
  }
  else if (draw_solid)
  {
    draw_debug_solid_mesh(node.indices.data(), node.indices.size() / 3, &node.vertices.data()->x, elem_size(node.vertices),
      node.vertices.size(), tm, color, false, DrawSolidMeshCull::FLIP);
  }

  for (int j = 0; j < node.indices.size(); j += 3)
  {
    Point3 p1 = node.vertices[node.indices[j + 0]];
    Point3 p2 = node.vertices[node.indices[j + 1]];
    Point3 p3 = node.vertices[node.indices[j + 2]];

    draw_cached_debug_line(p1, p2, color);
    draw_cached_debug_line(p2, p3, color);
    draw_cached_debug_line(p3, p1, color);
  }
}

void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, bool show_bbox,
  bool show_phys_collidable, bool show_traceable, bool draw_solid, bool show_face_orientation, int selected_node_id, bool edit_mode,
  const dag::Vector<bool> &hidden_nodes)
{
  begin_draw_cached_debug_lines();

  if (show_bbox)
    draw_cached_debug_box(collision_res.boundingBox, E3DCOLOR_MAKE(255, 255, 255, 255));

  const bool selectedNodeNotHidden = !hidden_nodes.empty() && selected_node_id >= 0 && !hidden_nodes[selected_node_id];
  const auto allNodes = collision_res.getAllNodes();
  int cnt = allNodes.size();
  for (int i = 0; i < cnt; i++)
  {
    const CollisionNode &node = allNodes[i];

    const CollisionNodeVisibility visibility = getNodeVisibility(node, show_phys_collidable, show_traceable);
    const bool isVisible = visibility == CollisionNodeVisibility::INVISIBLE;
    const bool isHidden = !hidden_nodes.empty() && hidden_nodes[i];
    if (isVisible || isHidden)
      continue;

    eastl::optional<E3DCOLOR> customColor;
    if (visibility == CollisionNodeVisibility::SHOW_AS_PHYS_COLLIDABLE)
      customColor = E3DCOLOR(phys_collidable_color);
    else if (visibility == CollisionNodeVisibility::SHOW_AS_TRACEABLE)
      customColor = E3DCOLOR(traceable_color);

    int alpha = 255;
    const bool isNotSelectedNode = i != selected_node_id;
    const bool hasSelectedNode = selected_node_id >= 0 && selectedNodeNotHidden;
    if (isNotSelectedNode && (hasSelectedNode || edit_mode))
    {
      alpha = 30;
      if (customColor)
        customColor.value().a = alpha;
    }

    if (node.type == COLLISION_NODE_TYPE_BOX)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 255, 255, alpha);
      if (draw_solid)
        draw_debug_solid_cube(node.modelBBox, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_box(node.modelBBox, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_SPHERE)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 255, 0, alpha);
      if (draw_solid)
        draw_debug_solid_sphere(node.boundingSphere.c, node.boundingSphere.r, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_sphere(node.boundingSphere.c, node.boundingSphere.r, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 0, 255, alpha);
      if (draw_solid)
        draw_debug_solid_capsule(node.capsule, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_capsule_w(node.capsule, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_MESH)
    {
      TMatrix nodeTm = node.tm;
      if (node_tree)
        collision_res.getCollisionNodeTm(&node, TMatrix::IDENT, node_tree, nodeTm);

      set_cached_debug_lines_wtm(nodeTm);

      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]);
      color.a = alpha;

      draw_collision_mesh(node, nodeTm, color, draw_solid, show_face_orientation);
    }
    else if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);

      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]);

      Tab<Point3> vertList;
      for (int j = 0; j < node.indices.size(); ++j)
        vertList.push_back(node.tm * node.vertices[node.indices[j]]);
      bool haveInvalidVertices = false;
      const float distEps = 1e-3f;
      vec4f nodeEps = v_splats(max(node.boundingSphere.r * distEps, 1e-2f));
      for (int k = 0; k < node.convexPlanes.size(); ++k)
      {
        const plane3f &plane = node.convexPlanes[k];
        for (const Point3_vec4 &vert : node.vertices)
        {
          vec4f vertex = v_ld(&vert.x);
          vec4f dist = v_plane_dist(plane, vertex);
          if (v_test_vec_x_gt(dist, nodeEps))
          {
            haveInvalidVertices = true;
            vec4f v_projPt = v_add(vertex, v_neg(v_mul(dist, plane)));
            Point3_vec4 projPt;
            v_st(&projPt.x, v_projPt);
            draw_cached_debug_line(node.tm * vert, node.tm * projPt, E3DCOLOR_MAKE(255, 0, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 0]], node.tm * node.vertices[node.indices[k * 3 + 1]],
              E3DCOLOR_MAKE(255, 255, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 1]], node.tm * node.vertices[node.indices[k * 3 + 2]],
              E3DCOLOR_MAKE(255, 255, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 2]], node.tm * node.vertices[node.indices[k * 3 + 0]],
              E3DCOLOR_MAKE(255, 255, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 0]], node.tm * projPt,
              E3DCOLOR_MAKE(255, 255, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 1]], node.tm * projPt,
              E3DCOLOR_MAKE(255, 255, 0, 255));
            draw_cached_debug_line(node.tm * node.vertices[node.indices[k * 3 + 2]], node.tm * projPt,
              E3DCOLOR_MAKE(255, 255, 0, 255));
          }
        }
      }
      color.a = clamp(alpha, 0, haveInvalidVertices ? 40 : 128);
      if (draw_solid)
        draw_collision_mesh(node, TMatrix::IDENT, color, draw_solid, show_face_orientation);
      else
        draw_cached_debug_trilist(vertList.data(), vertList.size() / 3, color);
    }
  }

  end_draw_cached_debug_lines();
}
