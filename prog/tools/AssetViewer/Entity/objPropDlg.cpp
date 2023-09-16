#include "objPropDlg.h"
#include <string>
#include <sstream>
#include <unordered_map>
enum
{
  ID_NODE_LIST,
  ID_PROPS,
};

ObjPropDialog::ObjPropDialog(const char *caption, int width, int height, const Tab<String> &nodes, const Tab<String> &scripts) :
  CDialogWindow(p2util::get_main_parent_handle(), width, height, caption), scripts(scripts)
{
  PropertyContainerControlBase *panel = getPanel();
  G_ASSERTF(panel, "ObjPropDialog: No panel found!");
  panel->createMultiSelectList(ID_NODE_LIST, nodes, height / 3);
  panel->createEditBox(ID_PROPS, "", "", true, true, true, height / 2 - 25);
  panel->setEventHandler(this);
}

void ObjPropDialog::applyChanges(const Tab<int> &sels)
{
  if (!changed)
    return;
  PropertyContainerControlBase *panel = getPanel();
  for (int i = 0; i < sels.size(); ++i)
    scripts[sels[i]] = panel->getText(ID_PROPS);
  shouldSave = true;
}

void ObjPropDialog::onClick(int pcb_id, PropertyContainerControlBase *panel) {}

void ObjPropDialog::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  if (pcb_id == ID_NODE_LIST)
  {
    applyChanges(prevSels);
    Tab<int> sels;
    panel->getSelection(ID_NODE_LIST, sels);
    changed = false;
    prevSels = sels;
    std::unordered_map<std::string, int> lines;
    for (int i = 0; i < sels.size(); ++i)
    {
      std::stringstream ss(scripts[sels[i]].c_str());
      std::string to;
      while (std::getline(ss, to))
        ++lines[to];
    }
    std::stringstream ss;
    for (const auto &line : lines)
      if (line.second == sels.size())
        ss << line.first << "\r\n";
    panel->setText(ID_PROPS, ss.str().c_str());
  }
  else if (pcb_id == ID_PROPS)
    changed = true;
}

bool ObjPropDialog::onOk()
{
  if (changed)
  {
    PropertyContainerControlBase *panel = getPanel();
    Tab<int> sels;
    panel->getSelection(ID_NODE_LIST, sels);
    applyChanges(sels);
  }
  return shouldSave;
}
