// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "base_nodes_panel.h"

#include "plugin.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <imgui/imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace
{
enum
{
  PID_BASE_NODES_RELOAD = 11999,
  PID_BASE_NODES_TREE = 12000,
};

constexpr const char *BASE_NODE_DRAG_PAYLOAD = "DAGOR_BASE_NODE";
constexpr size_t BASE_NODE_NAME_BUF_SIZE = 128;
} // namespace

BaseNodesPanel::BaseNodesPanel(GraphEditorPlg &plg) : plugin(plg)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Base nodes");
  buildPanelContents();
}

BaseNodesPanel::~BaseNodesPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void BaseNodesPanel::updateImgui()
{
  if (panelWindow)
  {
    panelWindow->updateImgui();
  }
}

void BaseNodesPanel::refresh()
{
  if (!panelWindow)
  {
    return;
  }
  // Mirrors the PropertiesPanel rebuild pattern: clear() wipes every control on the panel,
  // dropping the previous tree's leaf-handle indices. Re-populate from scratch against the
  // plugin's current baseNodesBlk -- any newly added shader / subgraph file will surface.
  panelWindow->clear();
  leafToTemplateUid.clear();
  buildPanelContents();
}

void BaseNodesPanel::buildPanelContents()
{
  panelWindow->createButton(PID_BASE_NODES_RELOAD, "Reload base nodes");
  PropPanel::ContainerPropertyControl *tree = panelWindow->createTree(PID_BASE_NODES_TREE, "Nodes:", hdpi::_pxScaled(0));
  populateTree(tree);
  tree->setTreeDragHandler(this);
}

void BaseNodesPanel::onClick(int pcb_id, PropPanel::ContainerPropertyControl * /*panel*/)
{
  if (pcb_id == PID_BASE_NODES_RELOAD)
  {
    // plugin.reloadBaseNodes drops the cached descriptor state, re-runs the lazy loader
    // (which re-scans shader templates and subgraphsDir/*.subgraph.blk for subgraphs), then
    // calls back into this->refresh() to rebuild the tree against the fresh blk.
    plugin.reloadBaseNodes();
  }
}

void BaseNodesPanel::populateTree(PropPanel::ContainerPropertyControl *tree)
{
  const DataBlock &blk = plugin.getBaseNodesBlk();
  if (blk.blockCount() == 0)
  {
    tree->setTreeMessage("base_nodes.blk could not be loaded");
    return;
  }

  // Group nodes by category, preserving the order categories first appear in the BLK
  // (matches the order the JS editor's "Add node" menu shows them).
  eastl::vector<eastl::string> categoryOrder;
  eastl::vector<PropPanel::TLeafHandle> categoryLeaves;

  auto findCategory = [&](const char *name) -> int {
    for (int i = 0; i < (int)categoryOrder.size(); ++i)
    {
      if (categoryOrder[i] == name)
      {
        return i;
      }
    }
    return -1;
  };

  for (uint32_t i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *node = blk.getBlock(i);
    if (strcmp(node->getBlockName(), "node") != 0)
    {
      continue;
    }
    const char *category = node->getStr("category", "");
    const char *name = node->getStr("name", "");
    const char *templateUid = node->getStr("templateUid", "");
    if (!category[0] || !name[0])
    {
      continue;
    }
    if (!templateUid[0])
    {
      // Without a uid the descriptor is unreachable from drop handlers (uid is the
      // payload); skip rather than offer an undraggable leaf. add_template_uids.py
      // should have populated every node{} -- if this fires, the migrator was skipped.
      continue;
    }

    int idx = findCategory(category);
    if (idx < 0)
    {
      categoryOrder.emplace_back(category);
      categoryLeaves.push_back(tree->createTreeLeaf(nullptr, category, ""));
      idx = (int)categoryOrder.size() - 1;
    }

    PropPanel::TLeafHandle leaf = tree->createTreeLeaf(categoryLeaves[idx], name, "");
    leafToTemplateUid.emplace(leaf, eastl::string(templateUid));
  }

  for (PropPanel::TLeafHandle leaf : categoryLeaves)
  {
    tree->setExpanded(leaf, true);
  }
}

void BaseNodesPanel::onBeginDrag(PropPanel::TLeafHandle leaf)
{
  auto it = leafToTemplateUid.find(leaf);
  if (it == leafToTemplateUid.end())
  {
    return;
  }

  // Drag payload is the templateUid (stable across template renames). The drag
  // tooltip still shows the human-readable descriptor name via the tree's text.
  char buf[BASE_NODE_NAME_BUF_SIZE] = {};
  const eastl::string &uid = it->second;
  const size_t copy = eastl::min<size_t>(uid.size(), sizeof(buf) - 1);
  memcpy(buf, uid.c_str(), copy);

  ImGui::SetDragDropPayload(BASE_NODE_DRAG_PAYLOAD, buf, sizeof(buf));
  const char *displayName = "";
  if (const DataBlock *desc = plugin.findBaseNodeBlockByUid(uid.c_str()))
  {
    displayName = desc->getStr("name", "");
  }
  ImGui::TextUnformatted(displayName);
}
