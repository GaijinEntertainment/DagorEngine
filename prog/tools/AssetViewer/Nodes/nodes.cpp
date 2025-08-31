// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodes.h"
#include <assets/asset.h>
#include <generic/dag_sort.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_geomTree.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>
#include <generic/dag_tab.h>
#include <propPanel/control/container.h>
#include <regExp/regExp.h>

#include <gui/dag_stdGuiRenderEx.h>

struct SkeletonNodesData
{
  dag::Index16 nodeId;
  Point2 pos;
  real z;
};

static Tab<dag::Index16> selectedNodes(midmem);


static const E3DCOLOR LINK_COLOR(255, 255, 0);
static const E3DCOLOR SEL_LINK_COLOR(255, 0, 0);

static const E3DCOLOR NODE_COLOR(125, 200, 0);
static const E3DCOLOR ROOT_NODE_COLOR(0, 200, 200);

static const E3DCOLOR SEL_NODE_COLOR(125, 0, 0);
static const E3DCOLOR CUR_SEL_NODE_COLOR(125, 125, 125);

enum
{
  PID_DRAW_LINKS = 5,
  PID_DRAW_ROOT_LINKS,
  PID_DRAW_NODE_ANOTATE,
  PID_SELECTED_NODE_NAME,
  PID_CIRCLE_RADIUS,
  PID_DRAW_NODE_TYPE,
  PID_AXIS_LEN,
  PID_SELECTED_NODE_TM_0,
  PID_SELECTED_NODE_TM_1,
  PID_SELECTED_NODE_TM_2,
  PID_SELECTED_NODE_TM_3,
  PID_MASK_NODES_GROUP,
  PID_MASK_NODES_UPDATE_BTN,
  PID_MASK_NODES_UNCHECK_ALL,
  PID_MASK_NODES,
  PID_MASK_NODES_LAST = PID_MASK_NODES + 64,
  PID_SELECT_NODES_GROUP,
  PID_SELECT_NODE0,
  PID_SELECT_NODE_LAST = PID_SELECT_NODE0 + 512,
  PID_SELECT_NODES_TREE,
};

static inline int cmpNodesData(const SkeletonNodesData *n1, const SkeletonNodesData *n2) { return n2->z - n1->z; }

static inline real getMinP3(const Point3 &p3, real clamp)
{
  real m = p3.x < p3.y ? p3.x : p3.y;
  m = m < p3.z ? m : p3.z;
  return m < clamp ? clamp : m;
}

static inline void renderNode(dag::Index16 node, const mat44f &node_wtm, const Point3 &pos,
  NodesPlugin::PresentationType presentation_type, real radius, real axis_len, int &select_node_ind, Point3 &last_pos,
  const E3DCOLOR &def_color = NODE_COLOR)
{
  bool curSel = ((select_node_ind > -1) && (selectedNodes[select_node_ind] == node));
  if (curSel)
  {
    select_node_ind--;
    draw_cached_debug_line(last_pos, pos, SEL_LINK_COLOR);
    last_pos = pos;
  }

  E3DCOLOR nodeColor = curSel ? (select_node_ind < 0 ? CUR_SEL_NODE_COLOR : SEL_NODE_COLOR) : def_color;
  if (presentation_type == NodesPlugin::FIG_TYPE_CIRCLE)
    draw_cached_debug_circle(pos, as_point3(&node_wtm.col2), as_point3(&node_wtm.col1), radius, nodeColor);
  else
    draw_debug_sph(pos, radius, nodeColor);
  if (axis_len > 0.f)
  {
    draw_cached_debug_line(pos, pos + as_point3(&node_wtm.col0) * axis_len, E3DCOLOR_MAKE(255, 0, 0, 255));
    draw_cached_debug_line(pos, pos + as_point3(&node_wtm.col1) * axis_len, E3DCOLOR_MAKE(0, 255, 0, 255));
    draw_cached_debug_line(pos, pos + as_point3(&node_wtm.col2) * axis_len, E3DCOLOR_MAKE(0, 0, 255, 255));
  }
}

static void renderLinks(Tab<bool> &nodes_is_enabled_array, const GeomNodeTree &tree, dag::Index16 node, const Point3 &p_pos,
  int &select_node_ind, bool draw_link_now = true)
{
  const int cnt = tree.getChildCount(node);
  for (int i = 0; i < cnt; i++)
  {
    const auto childNode = tree.getChildNodeIdx(node, i);

    bool curSel = ((select_node_ind > -1) && selectedNodes[select_node_ind] == tree.getChildNodeIdx(node, i));
    if (curSel)
      select_node_ind--;

    const bool needFilterNode = (childNode && !nodes_is_enabled_array[childNode.index()]);

    Point3 pos = tree.getNodeWposRelScalar(childNode);
    if (draw_link_now && !curSel && !needFilterNode)
      draw_cached_debug_line(p_pos, pos, LINK_COLOR);

    renderLinks(nodes_is_enabled_array, tree, childNode, pos, select_node_ind);
  }
}

void NodesPlugin::applyMask(int index, PropPanel::ContainerPropertyControl *panel)
{
  const DataBlock *maskBlk = nodeFilterMasksBlk.getBlock(index);
  const char *maskText = maskBlk->getStr("name", nullptr);
  const char *excludeText = maskBlk->getStr("exclude", nullptr);
  RegExp re;
  RegExp reExclude;
  const bool useExclude = excludeText && reExclude.compile(excludeText, "");
  if (maskText && re.compile(maskText, ""))
  {
    const unsigned nodeCount = geomNodeTree->nodeCount();
    const bool isNodeEnabled = panel->getBool(PID_MASK_NODES + index);
    for (dag::Index16 i(0), ie(nodeCount); i != ie; ++i)
    {
      const char *nodeName = geomNodeTree->getNodeName(i);
      if (re.test(nodeName))
      {
        if (useExclude && reExclude.test(nodeName))
          continue;
        nodesIsEnabledArray[i.index()] = isNodeEnabled;
      }
    }
  }
  else
    debug("bad regexp: '%s'", maskText);
}

void NodesPlugin::updateAllMaskFilters(PropPanel::ContainerPropertyControl *panel)
{
  const int blockCount = nodeFilterMasksBlk.blockCount();
  for (int i = 0; i < blockCount; ++i)
    applyMask(i, panel);
}

bool NodesPlugin::begin(DagorAsset *asset)
{
  const char *name = asset->getName();
  G_ASSERT(name);
  geomNodeTree = (GeomNodeTree *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), GeomNodeTreeGameResClassId);

  if (geomNodeTree)
  {
    v_mat44_ident(geomNodeTree->getRootTm());
    geomNodeTree->invalidateWtm();
    geomNodeTree->calcWtm();
    bbox.setempty();
    geomNodeTree->calcWorldBox(bbox);
    radius = getMinP3(bbox.width(), 0.5) * 0.05;

    if (EDITORCORE->getCurrentViewport())
    {
      geomNodeTree->calcWorldBox(bbox);

      EDITORCORE->getCurrentViewport()->zoomAndCenter(bbox);
    }

    const unsigned nodesCount = geomNodeTree->nodeCount();
    nodesIsEnabledArray.resize(nodesCount);
    for (unsigned i = 0; i < nodesCount; ++i)
      nodesIsEnabledArray[i] = true;
  }

  return true;
}

bool NodesPlugin::end()
{
  if (geomNodeTree)
  {
    release_game_resource((GameResource *)geomNodeTree);
    geomNodeTree = NULL;
  }

  selectedNodes.clear();

  return true;
}

void NodesPlugin::renderTransObjects()
{
  if (!geomNodeTree)
    return;

  begin_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(TMatrix::IDENT);


  if (geomNodeTree->empty())
    return;

  Point3 rootPos = geomNodeTree->getRootWposRelScalar();

  int selectNodeInd = selectedNodes.size() - 1;
  if (drawLinks)
  {
    if ((selectNodeInd > -1) && (selectedNodes[selectNodeInd].index() == 0))
      selectNodeInd--;
    renderLinks(nodesIsEnabledArray, *geomNodeTree, dag::Index16(0), rootPos, selectNodeInd, drawRootLinks);
  }

  selectNodeInd = selectedNodes.size() - 1;

  Point3 lastPos = rootPos;
  renderNode(dag::Index16(0), geomNodeTree->getRootWtmRel(), rootPos, presentationType, radius, axisLen, selectNodeInd, lastPos,
    ROOT_NODE_COLOR);

  for (dag::Index16 i(0), ie(geomNodeTree->nodeCount()); i != ie; ++i)
  {
    if (!nodesIsEnabledArray[i.index()])
      continue;

    renderNode(i, geomNodeTree->getNodeWtmRel(i), geomNodeTree->getNodeWposRelScalar(i), presentationType, radius, axisLen,
      selectNodeInd, lastPos);
  }

  end_draw_cached_debug_lines();
}

bool NodesPlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "skeleton") == 0; }

void NodesPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  panel.setEventHandler(this);

  panel.createCheckBox(PID_DRAW_LINKS, "draw links", drawLinks);
  panel.createCheckBox(PID_DRAW_ROOT_LINKS, "draw root links", drawRootLinks, drawLinks);
  panel.createCheckBox(PID_DRAW_NODE_ANOTATE, "draw node anotate", drawNodeAnotate);

  panel.createEditBox(PID_SELECTED_NODE_NAME, "selected node:", "", false);
  panel.createEditBox(PID_SELECTED_NODE_TM_0, "tm:", "", false);
  panel.createEditBox(PID_SELECTED_NODE_TM_1, "", "", false);
  panel.createEditBox(PID_SELECTED_NODE_TM_2, "", "", false);
  panel.createEditBox(PID_SELECTED_NODE_TM_3, "", "", false);
  panel.createEditFloat(PID_CIRCLE_RADIUS, "circle radius", radius);
  panel.createEditFloat(PID_AXIS_LEN, "axis len", axisLen);

  PropPanel::ContainerPropertyControl *rg = panel.createRadioGroup(PID_DRAW_NODE_TYPE, "node presentation type:");

  rg->createRadio(FIG_TYPE_CIRCLE, "[2D] circle");
  rg->createRadio(FIG_TYPE_SPHERE, "[3D] sphere");
  panel.setInt(PID_DRAW_NODE_TYPE, presentationType);

  PropPanel::ContainerPropertyControl &nodesFilterByMask = *panel.createGroup(PID_MASK_NODES_GROUP, "Filter nodes by name mask");

  nodesFilterByMask.createButton(PID_MASK_NODES_UPDATE_BTN, "Update nodes visibility");
  nodesFilterByMask.createButton(PID_MASK_NODES_UNCHECK_ALL, "Uncheck all");

  const int blockCount = nodeFilterMasksBlk.blockCount();
  for (int i = 0, c = min(blockCount, PID_MASK_NODES_LAST - PID_MASK_NODES); i < c; ++i)
  {
    const DataBlock *maskBlk = nodeFilterMasksBlk.getBlock(i);
    const char *maskText = maskBlk->getStr("name", "");
    const char *excludeText = maskBlk->getStr("exclude", nullptr);
    const String filterText = excludeText ? String(32, "%s (excl. '%s')", maskText, excludeText) : String(32, "%s", maskText);
    nodesFilterByMask.createCheckBox(PID_MASK_NODES + i, filterText.str(), maskBlk->getBool("isEnabled", false));
  }

  nodesFilterByMask.setBoolValue(true);

  const unsigned nodeCount = geomNodeTree->nodeCount();
  const unsigned maxNodeButtons = PID_SELECT_NODE_LAST - PID_SELECT_NODE0;
  PropPanel::ContainerPropertyControl &selectNodeByName =
    *panel.createGroup(PID_SELECT_NODES_GROUP, String(0, "Nodes List (%d nodes)", nodeCount));

  if (nodeCount > maxNodeButtons)
    selectNodeByName.createStatic(-1, "Too many nodes to display all");

  FastNameMap nodeNames;
  for (unsigned i = 0, count = min(nodeCount, maxNodeButtons); i < count; ++i)
    nodeNames.addString(geomNodeTree->getNodeName(dag::Index16(i)));

  iterate_names_in_lexical_order(nodeNames,
    [&](int i, const char *name) { selectNodeByName.createButton(PID_SELECT_NODE0 + i, name); });

  nodeTreePanel =
    panel.createMultiSelectTree(PID_SELECT_NODES_TREE, String(0, "Nodes Tree (%d nodes)", nodeCount), hdpi::_pxScaled(400));

  struct TreeNodeIndexAndLeaf
  {
    dag::Index16 parentNode;
    PropPanel::TLeafHandle parentLeaf;
  };
  Tab<TreeNodeIndexAndLeaf> nodes;

  PropPanel::TLeafHandle rootHandle = nodeTreePanel->createTreeLeaf(0, "::root", "");
  nodeTreePanel->setBool(rootHandle, true);
  nodeTreePanel->setUserData(rootHandle, 0);
  nodes.push_back({dag::Index16(0), rootHandle});

  for (unsigned i = 0; i < nodes.size(); ++i)
  {
    for (unsigned j = 0, c = geomNodeTree->getChildCount(nodes[i].parentNode); j < c; ++j)
    {
      const dag::Index16 node = geomNodeTree->getChildNodeIdx(nodes[i].parentNode, j);
      const char *nodeName = geomNodeTree->getNodeName(node);

      PropPanel::TLeafHandle leaf = nodeTreePanel->createTreeLeaf(nodes[i].parentLeaf, nodeName, "");
      nodeTreePanel->setBool(leaf, true);
      nodeTreePanel->setUserData(leaf, (void *)((intptr_t)((int)node)));
      nodes.push_back({node, leaf});
    }
  }
}


void NodesPlugin::onChange(int pid, PropPanel::ContainerPropertyControl *panel)
{
  if (pid == PID_DRAW_LINKS)
  {
    drawLinks = panel->getBool(pid);
    panel->setEnabledById(PID_DRAW_ROOT_LINKS, drawLinks);
  }
  else if (pid == PID_DRAW_NODE_ANOTATE)
  {
    drawNodeAnotate = panel->getBool(pid);
    repaintView();
  }
  else if (pid == PID_DRAW_ROOT_LINKS)
    drawRootLinks = panel->getBool(pid);
  else if (pid == PID_DRAW_NODE_TYPE && panel->getInt(pid) != PropPanel::RADIO_SELECT_NONE)
    presentationType = (PresentationType)panel->getInt(pid);
  else if (pid == PID_CIRCLE_RADIUS)
    radius = panel->getFloat(PID_CIRCLE_RADIUS);
  else if (pid == PID_AXIS_LEN)
    axisLen = panel->getFloat(PID_AXIS_LEN);
  else if (pid >= PID_MASK_NODES && pid <= PID_MASK_NODES_LAST)
    applyMask(pid - PID_MASK_NODES, panel);
  else if (pid == PID_SELECT_NODES_TREE)
  {
    dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
    nodeTreePanel->getSelectedLeafs(selectedLeafs);
    if (selectedLeafs.size() > 0)
      selectNode(dag::Index16((intptr_t)nodeTreePanel->getUserData(selectedLeafs.back())));
  }
}

dag::Index16 NodesPlugin::findObject(const Point3 &p, const Point3 &dir)
{
  const Point3 zero3(0, 0, 0);

  for (dag::Index16 i(0), ie(geomNodeTree->nodeCount()); i != ie; ++i)
  {
    Capsule capsule(zero3, zero3, radius);
    capsule.transform(geomNodeTree->getNodeWtmRel(i));

    real maxt = 1000.f;
    if (capsule.rayHit(p, dir, maxt))
      return i;
  }

  return {};
}

void NodesPlugin::selectNode(dag::Index16 node)
{
  selectedNodes.clear();

  const char *nodeName = "";
  TMatrix nodeTm = TMatrix::ZERO;

  if (node)
  {
    for (auto n = node; n; n = geomNodeTree->getParentNodeIdx(n))
      selectedNodes.push_back(n);
    nodeName = geomNodeTree->getNodeName(node);
    geomNodeTree->getNodeTmScalar(node, nodeTm);
  }

  if (PropPanel::ContainerPropertyControl *panel = getPluginPanel())
  {
    panel->setText(PID_SELECTED_NODE_NAME, nodeName);

    panel->setText(PID_SELECTED_NODE_TM_0, node ? String(32, "%@", nodeTm.getcol(0)) : "");
    panel->setText(PID_SELECTED_NODE_TM_1, node ? String(32, "%@", nodeTm.getcol(1)) : "");
    panel->setText(PID_SELECTED_NODE_TM_2, node ? String(32, "%@", nodeTm.getcol(2)) : "");
    panel->setText(PID_SELECTED_NODE_TM_3, node ? String(32, "%@", nodeTm.getcol(3)) : "");
  }

  if (!drawNodeAnotate)
    repaintView();
}

bool NodesPlugin::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (!inside)
    return false;

  Point3 dir, world;

  wnd->clientToWorld(Point2(x, y), world, dir);

  auto node = findObject(world, dir);
  selectNode(node);

  return node ? true : false;
}

struct NodePos
{
  int count;
  Point2 pos;
};

void NodesPlugin::drawObjects(IGenViewportWnd *wnd)
{

  drawInfo(wnd);
  if (!geomNodeTree || geomNodeTree->empty())
    return;

  Point2 pos;
  real z;

  static Tab<SkeletonNodesData> nodesData(midmem);
  nodesData.clear();

  static Tab<NodePos> nodesPos(midmem);
  nodesPos.clear();

  if (drawNodeAnotate)
  {
    for (dag::Index16 i(0), ie(geomNodeTree->nodeCount()); i != ie; ++i)
    {
      if (!nodesIsEnabledArray[i.index()])
        continue;

      if (!wnd->worldToClient(geomNodeTree->getNodeWposRelScalar(i), pos, &z) || (z < 0.001))
        continue;

      append_items(nodesData, 1);

      nodesData.back().pos = pos;
      nodesData.back().z = z;
      nodesData.back().nodeId = i;

      bool found = false;
      for (int j = 0; j < nodesPos.size(); ++j)
      {
        NodePos &p = nodesPos[j];
        if (fabs(p.pos.x - pos.x) < 0.1f && fabs(p.pos.y - pos.y) < 0.1f)
        {
          found = true;
          ++p.count;
          nodesData.back().pos.y += ((float)p.count) * 14.f;
          break;
        }
      }

      if (!found)
      {
        NodePos &p = nodesPos.push_back();
        p.count = 0;
        p.pos = pos;
      }
    }
  }
  else if (selectedNodes.size())
  {
    if (!wnd->worldToClient(geomNodeTree->getNodeWposRelScalar(selectedNodes[0]), pos, &z) || (z < 0.001))
      return;

    const char *name = geomNodeTree->getNodeName(selectedNodes[0]);
    if (!name)
      return;

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(pos.x + 1, pos.y + 1, name);
    StdGuiRender::set_color(COLOR_LTGREEN);
    StdGuiRender::draw_strf_to(pos.x, pos.y, name);

    return;
  }
  else
    return;

  if (!nodesData.size())
    return;

  sort(nodesData, &cmpNodesData);

  StdGuiRender::set_font(0);

  for (int i = nodesData.size() - 1; i >= 0; i--)
  {
    auto id = nodesData[i].nodeId;
    const char *name = geomNodeTree->getNodeName(id);
    if (!name)
      continue;

    Point2 &screen = nodesData[i].pos;

    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(screen.x + 1, screen.y + 1, name);
    StdGuiRender::set_color(COLOR_LTGREEN);
    StdGuiRender::draw_strf_to(screen.x, screen.y, name);
  }
}

bool NodesPlugin::getSelectionBox(BBox3 &box) const
{
  if (!geomNodeTree)
    return false;
  box = bbox;
  return true;
}

void NodesPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_MASK_NODES_UPDATE_BTN)
  {
    updateAllMaskFilters(panel);
  }
  else if (pcb_id == PID_MASK_NODES_UNCHECK_ALL)
  {
    const int blockCount = nodeFilterMasksBlk.blockCount();
    for (int i = 0, c = min(blockCount, PID_MASK_NODES_LAST - PID_MASK_NODES); i < c; ++i)
      panel->setBool(PID_MASK_NODES + i, false);

    updateAllMaskFilters(panel);
  }
  else if (pcb_id >= PID_SELECT_NODE0 && pcb_id < PID_SELECT_NODE_LAST)
  {
    dag::Index16 selectNodeId(pcb_id - PID_SELECT_NODE0);
    selectNode(selectNodeId);
  }
}
