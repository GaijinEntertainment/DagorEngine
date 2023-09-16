#include <scriptHelpers/tunedParams.h>
#include <scriptHelpers/scriptPanel.h>
#include "scriptHelpersPanelUserData.h"

#include <sepGui/wndPublic.h>
#include <propPanel2/c_panel_base.h>
#include <propPanel2/comWnd/panel_window.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <winGuiWrapper/wgw_dialogs.h>


using namespace ScriptHelpers;


namespace ScriptHelpers
{
enum
{
  EFFECTS_PROP_WINDOW_TYPE = 25, // just not to conflict with other types
  EFFECTS_TREE_WINDOW_TYPE = 26,
};

class ParamsTreeCB;
class ScriptHelpersPropPanelBar;

IWndManager *wnd_manager = NULL;

ScriptHelpersPropPanelBar *prop_bar = NULL;
TreeBaseWindow *tree_list = NULL;
ParamsTreeCB *tree_callback = NULL;

extern TunedElement *root_element;
extern TunedElement *selected_elem;
extern ParamChangeCB *param_change_cb;
}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{
class ScriptHelpersPropPanelBar : public ControlEventHandler, public IPropPanelCB
{
public:
  static const int START_PID = 100;

  bool shouldRebuild;


  ScriptHelpersPropPanelBar(void *phandle, int x, int y, unsigned w, unsigned h) :

    mPanel(NULL), shouldRebuild(false)
  {
    mPanel = new CPanelWindow(this, phandle, x, y, w, h, "");
  }


  ~ScriptHelpersPropPanelBar() { del_it(mPanel); }


  void *getParentWindowHandle()
  {
    if (mPanel)
      return mPanel->getParentWindowHandle();

    return 0;
  }


  CPanelWindow *getPanelWindow() { return mPanel; }


  virtual void rebuildTreeList() { shouldRebuild = true; }

  void updateArrayItemCaption(int pcb_id, PropertyContainerControlBase &panel)
  {
    PropertyControlBase *control = panel.getById(pcb_id);
    if (!control)
      return;

    // The first parent is the structure's group, the parent's parent is the array item.
    PropertyControlBase *arrayItemGroup = control->getParent();
    arrayItemGroup = arrayItemGroup ? arrayItemGroup->getParent() : nullptr;
    if (!arrayItemGroup)
      return;

    ScriptHelpersPanelUserData::DataType dataType;
    TunedElement *arrayItem = ScriptHelpersPanelUserData::retrieve(arrayItemGroup->getUserDataValue(), dataType);
    if (!arrayItem || dataType != ScriptHelpersPanelUserData::DataType::ArrayItem)
      return;

    PropertyControlBase *arrayGroup = arrayItemGroup->getParent();
    if (!arrayGroup)
      return;

    TunedElement *array = ScriptHelpersPanelUserData::retrieve(arrayGroup->getUserDataValue(), dataType);
    if (!array || dataType != ScriptHelpersPanelUserData::DataType::Array)
      return;

    String caption;
    if (array->getPanelArrayItemCaption(caption, *array, *arrayItem))
      arrayItemGroup->setCaptionValue(caption);
  }

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel)
  {
    TunedElement *elem = NULL;

    if (selected_elem)
      elem = selected_elem;
    else if (root_element)
      elem = root_element;

    if ((elem) && (mPanel))
    {
      PropertyContainerControlBase *_pp = mPanel->getContainer();
      G_ASSERT(_pp && "ScriptHelpersPropPanelBar: There is no container found!");

      int pid = START_PID;

      elem->getValues(pid, *panel);
      updateArrayItemCaption(pcb_id, *panel);
      on_param_change();
    }
  }


  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel)
  {
    shouldRebuild = false;

    if (selected_elem)
      selected_elem->onClick(pcb_id, *panel, *this);
    else if (root_element)
      root_element->onClick(pcb_id, *panel, *this);

    if (shouldRebuild)
      panel->setPostEvent(pcb_id);
  }


  virtual void onPostEvent(int pcb_id, PropertyContainerControlBase *panel)
  {
    rebuild_tree_list();
    on_param_change();

    // maximize first group

    PropertyControlBase *cb = mPanel->getById(START_PID + 1);
    if (cb)
    {
      PropertyContainerControlBase *ccb = cb->getContainer();
      if (ccb)
        ccb->setBoolValue(false);
    }
  }


  virtual void fillPanel()
  {
    mPanel->showPanel(false);

    G_ASSERT(mPanel && "ScriptHelpersPropPanelBar: There is no panel found!");
    PropertyContainerControlBase *_pp = mPanel->getContainer();
    G_ASSERT(_pp && "ScriptHelpersPropPanelBar: There is no container found!");
    _pp->clear();

    int pid = START_PID;

    if (selected_elem)
    {
      if (root_element)
        root_element->resetPropPanel();
      selected_elem->fillPropPanel(pid, *_pp);
    }
    else if (root_element)
      root_element->fillPropPanel(pid, *_pp);

    mPanel->showPanel(true);
  }

protected:
  CPanelWindow *mPanel;
};


void on_param_change()
{
  if (param_change_cb)
    param_change_cb->onScriptHelpersParamChange();
}


void set_param_change_cb(ParamChangeCB *cb) { param_change_cb = cb; }

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ScriptHelpers::ParamsTreeCB : public ITreeViewEventHandler
{
public:
  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel)
  {
    selected_elem = (TunedElement *)tree.getItemData(new_sel);

    if (prop_bar)
      prop_bar->fillPanel();
  }

  virtual void onTvListSelection(TreeBaseWindow &tree, int index) {}

  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu) { return false; }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void tree_fill(TreeBaseWindow *tree, TLeafHandle parent, TunedElement *e)
{
  G_ASSERT(e);

  TLeafHandle cur = tree->addItem(e->getName(), -1, parent, e);

  int num = e->subElemCount();
  for (int i = 0; i < num; ++i)
  {
    TunedElement *se = e->getSubElem(i);
    if (!se)
      continue;

    ::tree_fill(tree, cur, se);
  }
}


void ScriptHelpers::rebuild_tree_list()
{
  selected_elem = NULL;

  if (tree_list)
  {
    tree_list->clear();

    if (root_element)
    {
      ::tree_fill(tree_list, tree_list->getRoot(), root_element);
      tree_list->expand(tree_list->getRoot());
    }
  }

  if (prop_bar)
    prop_bar->fillPanel();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


bool ScriptHelpers::create_helper_bar(IWndManager *manager, void *htree, void *hpropbar)
{
  wnd_manager = manager;

  manager->setWindowType(htree, EFFECTS_TREE_WINDOW_TYPE);
  manager->setWindowType(hpropbar, EFFECTS_PROP_WINDOW_TYPE);
  return true;
}


IWndEmbeddedWindow *ScriptHelpers::on_wm_create_window(void *handle, int type)
{
  switch (type)
  {
    case EFFECTS_PROP_WINDOW_TYPE:
    {
      if (prop_bar)
        return NULL;

      wnd_manager->setCaption(handle, "Effects Props");

      unsigned w, h;
      wnd_manager->getWindowClientSize(handle, w, h);
      prop_bar = new ScriptHelpersPropPanelBar(handle, 0, 0, w, h);

      return prop_bar->getPanelWindow();
    }
    break;

    case EFFECTS_TREE_WINDOW_TYPE:
    {
      if (tree_list)
        return NULL;

      wnd_manager->setCaption(handle, "Effects Tree");

      unsigned w, h;
      wnd_manager->getWindowClientSize(handle, w, h);
      tree_callback = new ParamsTreeCB();
      tree_list = new TreeBaseWindow(tree_callback, handle, 0, 0, w, h, "", false);

      return tree_list;
    }
  }

  return NULL;
}


bool ScriptHelpers::on_wm_destroy_window(void *handle)
{
  if (prop_bar && prop_bar->getParentWindowHandle() == handle)
  {
    del_it(prop_bar);
    return true;
  }

  if (tree_list && tree_list->getParentWindowHandle() == handle)
  {
    del_it(tree_list);
    del_it(tree_callback);
    return true;
  }

  return false;
}
