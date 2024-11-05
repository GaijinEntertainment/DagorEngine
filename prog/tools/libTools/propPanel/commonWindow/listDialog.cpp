// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/commonWindow/listDialog.h>
#include <propPanel/control/container.h>
#include <propPanel/c_util.h>

namespace PropPanel
{

enum
{
  ID_LIST,
};

ListDialog::ListDialog(const char *caption, const Tab<String> &vals, hdpi::Px width, hdpi::Px height) :
  DialogWindow(nullptr, width, height, caption)
{
  PropPanel::ContainerPropertyControl *_panel = ListDialog::getPanel();
  G_ASSERT(_panel && "ListDialog: No panel found!");

  for (const String &val : vals)
  {
    G_ASSERT(!val.find("\r"));
    G_ASSERT(!val.find("\n"));
  }

  _panel->createList(ID_LIST, "", vals, -1);
  _panel->getById(ID_LIST)->setHeight(height);
}

void ListDialog::onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_LIST)
    clickDialogButton(DIALOG_ID_OK);
}

int ListDialog::getSelectedIndex() const
{
  PropPanel::ContainerPropertyControl *_panel = const_cast<ListDialog *>(this)->getPanel();
  return _panel->getInt(ID_LIST);
}

const char *ListDialog::getSelectedText()
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  lastSelectText = _panel->getText(ID_LIST);
  return lastSelectText;
}

void ListDialog::setSelectedIndex(int index)
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  _panel->setInt(ID_LIST, index);
}

} // namespace PropPanel