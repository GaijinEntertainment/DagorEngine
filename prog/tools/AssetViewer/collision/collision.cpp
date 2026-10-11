// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collision.h"
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/collisionResourceBuilder.h>
#include <generic/dag_sort.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <imgui/imgui.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <math/dag_capsule.h>
#include <math/dag_geomTree.h>
#include <osApiWrappers/dag_clipboard.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/strUtil.h>
#include <EASTL/optional.h>
#include <memory/dag_framemem.h>
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

// The traceable / phys collidable pair only, as the tree prefix shows it; the fuse buckets nodes by their full
// behavior flags, so the pair is one phrase per node.
static const char *behavior_name(const CollisionNode &node)
{
  const bool traceable = node.checkBehaviorFlags(CollisionNode::TRACEABLE);
  const bool physCollidable = node.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
  if (traceable && physCollidable)
    return "traceable + phys collidable";
  return traceable ? "traceable" : physCollidable ? "phys collidable" : "no behavior";
}

// A node's size as both the panel line and the viewport label print it: a primitive's shape, else its face count.
static String node_geometry_text(const CollisionResource &res, const CollisionNode &node)
{
  switch (node.type)
  {
    case COLLISION_NODE_TYPE_BOX: return String("box");
    case COLLISION_NODE_TYPE_SPHERE: return String("sphere");
    case COLLISION_NODE_TYPE_CAPSULE: return String("capsule");
    default: return String(32, "%d faces", res.getNodeFaceCount(node.nodeIndex));
  }
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
  nodeTree.reset();
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
    InitCollisionResource(*asset, &collisionRes, nodeTree);
    nodesProcessing.init(asset, collisionRes, this);
    if (collisionRes)
      physMats.build(*collisionRes);
    curAsset = asset;
    if (showDegenerativeTriangles && collisionRes)
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
  // The panel content dies under the next current plugin, where onPropPanelClear cannot reach this one.
  nodesProcessing.setPropPanel(nullptr);
  selectedNodeId = -1;

  clearAssetStats();
  ReleaseCollisionResource(&collisionRes, nodeTree);
  destroy_it(modelEntity);
  modelAssetName.clear();
  curAsset = nullptr;
  degenerativeNodes.clear();
  physMats.clear();
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

void CollisionPlugin::renderTransObjects(BaseTexture *color_target)
{
  if (!collisionRes)
    return;

  if (showFaceOrientation && color_target)
  {
    updateFaceOrientationRenderDepth(color_target);
    d3d::set_render_target({faceOrientationRenderDepth.getTex2D(), 0, 0}, DepthAccess::RW, {{color_target, 0, 0}});
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
  }

  if (const ViewportWindow *vpw = static_cast<ViewportWindow *>(EDITORCORE->getRenderViewport()))
  {
    CollisionRenderOptions opts;
    opts.showBbox = showBbox;
    opts.colorByPhysMat = colorByPhysMat;
    opts.showPhysCollidable = showPhysCollidable;
    opts.showTraceable = showTraceable;
    opts.drawSolid = drawSolid;
    opts.showFaceOrientation = showFaceOrientation;
    opts.showDegenerativeTriangles = showDegenerativeTriangles;
    opts.degenerativeNodes = degenerativeNodes;
    opts.selectedNodeId = selectedNodeId;
    opts.editMode = nodesProcessing.editMode;
    opts.hiddenNodes = nodesProcessing.selectionNodesProcessing.hiddenNodes;
    opts.physMats = &physMats;
    RenderCollisionResource(*collisionRes, nodeTree.get(), vpw->getViewTm(), vpw->getProjTm(), opts);
  }
  nodesProcessing.renderNodes(selectedNodeId, drawSolid);

  fillAssetStats();
}

void CollisionPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  panel.setEventHandler(this);

  if (curAsset)
  {
    if (curAsset->props.getBool("collapseNodes", curAsset->props.getBool("collapseAndOptimize", false)))
      panel.createStatic(-1, "Collapse nodes: ON");
    if (const DagorAsset *pa = get_model_asset_from_collision_asset(*curAsset))
      panel.createStatic(-1, String(0, "Paired asset type: %s", pa->getTypeStr()));
  }
  panel.createCheckBox(PID_DRAW_NODE_ANOTATE, "draw node anotate", drawNodeAnotate);
  panel.createCheckBox(PID_SHOW_BBOX, "Show bounding box", showBbox);
  panel.createCheckBox(PID_COLOR_BY_PHYSMAT, "Color by phys material", colorByPhysMat);
  panel.setTooltipId(PID_COLOR_BY_PHYSMAT, "Off: one color per node. The phys material filter below stays active.");
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

  fillPhysMatPanel(panel);

  nodesProcessing.setPropPanel(&panel);
  nodesProcessing.fillCollisionInfoPanel();
  nodesProcessing.setPanelAfterReject();
  updateSelectionPanels(panel); // a rebuilt panel starts from the live selection
}

static String physmat_row_caption(int mat_id, uint32_t faces)
{
  String caption(64, "%s", PhysMatLegend::nameOf(mat_id));
  if (faces > 0)
    caption.aprintf(32, "  %u faces", faces);
  return caption;
}

// The row <-> control id encoding: the rows and the swatches each own one id window.
static int physmat_row_pid(int row) { return PID_PHYSMAT_ROW_BASE + row; }
static int physmat_swatch_pid(int row) { return PID_PHYSMAT_SWATCH_BASE + row; }
// The swatch square. A table cell holds exactly its column width, the padding falling outside, so the column is
// set to this - but fillPhysMatPanel runs on asset selection, outside the frame, so the draw refreshes it.
static float physmat_swatch_side() { return ImGui::GetFrameHeight(); }

static int physmat_row_from_pid(int pcb_id, int base, int row_count) // -1 outside the window
{
  const int row = pcb_id - base;
  return (unsigned)row < (unsigned)row_count ? row : -1;
}

void CollisionPlugin::fillPhysMatPanel(PropPanel::ContainerPropertyControl &panel)
{
  PropPanel::ContainerPropertyControl *group = panel.createGroup(PID_PHYSMAT_GROUP, "Phys materials");
  // A line splits evenly by default, which left the swatch half of it. Buttons carry no width, so they still share theirs.
  group->setUseFixedWidthColumnsWithZeroWidthStretch();
  group->createButton(PID_PHYSMAT_ALL, "All");
  group->createButton(PID_PHYSMAT_NONE, "None", true, false);
  group->createButton(PID_PHYSMAT_INVERT, "Invert", true, false);
  dag::ConstSpan<NodeMaterialFaces> rows = physMats.resourceMaterials();
  G_ASSERT(rows.size() <= PID_PHYSMAT_ROW_COUNT_MAX);
  // The caption and the enable state come from updatePhysMatPanel, which fillPropPanel runs next.
  for (int row = 0; row < rows.size(); ++row)
  {
    group->createCustomControlHolder(physmat_swatch_pid(row), this);
    group->setWidthById(physmat_swatch_pid(row), hdpi::_pxActual((int)physmat_swatch_side()));
    group->createCheckBox(physmat_row_pid(row), "", physMats.isVisible(rows[row].matId), true, false);
    group->setWidthById(physmat_row_pid(row), hdpi::Px::ZERO);
  }
}

// The stock color control dims under ImGui's disabled alpha and opens a picker when enabled, so the
// legend swatch is a plain rect in the face color, drawn here.
void CollisionPlugin::customControlUpdate(int id)
{
  dag::ConstSpan<NodeMaterialFaces> rows = physMats.resourceMaterials();
  const int row = physmat_row_from_pid(id, PID_PHYSMAT_SWATCH_BASE, rows.size());
  if (row < 0)
    return;
  if (PropPanel::ContainerPropertyControl *panel = getPluginPanel())
    panel->setWidthById(id, hdpi::_pxActual((int)physmat_swatch_side())); // the column reads it on the next frame
  const E3DCOLOR c = physMats.colorOf(rows[row].matId);
  const float side = min(physmat_swatch_side(), ImGui::GetContentRegionAvail().x);
  const ImVec2 p = ImGui::GetCursorScreenPos();
  // Faded while the viewport draws the node palette instead: the rows still filter, but the legend half is off.
  ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + side, p.y + side), IM_COL32(c.r, c.g, c.b, colorByPhysMat ? 255 : 64),
    ImGui::GetStyle().FrameRounding);
  ImGui::Dummy(ImVec2(side, side));
}

// With a node selected the rows show that node's counts and the materials it lacks are disabled;
// the filter state itself is one set for the resource and does not follow the selection.
void CollisionPlugin::updatePhysMatPanel(PropPanel::ContainerPropertyControl &panel)
{
  dag::ConstSpan<NodeMaterialFaces> rows = physMats.resourceMaterials();
  dag::ConstSpan<NodeMaterialFaces> nodeMats = physMats.nodeMaterials(selectedNodeId); // empty without a selection
  for (int row = 0; row < rows.size(); ++row)
  {
    uint32_t faces = rows[row].faces;
    bool enabled = true;
    if (selectedNodeId >= 0)
    {
      const NodeMaterialFaces *m =
        eastl::find_if(nodeMats.begin(), nodeMats.end(), [&](const NodeMaterialFaces &n) { return n.matId == rows[row].matId; });
      enabled = m != nodeMats.end();
      faces = enabled ? m->faces : 0;
    }
    panel.setCaption(physmat_row_pid(row), physmat_row_caption(rows[row].matId, faces).str());
    panel.setEnabledById(physmat_row_pid(row), enabled);
  }
}

String CollisionPlugin::selectedNodeInfoCaption() const
{
  if (!collisionRes || (unsigned)selectedNodeId >= collisionRes->getAllNodes().size())
    return String("Selected: none");
  const CollisionNode &node = collisionRes->getAllNodes()[selectedNodeId];
  return String(128, "Selected: %s: %s, %s", collisionRes->getNodeName(selectedNodeId), behavior_name(node),
    node_geometry_text(*collisionRes, node).str());
}

void CollisionPlugin::updateSelectionPanels(PropPanel::ContainerPropertyControl &panel)
{
  updatePhysMatPanel(panel);
  panel.setCaption(PID_SELECTED_NODE_INFO, selectedNodeInfoCaption());
}

void CollisionPlugin::setSelectedNodeId(int id)
{
  selectedNodeId = id;
  if (PropPanel::ContainerPropertyControl *panel = getPluginPanel())
    updateSelectionPanels(*panel);
}

// The panel is deleted right after this call; node processing must not keep the pointer, a viewport click may follow.
void CollisionPlugin::onPropPanelClear(PropPanel::ContainerPropertyControl &panel)
{
  IGenEditorPlugin::onPropPanelClear(panel);
  nodesProcessing.setPropPanel(nullptr);
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

    case PID_COLOR_BY_PHYSMAT:
      colorByPhysMat = panel->getBool(pcb_id);
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
      if (showDegenerativeTriangles && collisionRes)
        degenerativeNodes = collisionRes->getDegenerativeNodes(curAsset->getName());
      repaintView();
      break;

    case PID_PRINT_KDOP_LOG: printKdopLog(); break;
    case PID_NEXT_EDIT_NODE: setSelectedNodeId(-1); break;

    case PID_PHYSMAT_ALL:
    case PID_PHYSMAT_NONE:
    {
      const bool visible = pcb_id == PID_PHYSMAT_ALL;
      physMats.setAllVisible(visible);
      for (int row = 0; row < physMats.resourceMaterials().size(); ++row)
        panel->setBool(physmat_row_pid(row), visible);
      repaintView();
      break;
    }

    case PID_PHYSMAT_INVERT:
    {
      dag::ConstSpan<NodeMaterialFaces> rows = physMats.resourceMaterials();
      for (int row = 0; row < rows.size(); ++row)
      {
        const bool visible = !physMats.isVisible(rows[row].matId);
        physMats.setRowVisible(row, visible);
        panel->setBool(physmat_row_pid(row), visible);
      }
      repaintView();
      break;
    }

    default:
      if (const int row = physmat_row_from_pid(pcb_id, PID_PHYSMAT_ROW_BASE, physMats.resourceMaterials().size()); row >= 0)
      {
        physMats.setRowVisible(row, panel->getBool(pcb_id));
        repaintView();
      }
      break;
  }
  nodesProcessing.onClick(pcb_id);
  // Refresh after every click: Save and Cancel of an edit rebuild the "Collision nodes" group with an empty static
  // text, and the refresh is a no-op otherwise.
  updateSelectionPanels(*panel);
}

void CollisionPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_SELECTABLE_NODES_LIST)
    setSelectedNodeId(-1);
  else if (pcb_id == PID_COLLISION_NODES_TREE)
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_COLLISION_NODES_TREE)->getContainer();
    PropPanel::TLeafHandle leaf = tree->getSelLeaf();
    setSelectedNodeId(getNodeIdx(tree, leaf));
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

static bool trace_ray_through_nodes(CollisionResource *collision_res, const PhysMatLegend &phys_mats, IGenViewportWnd *wnd, int x,
  int y, CollResIntersectionsType &intersected_nodes_list)
{
  CollResIntersectionsType physIntersectedNodesList;
  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);
  const float t = 1000.f;
  collision_res->traceRay(TMatrix::IDENT, NULL, world, dir, t, intersected_nodes_list, false, CollisionNode::TRACEABLE);
  collision_res->traceRay(TMatrix::IDENT, NULL, world, dir, t, physIntersectedNodesList, false, CollisionNode::PHYS_COLLIDABLE);
  intersected_nodes_list.insert(intersected_nodes_list.end(), physIntersectedNodesList.begin(), physIntersectedNodesList.end());
  // A hit on an unchecked material is not drawn, so it must not select its node either: drop it before the
  // per-node dedup below would keep it as the node's one hit.
  intersected_nodes_list.erase(
    eastl::remove_if(intersected_nodes_list.begin(), intersected_nodes_list.end(),
      [&](const IntersectedNode &hit) { return !phys_mats.isVisible(collision_res->getHitPhysMat(hit.triRef)); }),
    intersected_nodes_list.end());
  stlsort::sort_branchless(intersected_nodes_list.begin(), intersected_nodes_list.end());

  auto compare = [](const IntersectedNode &lhs, const IntersectedNode &rhs) {
    return tri_ref::nodeIndex(lhs.triRef) == tri_ref::nodeIndex(rhs.triRef);
  };

  intersected_nodes_list.erase(eastl::unique(intersected_nodes_list.begin(), intersected_nodes_list.end(), compare),
    intersected_nodes_list.end());
  return !intersected_nodes_list.empty();
}

bool CollisionPlugin::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (!collisionRes)
    return false;

  CollResIntersectionsType sortedIntersectedNodesList;
  if (trace_ray_through_nodes(collisionRes, physMats, wnd, x, y, sortedIntersectedNodesList))
  {
    dag::ConstSpan<CollisionNode> nodes = collisionRes->getAllNodes();
    for (int i = 0; i < (int)sortedIntersectedNodesList.size(); ++i)
    {
      const int candidateId = (int)tri_ref::nodeIndex(sortedIntersectedNodesList[i].triRef);
      dag::Vector<bool> &hiddenNodes = nodesProcessing.selectionNodesProcessing.hiddenNodes;
      const bool isVisible =
        getNodeVisibility(nodes[candidateId], showPhysCollidable, showTraceable) != CollisionNodeVisibility::INVISIBLE;
      if (isVisible && !hiddenNodes[candidateId])
      {
        if (selectedNodeId != candidateId)
        {
          setSelectedNodeId(candidateId);
          nodesProcessing.selectNode(collisionRes->getNodeName(selectedNodeId), key_modif == wingw::M_CTRL);
          return false;
        }
      }
    }
  }
  setSelectedNodeId(-1);
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

    // a SPHERE node's bounding sphere already carries the placement; every other type stays
    // stored-frame (box bspheres were never baked) and needs getNodeTm composed
    const CollisionNode &annNode = allNodes[i];
    const BSphere3 annBSphere = collisionRes->getNodeBSphere(i);
    Point3 pos3 = annNode.type == COLLISION_NODE_TYPE_SPHERE ? annBSphere.c : collisionRes->getNodeTm(i) * annBSphere.c;

    if (!wnd->worldToClient(pos3, pos, &z) || (z < 0.001f))
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
    if (!name || (selectedNodeId >= 0 && selectedNodeId != id) || !physMats.hasVisibleGeometry(id))
      continue;

    Point2 &screen = nodesData[i].pos;

    String selectedName(128, name);
    if (selectedNodeId >= 0)
    {
      selectedName.aprintf(64, " | %s |", node_geometry_text(*collisionRes, node).str());
      dag::ConstSpan<NodeMaterialFaces> mats = physMats.nodeMaterials(id);
      for (int i = 0; i < mats.size(); ++i)
      {
        selectedName.aprintf(64, "%s %s", i ? "," : "", PhysMatLegend::nameOf(mats[i].matId));
        if (mats[i].faces > 0)
          selectedName.aprintf(16, " %u", mats[i].faces);
      }
      if (node.behaviorFlags >> 3)
      {
        selectedName.aprintf(128, " Flags : %s %s %s %s",
          (node.behaviorFlags & CollisionNode::FLAG_ALLOW_HOLE) ? "" : "noOverlapHoles |",
          (node.behaviorFlags & CollisionNode::FLAG_CUT_REQUIRED) ? "noOverlapHolesIfNoCut |" : "",
          (node.behaviorFlags & CollisionNode::FLAG_CHECK_SIDE) ? "noOverlapEdgeHoles |" : "",
          (node.behaviorFlags & CollisionNode::FLAG_ALLOW_BULLET_DECAL) ? "" : "noBullets |");
      }
      // The flags block above leaves trailing separators (a pipe, or spaces when its last slot was empty): trim them.
      while (selectedName.length() > 0 && strchr(" |", selectedName[selectedName.length() - 1]))
        selectedName.chop(1);
      selectedName.aprintf(32, " | %s", behavior_name(node));
    }
    const bool isHidden = nodesProcessing.selectionNodesProcessing.hiddenNodes[id];
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

void CollisionPlugin::updateFaceOrientationRenderDepth(BaseTexture *color_target)
{
  TextureInfo targetInfo;
  color_target->getinfo(targetInfo);
  const int targetW = targetInfo.w, targetH = targetInfo.h;

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
  if (leaf && collisionRes)
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

// The display variant without a skeleton bakes every mirrored placement: a rebuild of the
// resource through the builder (the game resource itself stays as loaded).
static void reset_nodes_tm(CollisionResource **collision_res, GeomNodeTree *node_tree, const char *asset_name)
{
  if (node_tree)
    return;
  CollisionResourceBuilder builder;
  builder.fromResource(**collision_res);
  if (!builder.bakeMirroredNodes())
    return;
  CollisionResource *baked = builder.build(asset_name);
  del_it(*collision_res);
  *collision_res = baked;
}

void InitCollisionResource(const DagorAsset &asset, CollisionResource **collision_res, GeomNodeTreeUniquePtr &node_tree)
{
  *collision_res = (CollisionResource *)::get_game_resource_ex(asset.getName(), CollisionGameResClassId);
  if (*collision_res)
  {
    auto *clone = (*collision_res)->deepCopy();
    ::release_game_resource_ex(*collision_res, CollisionGameResClassId);
    *collision_res = clone;
  }
  else // resource is missing or fails to build, don't crash on such
  {
    DAEDITOR3.conError("cannot load/build collision asset: %s", asset.getName());
    return;
  }

  const DataBlock &props = asset.getProfileTargetProps(_MAKE4C('PC'), NULL);
  if (const char *skeletonName = props.getStr("ref_skeleton", nullptr))
  {
    node_tree = getSkeleton(asset.getMgr(), skeletonName, ::get_app().getConsole());
    if (node_tree)
    {
      node_tree->invalidateWtm();
      node_tree->calcWtm();
      (*collision_res)->initializeWithGeomNodeTree(*node_tree);
    }
  }
  reset_nodes_tm(collision_res, node_tree.get(), asset.getName());
}


void ReleaseCollisionResource(CollisionResource **collision_res, GeomNodeTreeUniquePtr &node_tree)
{
  del_it(*collision_res);
  node_tree.reset();
}

struct ScreenSpaceParams
{
  TMatrix viewItm;
  Point3 camPos;
  float pixelToWorld;
};

// A node's draw color; with perFaceMaterial (mesh nodes only) each face takes its own material's color at color's alpha.
struct NodeColoring
{
  E3DCOLOR color;
  bool perFaceMaterial;
};

// opts.physMats filters the faces; the degenerate markers stay unfiltered, they are a diagnostic.
static void draw_collision_mesh(const CollisionResource &collision_res, int node_id, const TMatrix &tm, const NodeColoring &coloring,
  dag::ConstSpan<uint32_t> degenerative_indices, const CollisionRenderOptions &opts, const ScreenSpaceParams &params)
{
  // Every face of a material takes the same color: keep the last one.
  int lastMatId = PHYSMAT_INVALID - 1;
  E3DCOLOR lastColor;
  auto faceColor = [&](int mat_id) {
    if (mat_id != lastMatId)
    {
      lastMatId = mat_id;
      lastColor = coloring.perFaceMaterial ? opts.physMats->colorOf(mat_id) : coloring.color;
      lastColor.a = coloring.color.a;
    }
    return lastColor;
  };
  auto matVisible = [&](int mat_id) { return !opts.physMats || opts.physMats->isVisible(mat_id); };
  const int faceCount = collision_res.getNodeFaceCount(node_id);
  const int vertCount = collision_res.getNodeVertCount(node_id);
  // Every pass below addresses the node's verts by face index, so materialize them once, in frame memory.
  dag::Vector<Point3_vec4, framemem_allocator> verts(vertCount);
  collision_res.iterateNodeVerts(node_id, [&](int vi, vec4f v) { v_st(&verts[vi].x, v); });
  if (verts.empty())
    return;

  if (opts.showFaceOrientation || opts.drawSolid)
  {
    // draw_debug_solid_mesh uploads a uint16 index buffer, so it can only represent a node with up to
    // 65536 node-local verts; a larger node (heavy per-node-BLAS dup) would truncate its face indices.
    // Skip the solid fill for it -- the wireframe below still renders. Editor debug viz only.
    if (faceCount > 0 && vertCount <= 65536)
    {
      dag::Vector<uint16_t, framemem_allocator> idx;
      auto drawSolid = [&](const uint16_t *indices, int faces, E3DCOLOR c, bool shaded) {
        if (faces > 0)
          draw_debug_solid_mesh(indices, faces, &verts[0].x, sizeof(Point3_vec4), vertCount, tm, c, shaded, DrawSolidMeshCull::FLIP);
      };
      if (opts.showFaceOrientation || !coloring.perFaceMaterial)
      {
        idx.reserve(faceCount * 3);
        collision_res.iterateNodeFacesByMaterial(node_id, matVisible, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
          idx.push_back((uint16_t)i0);
          idx.push_back((uint16_t)i1);
          idx.push_back((uint16_t)i2);
        });
        if (opts.showFaceOrientation)
        {
          ShaderGlobal::set_int(debug_ri_face_orientationVarId, 1);
          drawSolid(idx.data(), (int)idx.size() / 3, E3DCOLOR(255, 255, 255), true);
          ShaderGlobal::set_int(debug_ri_face_orientationVarId, 0);
        }
        else
          drawSolid(idx.data(), (int)idx.size() / 3, coloring.color, false);
      }
      else
      {
        // The solid draw takes one color: the visible faces fill one buffer sorted by material, one draw per material range.
        struct MaterialRange
        {
          int matId;
          uint32_t faces, start, cursor;
        };
        dag::Vector<MaterialRange, framemem_allocator> ranges;
        ranges.reserve(collision_res.getNodePhysMatCount(node_id));
        auto rangeOf = [&](int mat) {
          return eastl::find_if(ranges.begin(), ranges.end(), [mat](const MaterialRange &r) { return r.matId == mat; });
        };
        collision_res.iterateNodeFacesWithLeafMaterial(node_id, [&](int, uint32_t, uint32_t, uint32_t, int mat) {
          if (!matVisible(mat))
            return;
          MaterialRange *r = rangeOf(mat);
          if (r == ranges.end())
            ranges.push_back({mat, 1, 0, 0});
          else
            r->faces++;
        });
        uint32_t visibleFaces = 0;
        for (MaterialRange &r : ranges)
        {
          r.start = r.cursor = visibleFaces;
          visibleFaces += r.faces;
        }
        idx.resize(visibleFaces * 3);
        collision_res.iterateNodeFacesWithLeafMaterial(node_id, [&](int, uint32_t i0, uint32_t i1, uint32_t i2, int mat) {
          if (!matVisible(mat))
            return;
          uint16_t *dst = idx.data() + rangeOf(mat)->cursor++ * 3;
          dst[0] = (uint16_t)i0;
          dst[1] = (uint16_t)i1;
          dst[2] = (uint16_t)i2;
        });
        for (const MaterialRange &r : ranges)
          drawSolid(idx.data() + r.start * 3, (int)r.faces, faceColor(r.matId), false);
      }
    }
  }

  collision_res.iterateNodeFacesWithLeafMaterial(node_id, [&](int, uint32_t i0, uint32_t i1, uint32_t i2, int mat) {
    if (!matVisible(mat))
      return;
    const E3DCOLOR c = faceColor(mat);
    draw_cached_debug_line(verts[i0], verts[i1], c);
    draw_cached_debug_line(verts[i1], verts[i2], c);
    draw_cached_debug_line(verts[i2], verts[i0], c);
  });

  if (degenerative_indices.empty())
    return;

  // Flush mesh wireframe into its own draw call so degenerate markers always render on top,
  // preventing mesh edges from occluding them.
  flush_cached_debug_lines();
  const E3DCOLOR lineColor = E3DCOLOR(255, 0, 0, coloring.color.a);
  const E3DCOLOR markerColor = E3DCOLOR(255, 255, 0, coloring.color.a);

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

  for (int i = 0; i + 2 < degenerative_indices.size(); i += 3)
  {
    const Point3 &p1 = verts[degenerative_indices[i + 0]];
    const Point3 &p2 = verts[degenerative_indices[i + 1]];
    const Point3 &p3 = verts[degenerative_indices[i + 2]];
    draw_thick_line(p1, p2);
    draw_thick_line(p2, p3);
    draw_thick_line(p3, p1);
  }

  for (int i = 0; i + 2 < degenerative_indices.size(); i += 3)
  {
    draw_screen_marker(verts[degenerative_indices[i + 0]]);
    draw_screen_marker(verts[degenerative_indices[i + 1]]);
    draw_screen_marker(verts[degenerative_indices[i + 2]]);
  }
}

void RenderCollisionResource(const CollisionResource &collision_res, GeomNodeTree *node_tree, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, const CollisionRenderOptions &opts)
{
  int vx, vy, vw, vh;
  float vzn, vzf;
  d3d::getview(vx, vy, vw, vh, vzn, vzf);
  ScreenSpaceParams ssParams;
  ssParams.viewItm = inverse(view_tm);
  ssParams.camPos = ssParams.viewItm.getcol(3);
  ssParams.pixelToWorld = (vh > 0 && proj_tm[1][1] > 1e-6f) ? 2.0f / (proj_tm[1][1] * vh) : 0.001f;

  begin_draw_cached_debug_lines();

  if (opts.showBbox)
    draw_cached_debug_box(collision_res.boundingBox, E3DCOLOR_MAKE(255, 255, 255, 255));

  // A selected node with nothing to show (eye or material filter) gives no focus, so the rest is not dimmed against it.
  const bool selectedNodeShown = !opts.hiddenNodes.empty() && opts.selectedNodeId >= 0 && !opts.hiddenNodes[opts.selectedNodeId] &&
                                 (!opts.physMats || opts.physMats->hasVisibleGeometry(opts.selectedNodeId));
  const auto allNodes = collision_res.getAllNodes();
  int cnt = allNodes.size();
  for (int i = 0; i < cnt; i++)
  {
    const CollisionNode &node = allNodes[i];
    dag::ConstSpan<uint32_t> degenerativeIndices;
    if (opts.showDegenerativeTriangles)
    {
      const DegenerativeNodeData *degenerativeNode = eastl::find_if(opts.degenerativeNodes.begin(), opts.degenerativeNodes.end(),
        [&node](const DegenerativeNodeData &degenerative_node) { return degenerative_node.node == &node; });
      if (degenerativeNode != opts.degenerativeNodes.end())
        degenerativeIndices = degenerativeNode->indices;
    }

    const CollisionNodeVisibility visibility = getNodeVisibility(node, opts.showPhysCollidable, opts.showTraceable);
    const bool isVisible = visibility == CollisionNodeVisibility::INVISIBLE;
    const bool isHidden = !opts.hiddenNodes.empty() && opts.hiddenNodes[i];
    if (isVisible || isHidden)
      continue;
    // Nothing of the node passes the material filter: skip it whole, markers included, so drawn and selectable agree.
    if (opts.physMats && !opts.physMats->hasVisibleGeometry(i))
      continue;

    eastl::optional<E3DCOLOR> customColor;
    if (visibility == CollisionNodeVisibility::SHOW_AS_PHYS_COLLIDABLE)
      customColor = E3DCOLOR(phys_collidable_color);
    else if (visibility == CollisionNodeVisibility::SHOW_AS_TRACEABLE)
      customColor = E3DCOLOR(traceable_color);

    int alpha = 255;
    const bool isNotSelectedNode = i != opts.selectedNodeId;
    const bool hasSelectedNode = opts.selectedNodeId >= 0 && selectedNodeShown;
    if (isNotSelectedNode && (hasSelectedNode || opts.editMode))
    {
      alpha = 30;
      if (customColor)
        customColor.value().a = alpha;
    }
    if (opts.showDegenerativeTriangles)
      customColor = E3DCOLOR_MAKE(0, 255, 0, alpha);

    // The one home of the color rule: a mode override wins, then the material color, then the legacy color.
    // Only a mesh node holds several materials, so only it colors per face.
    auto nodeColoring = [&](E3DCOLOR legacy, bool per_face) -> NodeColoring {
      if (customColor)
        return {customColor.value(), false};
      if (!opts.colorByPhysMat || !opts.physMats)
        return {legacy, false};
      if (per_face)
        return {legacy, true}; // the faces take their own material color; only legacy.a is read
      E3DCOLOR c = opts.physMats->colorOf(collision_res.getNodePhysMatId(i, 0));
      c.a = legacy.a;
      return {c, false};
    };

    if (node.type == COLLISION_NODE_TYPE_BOX)
    {
      E3DCOLOR color = nodeColoring(E3DCOLOR_MAKE(255, 255, 255, alpha), false).color;
      BBox3 nodeBBox = collision_res.getNodeBBox(i);
      if (opts.drawSolid)
        draw_debug_solid_cube(nodeBBox, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_box(nodeBBox, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_SPHERE)
    {
      E3DCOLOR color = nodeColoring(E3DCOLOR_MAKE(255, 255, 0, alpha), false).color;
      BSphere3 nodeBSphere = collision_res.getNodeBSphere(i);
      if (opts.drawSolid)
        draw_debug_solid_sphere(nodeBSphere.c, nodeBSphere.r, TMatrix::IDENT, color);

      set_cached_debug_lines_wtm(TMatrix::IDENT);
      draw_cached_debug_sphere(nodeBSphere.c, nodeBSphere.r, color);
    }
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      E3DCOLOR color = nodeColoring(E3DCOLOR_MAKE(255, 0, 255, alpha), false).color;
      Capsule nodeCapsule;
      // Fails for the zero-vert marker and leaves the capsule unassigned: nothing to draw.
      if (collision_res.getNodeCapsule(i, nodeCapsule))
      {
        if (opts.drawSolid)
          draw_debug_solid_capsule(nodeCapsule, TMatrix::IDENT, color);

        set_cached_debug_lines_wtm(TMatrix::IDENT);
        draw_cached_debug_capsule_w(nodeCapsule, color);
      }
    }
    else if (node.type == COLLISION_NODE_TYPE_MESH)
    {
      TMatrix nodeTm = collision_res.getNodeTm(i);
      if (node_tree)
        collision_res.getCollisionNodeTm(&node, TMatrix::IDENT, node_tree, nodeTm);

      set_cached_debug_lines_wtm(nodeTm);

      E3DCOLOR legacy = E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]);
      legacy.a = alpha;
      draw_collision_mesh(collision_res, i, nodeTm, nodeColoring(legacy, true), degenerativeIndices, opts, ssParams);
    }
    else if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);

      NodeColoring coloring = nodeColoring(E3DCOLOR(colors[i % (sizeof(colors) / sizeof(colors[0]))]), false);

      const TMatrix &nTm = collision_res.getNodeTm(i);
      bool haveInvalidVertices = false;
      const float distEps = 1e-3f;
      const float nodeEpsF = max(collision_res.getNodeBSphere(i).r * distEps, 1e-2f);
      vec4f nodeEps = v_splats(nodeEpsF);
      dag::ConstSpan<plane3f> convexPlanes = collision_res.getNodeConvexPlanes(i);
      const int nodeFaceCnt = collision_res.getNodeFaceCount(i);
      // Materialise the node's faces once (a getNodeFaceVerts() per face is a full per-node BLAS walk).
      Tab<Point3> faceVerts;
      faceVerts.resize(nodeFaceCnt * 3);
      collision_res.iterateNodeFacesVerts(i, [&](int fi, vec4f v0, vec4f v1, vec4f v2) {
        Point3_vec4 a, b, c;
        v_st(&a.x, v0);
        v_st(&b.x, v1);
        v_st(&c.x, v2);
        faceVerts[fi * 3 + 0] = a;
        faceVerts[fi * 3 + 1] = b;
        faceVerts[fi * 3 + 2] = c;
      });
      // Per-node-BLAS chunking reorders a convex node's faces (meshopt), so face index no longer equals
      // plane index. Build the plane->face map once by geometry (the face whose verts sit closest to the
      // plane), so the per-plane overlay is an O(1) lookup and pairs by best fit, not first-within-eps
      // (which could mis-match near-coplanar hull faces).
      Tab<int> faceOfPlane;
      faceOfPlane.resize(convexPlanes.size());
      mem_set_ff(faceOfPlane);
      for (int f = 0; f < nodeFaceCnt; ++f)
      {
        const Point3 &a = faceVerts[f * 3], &b = faceVerts[f * 3 + 1], &c = faceVerts[f * 3 + 2];
        int best = -1;
        float bestErr = nodeEpsF;
        for (int k = 0; k < convexPlanes.size(); ++k)
        {
          const plane3f &pl = convexPlanes[k];
          vec4f err = v_max(v_max(v_abs(v_plane_dist(pl, v_ldu_p3(&a.x))), v_abs(v_plane_dist(pl, v_ldu_p3(&b.x)))),
            v_abs(v_plane_dist(pl, v_ldu_p3(&c.x))));
          float e = v_extract_x(err);
          if (e < bestErr)
          {
            bestErr = e;
            best = k;
          }
        }
        if (best >= 0)
          faceOfPlane[best] = f;
      }
      for (int k = 0; k < convexPlanes.size(); ++k)
      {
        const plane3f &plane = convexPlanes[k];
        Point3 fv0, fv1, fv2;
        const bool hasFace = faceOfPlane[k] >= 0;
        if (hasFace)
        {
          const int f = faceOfPlane[k];
          fv0 = faceVerts[f * 3];
          fv1 = faceVerts[f * 3 + 1];
          fv2 = faceVerts[f * 3 + 2];
        }
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
      coloring.color.a = clamp(alpha, 0, haveInvalidVertices ? 40 : 128);
      if (opts.drawSolid)
        draw_collision_mesh(collision_res, i, TMatrix::IDENT, coloring, {}, opts, ssParams);
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
        draw_cached_debug_trilist(vertList.data(), vertList.size() / 3, coloring.color);
      }
    }
  }

  end_draw_cached_debug_lines();
}
