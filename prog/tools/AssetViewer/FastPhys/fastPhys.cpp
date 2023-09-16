#include "../av_plugin.h"
#include "../av_appwnd.h"

#include <animChar/dag_animCharacter2.h>

#include <assets/asset.h>
#include <de3_interface.h>
#include <generic/dag_initOnDemand.h>

#include <util/dag_fastPtrList.h>

#include <debug/dag_debug.h>

#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_control_event_handler.h>

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
  mFastPhysEditor(*this), hwndPanel(NULL), mActionTree(NULL), mActionTreeCB(NULL), propPanel(NULL), mAsset(NULL)
{}


FastPhysPlugin::~FastPhysPlugin() {}


bool FastPhysPlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "fastPhys") == 0; }


void FastPhysPlugin::addTreeAction(TLeafHandle parent, FpdAction *action)
{
  G_ASSERT(action);

  FpdObject *object = action->getObject();
  String name;
  if (object)
    name.printf(256, "%s: %s", action->actionName.str(), object->getName());
  else
    name = action->actionName.str();

  TLeafHandle cur = mActionTree->addItem(name.str(), -1, parent, action);

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

  TLeafHandle root = mActionTree->addItem("Actions", -1, mActionTree->getRoot(), NULL);

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
  hwndPanel = getAdditinalPropWindow();
  G_ASSERT(hwndPanel);
  manager.fixWindow(hwndPanel, true);

  propPanel = new FPPanel(*this);
  manager.setWindowType(hwndPanel, WINDOW_TYPE_ACTIONS_TREE);

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
  manager.setWindowType(hwndPanel, CLIENT_WINDOW_TYPE_NONE);
  manager.unregisterWindowHandler(this);

  return true;
}


IWndEmbeddedWindow *FastPhysPlugin::onWmCreateWindow(void *handle, int type)
{
  switch (type)
  {
    case WINDOW_TYPE_ACTIONS_TREE:
    {
      if (mActionTree)
        return NULL;

      IWndManager &manager = getWndManager();
      manager.setCaption(handle, "Actions Tree");

      unsigned w, h;
      manager.getWindowClientSize(handle, w, h);
      mActionTreeCB = new ActionsTreeCB(*this);
      mActionTree = new TreeBaseWindow(mActionTreeCB, handle, 0, 0, w, h, "", false);

      return mActionTree;
    }
  }

  return NULL;
}


bool FastPhysPlugin::onWmDestroyWindow(void *handle)
{
  if (mActionTree && mActionTree->getParentWindowHandle() == handle)
  {
    del_it(mActionTree);
    del_it(mActionTreeCB);

    return true;
  }

  return NULL;
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

void FastPhysPlugin::updateActionPanel()
{
  if (mActionTree)
  {
    TLeafHandle sr = mActionTree->getRoot();

    IFPObject *wobj = mFastPhysEditor.getSelObject();

    if (wobj && wobj->getObject())
    {
      FastPtrList found_idx;
      for (;;)
      {
        sr = mActionTree->search(wobj->getObject()->getName(), sr, true);
        if (!found_idx.addPtr(sr))
          break;

        FpdAction *action = NULL;

        if (sr)
          action = (FpdAction *)mActionTree->getItemData(sr);
        else
          return;

        if (action && action->getObject() == wobj->getObject())
        {
          mActionTree->setSelectedItem(sr);
          return;
        }
      }
    }

    mActionTree->setSelectedItem(sr);
  }
}


void FastPhysPlugin::refillPanel()
{
  if (propPanel)
    propPanel->refillPanel();
}


void FastPhysPlugin::fillPropPanel(PropertyContainerControlBase &panel) { refillPanel(); }


void FastPhysPlugin::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif) { mFastPhysEditor.handleKeyPress(wnd, vk, modif); }


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