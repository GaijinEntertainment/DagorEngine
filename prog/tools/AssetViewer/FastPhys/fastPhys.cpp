// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"

#include <animChar/dag_animCharacter2.h>

#include <assets/asset.h>
#include <de3_interface.h>
#include <generic/dag_initOnDemand.h>

#include <util/dag_fastPtrList.h>

#include <debug/dag_debug.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>

#include "fastPhys.h"
#include "fastPhysObject.h"
#include "fastPhysPanel.h"

//------------------------------------------------------------------

enum
{
  WINDOW_TYPE_ACTIONS_TREE = 0x6080,
};


//------------------------------------------------------------------

FastPhysPlugin::FastPhysPlugin() :
  mFastPhysEditor(*this),
  mActionTree(NULL),
  mActionTreeCB(NULL),
  propPanel(NULL),
  mAsset(NULL),
  mWindVel(0.0f, 0.0f, 0.0f),
  mWindTurb(0.0f),
  mWindPower(0.0f),
  mRespondsToWind(false)
{}


FastPhysPlugin::~FastPhysPlugin() {}


bool FastPhysPlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "fastPhys") == 0; }


void FastPhysPlugin::addTreeAction(PropPanel::TLeafHandle parent, FpdAction *action)
{
  G_ASSERT(action);

  FpdObject *object = action->getObject();
  String name;
  if (object)
    name.printf(256, "%s: %s", action->actionName.str(), object->getName());
  else
    name = action->actionName.str();

  PropPanel::TLeafHandle cur = mActionTree->addItem(name.str(), nullptr, parent, action);

  FpdContainerAction *c_action = FastPhysEditor::getContainerAction(action);

  if (c_action)
  {
    const PtrTab<FpdAction> &subact = c_action->getSubActions();

    for (int i = 0; i < subact.size(); ++i)
      addTreeAction(cur, subact[i]);
  }
}


void FastPhysPlugin::clearTree()
{
  if (!mActionTree)
    return;

  mActionTree->clear();
}

void FastPhysPlugin::refillActionTree()
{
  if (!mActionTree)
    return;

  clearTree();

  PropPanel::TLeafHandle root = mActionTree->addItem("Actions", nullptr, mActionTree->getRoot(), NULL);

  if (mFastPhysEditor.initAction)
    addTreeAction(root, mFastPhysEditor.initAction);

  if (mFastPhysEditor.updateAction)
    addTreeAction(root, mFastPhysEditor.updateAction);

  mActionTree->expand(mActionTree->getRoot());
}


bool FastPhysPlugin::begin(DagorAsset *asset)
{
  mAsset = asset;
  IWndManager &manager = getWndManager();
  manager.registerWindowHandler(this);

  propPanel = new FPPanel(*this);
  manager.setWindowType(nullptr, WINDOW_TYPE_ACTIONS_TREE);

  mFastPhysEditor.initUi(GUI_PLUGIN_TOOLBAR_ID);

  G_ASSERT(asset && "Aseet is empty");
  mFastPhysEditor.load(*asset);
  refillActionTree();
  fillPluginPanel();

  return true;
}

bool FastPhysPlugin::end()
{
  mFastPhysEditor.saveResource();
  clearTree();
  mAsset = NULL;

  if (mFastPhysEditor.isSimulationActive)
    mFastPhysEditor.stopSimulation();
  mFastPhysEditor.clearAll();
  mFastPhysEditor.closeUi();
  mFastPhysEditor.updateGizmo();
  del_it(propPanel);

  IWndManager &manager = getWndManager();
  manager.removeWindow(mActionTree);
  manager.unregisterWindowHandler(this);

  return true;
}


void FastPhysPlugin::registerEditorCommands(IEditorCommandSystem &command_system)
{
  mFastPhysEditor.registerEditorCommands(command_system);
}


void FastPhysPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *EDITORCORE->getWndManager();
  mFastPhysEditor.registerViewportAccelerators(wndManager);
}


void FastPhysPlugin::handleViewportAcceleratorCommand([[maybe_unused]] IGenViewportWnd &wnd, [[maybe_unused]] unsigned id)
{
  mFastPhysEditor.onClick(id, getPluginPanel());
}


void *FastPhysPlugin::onWmCreateWindow(int type)
{
  switch (type)
  {
    case WINDOW_TYPE_ACTIONS_TREE:
    {
      if (mActionTree)
        return nullptr;

      mActionTreeCB = new ActionsTreeCB(*this);
      mActionTree =
        new PropPanel::TreeBaseWindow(mActionTreeCB, nullptr, 0, 0, hdpi::_pxActual(0), hdpi::_pxActual(0), "", false, false, true);

      return mActionTree;
    }
  }

  return nullptr;
}


bool FastPhysPlugin::onWmDestroyWindow(void *window)
{
  if (mActionTree && mActionTree == window)
  {
    del_it(mActionTree);
    del_it(mActionTreeCB);

    return true;
  }

  return false;
}

/*
void FastPhysPlugin::showActionPanel(bool show)
{
  if (!hasActionPanel() && show)
  {


  }
  else if (hasActionPanel() && !show)
  {


    return;
  }
}
*/

void FastPhysPlugin::makeObjectToTreeNodeMap(ObjectToTreeNodeMap &map, PropPanel::TLeafHandle parent) const
{
  const int childrenCount = mActionTree->getChildrenCount(parent);
  for (int i = 0; i < childrenCount; ++i)
  {
    const PropPanel::TLeafHandle child = mActionTree->getChild(parent, i);
    FpdAction *action = (FpdAction *)mActionTree->getItemData(child);
    if (action && action->getObject())
      map.insert({action->getObject(), child});

    makeObjectToTreeNodeMap(map, child);
  }
}

void FastPhysPlugin::updateActionPanel()
{
  if (!mActionTree)
    return;

  dag::Vector<IFPObject *> objects;
  mFastPhysEditor.getSelectedObjects(objects);

  ObjectToTreeNodeMap objectToTreeNodeMap;
  makeObjectToTreeNodeMap(objectToTreeNodeMap, nullptr);

  dag::Vector<PropPanel::TLeafHandle> selectedTreeNodes;
  mActionTree->getSelectedItems(selectedTreeNodes, true, true);

  dag::Vector<PropPanel::TLeafHandle> treeNodesToSelect;
  for (IFPObject *object : objects)
  {
    auto [rangeBegin, rangeEnd] = objectToTreeNodeMap.equal_range_small(object->getObject());

    // When the user selects an object and a tree node mapped to that object was already selected then only select that
    // tree node, not all tree nodes that map to that object. This way the selection is probably more user friendly.
    bool selectAll = true;

    for (auto it = rangeBegin; it != rangeEnd; ++it)
    {
      PropPanel::TLeafHandle treeNode = it->second;
      if (eastl::find(selectedTreeNodes.begin(), selectedTreeNodes.end(), treeNode) != selectedTreeNodes.end())
      {
        selectAll = false;
        break;
      }
    }

    for (auto it = rangeBegin; it != rangeEnd; ++it)
    {
      PropPanel::TLeafHandle treeNode = it->second;
      if (selectAll || eastl::find(selectedTreeNodes.begin(), selectedTreeNodes.end(), treeNode) != selectedTreeNodes.end())
        treeNodesToSelect.push_back(it->second);
    }
  }

  mActionTree->setSelectedItems(treeNodesToSelect);
}


void FastPhysPlugin::refillPanel()
{
  if (propPanel)
    propPanel->refillPanel();
}


void FastPhysPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel) { refillPanel(); }


bool FastPhysPlugin::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return mFastPhysEditor.handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}


bool FastPhysPlugin::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return mFastPhysEditor.handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}


bool FastPhysPlugin::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return mFastPhysEditor.handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
}


bool FastPhysPlugin::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return mFastPhysEditor.handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
}


bool FastPhysPlugin::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  return mFastPhysEditor.handleMouseRBRelease(wnd, x, y, inside, buttons, key_modif);
}


bool FastPhysPlugin::getSelectionBox(BBox3 &box) const
{
  AnimV20::IAnimCharacter2 *_anim_char = mFastPhysEditor.getAnimChar();
  if (_anim_char)
  {
    // box = BBox3(_anim_char->getBoundingSphere());
    box = BBox3(Point3(-1, -1, -1), Point3(1, 2, 1));

    return true;
  }

  return false;
}


void FastPhysPlugin::updateImgui()
{
  DAEDITOR3.imguiBegin("Actions Tree");

  if (mActionTree)
    mActionTree->updateImgui();

  DAEDITOR3.imguiEnd();
}


//------------------------------------------------------------------


static InitOnDemand<FastPhysPlugin> plugin;


void init_plugin_fastphys()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}

//------------------------------------------------------------------