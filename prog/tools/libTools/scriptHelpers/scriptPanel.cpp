// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scriptHelpers/tunedParams.h>
#include <scriptHelpers/scriptPanel.h>
#include "scriptHelpersPanelUserData.h"

#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <propPanel/control/container.h>
#include <propPanel/control/container.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <imgui/imgui.h>


using namespace ScriptHelpers;
using hdpi::_pxActual;

namespace ScriptHelpers
{
class ScriptHelpersPropPanelBar;
}

class TunedElementUndoRedo : public UndoRedoObject
{
public:
  explicit TunedElementUndoRedo(PropPanel::ContainerPropertyControl &panel, int pid, TunedElement *undo, TunedElement *redo,
    ScriptHelpers::ScriptHelpersPropPanelBar *panel_bar) :
    panel(panel), pid(pid), redoElement(redo), undoElement(undo), panelBar(panel_bar)
  {}

  void restore(bool save_redo_data) override { applayChange(undoElement); }
  void redo() override { applayChange(redoElement); }

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "TunedElementUndoRedo"; }

private:
  void applayChange(eastl::unique_ptr<TunedElement> &element);

  PropPanel::ContainerPropertyControl &panel;
  int pid;
  eastl::unique_ptr<TunedElement> redoElement;
  eastl::unique_ptr<TunedElement> undoElement;
  ScriptHelpers::ScriptHelpersPropPanelBar *panelBar;
};

class TunedArrayUndoRedo : public UndoRedoObject
{
public:
  explicit TunedArrayUndoRedo(PropPanel::ContainerPropertyControl &panel, ScriptHelpers::IPropPanelCB &ppcb, int pid,
    TunedElement &source, TunedElement *removed_element) :
    panel(panel), ppcb(ppcb), source(source), pid(pid), removedElement(removed_element)
  {}

  void restore(bool save_redo_data) override
  {
    if (source.undoOperation(pid, panel, removedElement.get()))
      panel.setPostEvent(pid);
  }
  void redo() override { source.onClick(pid, panel, ppcb); }
  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "TunedArrayUndoRedo"; }

private:
  PropPanel::ContainerPropertyControl &panel;
  ScriptHelpers::IPropPanelCB &ppcb;
  TunedElement &source;
  int pid;
  eastl::unique_ptr<TunedElement> removedElement;
};

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
PropPanel::TreeBaseWindow *tree_list = NULL;
ParamsTreeCB *tree_callback = NULL;

extern TunedElement *root_element;
extern TunedElement *selected_elem;
extern ParamChangeCB *param_change_cb;
}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{
class ScriptHelpersPropPanelBar : public PropPanel::ControlEventHandler, public IPropPanelCB
{
public:
  static const int START_PID = 100;

  bool shouldRebuild;


  ScriptHelpersPropPanelBar(int x, int y, unsigned w, unsigned h) :

    mPanel(NULL), shouldRebuild(false)
  {
    mPanel = new PropPanel::ContainerPropertyControl(0, this, nullptr, x, y, _pxActual(w), _pxActual(h));
  }


  ~ScriptHelpersPropPanelBar() override { del_it(mPanel); }


  PropPanel::ContainerPropertyControl *getPanelWindow() { return mPanel; }


  void rebuildTreeList() override { shouldRebuild = true; }

  void updateArrayItemCaption(int pcb_id, PropPanel::ContainerPropertyControl &panel)
  {
    PropPanel::PropertyControlBase *control = panel.getById(pcb_id);
    if (!control)
      return;

    // The first parent is the structure's group, the parent's parent is the array item.
    PropPanel::PropertyControlBase *arrayItemGroup = control->getParent();
    arrayItemGroup = arrayItemGroup ? arrayItemGroup->getParent() : nullptr;
    if (!arrayItemGroup)
      return;

    ScriptHelpersPanelUserData::DataType dataType;
    TunedElement *arrayItem = ScriptHelpersPanelUserData::retrieve(arrayItemGroup->getUserDataValue(), dataType);
    if (!arrayItem || dataType != ScriptHelpersPanelUserData::DataType::ArrayItem)
      return;

    PropPanel::PropertyControlBase *arrayGroup = arrayItemGroup->getParent();
    if (!arrayGroup)
      return;

    TunedElement *array = ScriptHelpersPanelUserData::retrieve(arrayGroup->getUserDataValue(), dataType);
    if (!array || dataType != ScriptHelpersPanelUserData::DataType::Array)
      return;

    String caption;
    if (array->getPanelArrayItemCaption(caption, *array, *arrayItem))
      arrayItemGroup->setCaptionValue(caption);
  }

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    TunedElement *elem = NULL;

    if (selected_elem)
      elem = selected_elem;
    else if (root_element)
      elem = root_element;

    if ((elem) && (mPanel))
    {
      PropPanel::ContainerPropertyControl *_pp = mPanel->getContainer();
      G_ASSERT(_pp && "ScriptHelpersPropPanelBar: There is no container found!");

      if (obj_editor && !isUndoChange)
      {
        obj_editor->getUndoSystem()->begin();
        int pid = START_PID;
        TunedElement *undoElement = elem->findById(pcb_id, pid);
        if (undoElement)
        {
          TunedElement *redoElement = undoElement->cloneElem();
          int update_pid = pcb_id - 1;
          redoElement->getValues(update_pid, *panel);
          obj_editor->getUndoSystem()->put(new TunedElementUndoRedo(*panel, pcb_id, undoElement, redoElement, this));
          obj_editor->getUndoSystem()->accept("changeTunedElementParam");
        }
      }
      int pid = START_PID;

      elem->getValues(pid, *panel);
      updateArrayItemCaption(pcb_id, *panel);
      on_param_change();
    }
  }


  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    shouldRebuild = false;
    int pid = START_PID;
    TunedElement *element = selected_elem ? selected_elem : root_element;

    // handle only button clicks and avoid check box changes, it's handle in onChange()
    if (element->isArrayActionById(pcb_id, pid))
    {
      pid = START_PID;
      TunedElement *removedElement = element->findById(pcb_id, pid);
      obj_editor->getUndoSystem()->begin();
      obj_editor->getUndoSystem()->put(new TunedArrayUndoRedo(*panel, *this, pcb_id, *element, removedElement));
      obj_editor->getUndoSystem()->accept("changeTunedArray");
    }
    if (selected_elem)
      selected_elem->onClick(pcb_id, *panel, *this);
    else if (root_element)
      root_element->onClick(pcb_id, *panel, *this);

    if (shouldRebuild)
      panel->setPostEvent(pcb_id);
  }


  void onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    rebuild_tree_list();
    on_param_change();

    // maximize first group

    PropPanel::PropertyControlBase *cb = mPanel->getById(START_PID + 1);
    if (cb)
    {
      PropPanel::ContainerPropertyControl *ccb = cb->getContainer();
      if (ccb)
        ccb->setBoolValue(false);
    }
  }


  void fillPanel()
  {
    G_ASSERT(mPanel && "ScriptHelpersPropPanelBar: There is no panel found!");
    PropPanel::ContainerPropertyControl *_pp = mPanel->getContainer();
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
  }

  void setUndoChange(bool is_undo_change) { isUndoChange = is_undo_change; }

protected:
  PropPanel::ContainerPropertyControl *mPanel;
  bool isUndoChange = false;
};


void on_param_change()
{
  if (param_change_cb)
    param_change_cb->onScriptHelpersParamChange();
}


void set_param_change_cb(ParamChangeCB *cb) { param_change_cb = cb; }

}; // namespace ScriptHelpers

void TunedElementUndoRedo::applayChange(eastl::unique_ptr<TunedElement> &element)
{
  panelBar->setUndoChange(true);
  element.get()->setValue(pid, panel);
  panelBar->onChange(pid, &panel);
  panelBar->setUndoChange(false);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ScriptHelpers::ParamsTreeCB : public PropPanel::ITreeViewEventHandler
{
public:
  void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override
  {
    TunedElement *newElement = (TunedElement *)tree.getItemData(new_sel);
    bool isSelectedElemChanged = newElement != selected_elem;
    selected_elem = newElement;

    if (prop_bar)
      prop_bar->fillPanel();
    if (obj_editor && isSelectedElemChanged)
      obj_editor->getUndoSystem()->clear();
  }

  void onTvListSelection(PropPanel::TreeBaseWindow &tree, int index) override {}

  bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override { return false; }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void tree_fill(PropPanel::TreeBaseWindow *tree, PropPanel::TLeafHandle parent, TunedElement *e)
{
  G_ASSERT(e);

  PropPanel::TLeafHandle cur = tree->addItem(e->getName(), nullptr, parent, e);

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


bool ScriptHelpers::create_helper_bar(IWndManager *manager)
{
  wnd_manager = manager;

  manager->setWindowType(nullptr, EFFECTS_TREE_WINDOW_TYPE);
  manager->setWindowType(nullptr, EFFECTS_PROP_WINDOW_TYPE);
  return true;
}


void *ScriptHelpers::on_wm_create_window(int type)
{
  switch (type)
  {
    case EFFECTS_PROP_WINDOW_TYPE:
    {
      if (prop_bar)
        return NULL;

      prop_bar = new ScriptHelpersPropPanelBar(0, 0, 0, 0);

      return prop_bar->getPanelWindow();
    }
    break;

    case EFFECTS_TREE_WINDOW_TYPE:
    {
      if (tree_list)
        return NULL;

      tree_callback = new ParamsTreeCB();
      tree_list = new PropPanel::TreeBaseWindow(tree_callback, nullptr, 0, 0, _pxActual(0), _pxActual(0), "", false);

      return tree_list;
    }
  }

  return NULL;
}


void ScriptHelpers::removeWindows()
{
  del_it(tree_list);
  del_it(tree_callback);
  del_it(prop_bar);
}


void ScriptHelpers::updateImgui()
{
  if (tree_list)
  {
    if (ImGui::BeginChild("tree", ImVec2(0.0f, 250.0f), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoBackground))
      tree_list->updateImgui();
    ImGui::EndChild();
  }

  PropPanel::ContainerPropertyControl *propertyPanel = prop_bar ? prop_bar->getPanelWindow() : nullptr;
  if (propertyPanel)
    propertyPanel->updateImgui();
}

void ScriptHelpers::save_params_state(DataBlock &blk) { prop_bar->getPanelWindow()->saveState(blk); }

void ScriptHelpers::load_params_state(DataBlock &blk) { prop_bar->getPanelWindow()->loadState(blk); }
