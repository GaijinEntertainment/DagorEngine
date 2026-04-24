// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision.h"
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
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
#include <libTools/util/strUtil.h>
#include <EASTL/optional.h>
#include <winGuiWrapper/wgw_input.h>
#include <render/debug3dSolid.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>

#include <gui/dag_stdGuiRenderEx.h>
#include "../av_appwnd.h"
#include "../av_viewportWindow.h"
#include "../Entity/assetStatsFiller.h"
#include "../../sceneTools/assetExp/exporters/getSkeleton.h"
#include "propPanelPids.h"
#include "collisionUtils.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_viewScissor.h>

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

static const DagorAsset *get_model_asset_from_collision_asset(const DagorAsset &collision_asset)
{
  if (const char *name = collision_asset.props.getStr("ref_model", nullptr))
    if (const DagorAsset *modelAsset = DAEDITOR3.getAssetByName(name))
      return modelAsset;

  // Try common names as fallback.
  String name(collision_asset.getName());
  remove_trailing_string(name, "_collision");
  const DagorAsset *modelAsset = DAEDITOR3.getAssetByName(name);
  if (!modelAsset)
  {
    modelAsset = DAEDITOR3.getAssetByName(name + "_char");
    if (!modelAsset)
      modelAsset = DAEDITOR3.getAssetByName(name + "_dynmodel");
  }

  return modelAsset;
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
  showDegenerativeTriangles = false;
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
    curAsset = asset;
    if (showDegenerativeTriangles)
      degenerativeNodes = collisionRes->getDegenerativeNodes(curAsset->getName());

    if (const DagorAsset *modelAsset = get_model_asset_from_collision_asset(*asset))
      modelAssetName = modelAsset->getNameTypified();
    updateModel();
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
  destroy_it(modelEntity);
  modelAssetName.clear();
  curAsset = nullptr;
  degenerativeNodes.clear();
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
    showDegenerativeTriangles, degenerativeNodes, selectedNodeId, nodesProcessing.editMode,
    nodesProcessing.selectionNodesProcessing.hiddenNodes);
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
  panel.createCheckBox(PID_SHOW_MODEL, "Show model", showModel && !modelAssetName.empty(), !modelAssetName.empty());
  panel.setTooltipId(PID_SHOW_MODEL, modelAssetName);
  panel.createCheckBox(PID_SHOW_DEGENERATIVE_TRIANGLES, "Show degenerative triangles (for Jolt)", showDegenerativeTriangles);
  panel.setTooltipId(PID_SHOW_DEGENERATIVE_TRIANGLES,
    "Degenerate triangles highlighted with red lines and vertices shown as yellow spheres.");

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

    case PID_SHOW_MODEL:
      showModel = panel->getBool(pcb_id);
      updateModel();
      repaintView();
      break;

    case PID_SHOW_DEGENERATIVE_TRIANGLES:
      showDegenerativeTriangles = panel->getBool(pcb_id);
      if (showDegenerativeTriangles)
        degenerativeNodes = collisionRes->getDegenerativeNodes(curAsset->getName());
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
          nodesProcessing.selectNode(collisionRes->getNodeName(selectedNodeId), key_modif == wingw::M_CTRL);
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
  using PropPanel::ROOT_MENU_ITEM;
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
  tree->getSelectedLeafs(selectedLeafs, false, false);

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

    Point3 pos3 = collisionRes->getNodeTm(i) * collisionRes->getNodeBSphere(i).c;

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
    const char *name = collisionRes->getNodeName(id);
    if (!name || (selectedNodeId >= 0 && selectedNodeId != id))
      continue;

    Point2 &screen = nodesData[i].pos;

    String selectedName(128, name);
    if (selectedNodeId >= 0)
    {
      selectedName.aprintf(128, " | triangles %d, phmat '%s'", collisionRes->getNodeFaceCount(id),
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
      if (sel_name == collisionRes->getNodeName(n.nodeIndex))
        return &n - collisionRes->getAllNodes().data();
  }
  return -1;
}

void CollisionPlugin::updateModel()
{
  destroy_it(modelEntity);
  if (!showModel || modelAssetName.empty())
    return;

  if (DagorAsset *modelAsset = DAEDITOR3.getAssetByName(modelAssetName))
  {
    modelEntity = DAEDITOR3.createEntity(*modelAsset);
    if (modelEntity)
      modelEntity->setTm(TMatrix::IDENT);
  }
}

static void reset_nodes_tm(CollisionResource *collision_res, GeomNodeTree *node_tree)
{
  if (node_tree)
    return;

  auto allNodes = collision_res->getAllNodes();
  for (int ni = 0; ni < allNodes.size(); ni++)
  {
    auto &node = allNodes[ni];
    const TMatrix &nodeTmRef = collision_res->getNodeTm(ni);
    if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && nodeTmRef.det() < 0.0f)
      collision_res->bakeNodeTransform(ni);
  }
}

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTree **node_tree)
{
  *collision_res = (CollisionResource *)::get_game_resource_ex(asset.getName(), CollisionGameResClassId);

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
    ::release_game_resource_ex(*collision_res, CollisionGameResClassId);
    *collision_res = NULL;
  }

  if (*node_tree)
  {
    delete *node_tree;
    *node_tree = nullptr;
  }
}

struct ScreenSpaceParams
{
  TMatrix viewItm;
  Point3 camPos;
  float pixelToWorld;
};

static void draw_collision_mesh(const CollisionResource &collision_res, int node_id, const TMatrix &tm, const E3DCOLOR &color,
  dag::ConstSpan<uint16_t> degenerative_indices, bool draw_solid, bool show_face_orientation, const ScreenSpaceParams &params)
{
  if (show_face_orientation || draw_solid)
  {
    // draw_debug_solid_mesh uploads to a d3d buffer so it needs contiguous indices/vertices; materialize only when solid is drawn.
    const int faceCount = collision_res.getNodeFaceCount(node_id);
    const int vertCount = collision_res.getNodeVertCount(node_id);
    if (faceCount > 0 && vertCount > 0)
    {
      dag::Vector<uint16_t> idx(faceCount * 3);
      dag::Vector<Point3_vec4> verts(vertCount);
      collision_res.iterateNodeFaces(node_id, [&](int fi, uint16_t i0, uint16_t i1, uint16_t i2) {
        idx[fi * 3 + 0] = i0;
        idx[fi * 3 + 1] = i1;
        idx[fi * 3 + 2] = i2;
      });
      collision_res.iterateNodeVerts(node_id, [&](int vi, vec4f v) { v_st(&verts[vi].x, v); });
      if (show_face_orientation)
      {
        ShaderGlobal::set_int(debug_ri_face_orientationVarId, 1);
        draw_debug_solid_mesh(idx.data(), faceCount, &verts[0].x, sizeof(Point3_vec4), vertCount, tm, E3DCOLOR(255, 255, 255), true,
          DrawSolidMeshCull::FLIP);
        ShaderGlobal::set_int(debug_ri_face_orientationVarId, 0);
      }
      else
      {
        draw_debug_solid_mesh(idx.data(), faceCount, &verts[0].x, sizeof(Point3_vec4), vertCount, tm, color, false,
          DrawSolidMeshCull::FLIP);
      }
    }
  }

  collision_res.iterateNodeFacesVerts(node_id, [&](int, vec4f v0, vec4f v1, vec4f v2) {
    Point3_vec4 p1, p2, p3;
    v_st(&p1.x, v0);
    v_st(&p2.x, v1);
    v_st(&p3.x, v2);
    draw_cached_debug_line(p1, p2, color);
    draw_cached_debug_line(p2, p3, color);
    draw_cached_debug_line(p3, p1, color);
  });

  if (degenerative_indices.empty())
    return;

  // Flush mesh wireframe into its own draw call so degenerate markers always render on top,
  // preventing mesh edges from occluding them.
  flush_cached_debug_lines();
  const E3DCOLOR lineColor = E3DCOLOR(255, 0, 0, color.a);
  const E3DCOLOR markerColor = E3DCOLOR(255, 255, 0, color.a);

  constexpr float LINE_HALF_WIDTH_PX = 3.0f;
  constexpr float MARKER_RADIUS_PX = 8.0f;
  const float halfWidthScale = LINE_HALF_WIDTH_PX * params.pixelToWorld;
  const float markerRadiusScale = MARKER_RADIUS_PX * params.pixelToWorld;

  auto draw_thick_line = [&](const Point3 &a, const Point3 &b) {
    Point3 wa = tm * a;
    Point3 wb = tm * b;
    Point3 dir = wb - wa;
    float len = length(dir);
    if (len < 1e-6f)
      return;
    dir /= len;
    Point3 toMid = (wa + wb) * 0.5f - params.camPos;
    float toMidLen = length(toMid);
    if (toMidLen < 1e-6f)
      return;
    toMid /= toMidLen;
    Point3 sideDir = cross(dir, toMid);
    float sideDirLen = length(sideDir);
    if (sideDirLen < 1e-6f)
    {
      sideDir = cross(dir, params.viewItm.getcol(1));
      sideDirLen = length(sideDir);
      if (sideDirLen < 1e-6f)
      {
        sideDir = cross(dir, params.viewItm.getcol(0));
        sideDirLen = length(sideDir);
        if (sideDirLen < 1e-6f)
          return;
      }
    }
    sideDir /= sideDirLen;
    // Compute per-endpoint offsets so the quad has constant screen-space width along its full length
    const Point3 sideA = sideDir * (length(wa - params.camPos) * halfWidthScale);
    const Point3 sideB = sideDir * (length(wb - params.camPos) * halfWidthScale);
    Point3 q[4] = {wa - sideA, wa + sideA, wb + sideB, wb - sideB};
    draw_cached_debug_quad(q, lineColor);
  };

  auto draw_screen_marker = [&](const Point3 &p) {
    Point3 wp = tm * p;
    const float rad = length(wp - params.camPos) * markerRadiusScale;
    draw_cached_debug_hex(params.viewItm, wp, rad, markerColor);
  };

  // Degenerate markers index into the node's vertex array; materialize it once here since the debug-only path is rarely hit.
  dag::Vector<Point3_vec4> degenVerts(collision_res.getNodeVertCount(node_id));
  collision_res.iterateNodeVerts(node_id, [&](int vi, vec4f v) { v_st(&degenVerts[vi].x, v); });

  for (int i = 0; i + 2 < degenerative_indices.size(); i += 3)
  {
    const Point3 &p1 = degenVerts[degenerative_indices[i + 0]];
    const Point3 &p2 = degenVerts[degenerative_indices[i + 1]];
    const Point3 &p3 = degenVerts[degenerative_indices[i + 2]];
    draw_thick_line(p1, p2);
    draw_thick_line(p2, p3);
    draw_thick_line(p3, p1);
  }

  for (int i = 0; i + 2 < degenerative_indices.size(); i += 3)
  {
    draw_screen_marker(degenVerts[degenerative_indices[i + 0]]);
    draw_screen_marker(degenVerts[degenerative_indices[i + 1]]);
    draw_screen_marker(degenVerts[degenerative_indices[i + 2]]);
  }
}

void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, bool show_bbox,
  bool show_phys_collidable, bool show_traceable, bool draw_solid, bool show_face_orientation, bool show_degenerate_triangles,
  const dag::Vector<DegenerativeNodeData> &degenerative_nodes, int selected_node_id, bool edit_mode,
  const dag::Vector<bool> &hidden_nodes)
{
  TMatrix viewTm;
  d3d::gettm(TM_VIEW, viewTm);
  TMatrix4 projTm;
  d3d::gettm(TM_PROJ, &projTm);
  int vx, vy, vw, vh;
  float vzn, vzf;
  d3d::getview(vx, vy, vw, vh, vzn, vzf);
  ScreenSpaceParams ssParams;
  ssParams.viewItm = inverse(viewTm);
  ssParams.camPos = ssParams.viewItm.getcol(3);
  ssParams.pixelToWorld = (vh > 0 && projTm[1][1] > 1e-6f) ? 2.0f / (projTm[1][1] * vh) : 0.001f;

  begin_draw_cached_debug_lines();

  if (show_bbox)
    draw_cached_debug_box(collision_res.boundingBox, E3DCOLOR_MAKE(255, 255, 255, 255));

  const bool selectedNodeNotHidden = !hidden_nodes.empty() && selected_node_id >= 0 && !hidden_nodes[selected_node_id];
  const auto allNodes = collision_res.getAllNodes();
  int cnt = allNodes.size();
  for (int i = 0; i < cnt; i++)
  {
    const CollisionNode &node = allNodes[i];
    dag::ConstSpan<uint16_t> degenerativeIndices;
    if (show_degenerate_triangles)
    {
      const DegenerativeNodeData *degenerativeNode = eastl::find_if(degenerative_nodes.begin(), degenerative_nodes.end(),
        [&node](const DegenerativeNodeData &degenerative_node) { return degenerative_node.node == &node; });
      if (degenerativeNode != degenerative_nodes.end())
        degenerativeIndices = degenerativeNode->indices;
    }

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
    if (show_degenerate_triangles)
      customColor = E3DCOLOR_MAKE(0, 255, 0, alpha);

    if (node.type == COLLISION_NODE_TYPE_BOX)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 255, 255, alpha);
      BBox3 nodeBBox = collision_res.getNodeBBox(i);
      if (draw_solid)
        draw_debug_solid_cube(nodeBBox, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_box(nodeBBox, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_SPHERE)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 255, 0, alpha);
      BSphere3 nodeBSphere = collision_res.getNodeBSphere(i);
      if (draw_solid)
        draw_debug_solid_sphere(nodeBSphere.c, nodeBSphere.r, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_sphere(nodeBSphere.c, nodeBSphere.r, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR_MAKE(255, 0, 255, alpha);
      Capsule nodeCapsule;
      collision_res.getNodeCapsule(i, nodeCapsule);
      if (draw_solid)
        draw_debug_solid_capsule(nodeCapsule, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_capsule_w(nodeCapsule, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_MESH)
    {
      TMatrix nodeTm = collision_res.getNodeTm(i);
      if (node_tree)
        collision_res.getCollisionNodeTm(&node, TMatrix::IDENT, node_tree, nodeTm);

      set_cached_debug_lines_wtm(nodeTm);

      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]);
      color.a = alpha;
      draw_collision_mesh(collision_res, i, nodeTm, color, degenerativeIndices, draw_solid, show_face_orientation, ssParams);
    }
    else if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);

      E3DCOLOR color = customColor ? customColor.value() : E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]);

      const TMatrix &nTm = collision_res.getNodeTm(i);
      bool haveInvalidVertices = false;
      const float distEps = 1e-3f;
      vec4f nodeEps = v_splats(max(collision_res.getNodeBSphere(i).r * distEps, 1e-2f));
      dag::ConstSpan<plane3f> convexPlanes = collision_res.getNodeConvexPlanes(i);
      for (int k = 0; k < convexPlanes.size(); ++k)
      {
        const plane3f &plane = convexPlanes[k];
        Point3 fv0, fv1, fv2;
        const bool hasFace = collision_res.getNodeFaceVerts(i, k, fv0, fv1, fv2);
        collision_res.iterateNodeVerts(i, [&](int, vec4f vertex) {
          vec4f dist = v_plane_dist(plane, vertex);
          if (v_test_vec_x_gt(dist, nodeEps))
          {
            haveInvalidVertices = true;
            vec4f v_projPt = v_add(vertex, v_neg(v_mul(dist, plane)));
            Point3_vec4 vert, projPt;
            v_st(&vert.x, vertex);
            v_st(&projPt.x, v_projPt);
            draw_cached_debug_line(nTm * vert, nTm * projPt, E3DCOLOR_MAKE(255, 0, 0, 255));
            if (hasFace)
            {
              draw_cached_debug_line(nTm * fv0, nTm * fv1, E3DCOLOR_MAKE(255, 255, 0, 255));
              draw_cached_debug_line(nTm * fv1, nTm * fv2, E3DCOLOR_MAKE(255, 255, 0, 255));
              draw_cached_debug_line(nTm * fv2, nTm * fv0, E3DCOLOR_MAKE(255, 255, 0, 255));
              draw_cached_debug_line(nTm * fv0, nTm * projPt, E3DCOLOR_MAKE(255, 255, 0, 255));
              draw_cached_debug_line(nTm * fv1, nTm * projPt, E3DCOLOR_MAKE(255, 255, 0, 255));
              draw_cached_debug_line(nTm * fv2, nTm * projPt, E3DCOLOR_MAKE(255, 255, 0, 255));
            }
          }
        });
      }
      color.a = clamp(alpha, 0, haveInvalidVertices ? 40 : 128);
      if (draw_solid)
        draw_collision_mesh(collision_res, i, TMatrix::IDENT, color, {}, draw_solid, show_face_orientation, ssParams);
      else
      {
        Tab<Point3> vertList;
        vertList.reserve(collision_res.getNodeFaceCount(i) * 3);
        collision_res.iterateNodeFacesVerts(i, [&](int, vec4f v0, vec4f v1, vec4f v2) {
          Point3_vec4 p0, p1, p2;
          v_st(&p0.x, v0);
          v_st(&p1.x, v1);
          v_st(&p2.x, v2);
          vertList.push_back(nTm * p0);
          vertList.push_back(nTm * p1);
          vertList.push_back(nTm * p2);
        });
        draw_cached_debug_trilist(vertList.data(), vertList.size() / 3, color);
      }
    }
  }

  end_draw_cached_debug_lines();
}
