// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/commonWindow/multiListDialog.h>
#include <propPanel/control/container.h>
#include <propPanel/c_util.h>

namespace PropPanel
{

enum
{
  ID_LIST,

  ID_SELECT_ALL,
  ID_SELECT_NONE,
  ID_SELECT_INVERT,
};

MultiListDialog::MultiListDialog(const char *caption, hdpi::Px width, hdpi::Px height, const Tab<String> &vals, Tab<String> &sels) :
  DialogWindow(nullptr, width, height, caption), mSels(&sels), mSelsIndTab(NULL)
{
  PropPanel::ContainerPropertyControl *_panel = MultiListDialog::getPanel();
  G_ASSERT(_panel && "MultiListDialog: No panel found!");

  _panel->createMultiSelectList(ID_LIST, vals, height - hdpi::_pxScaled(125));

  _panel->createButton(ID_SELECT_ALL, "All", true, true);
  _panel->createButton(ID_SELECT_NONE, "None", true, false);
  _panel->createButton(ID_SELECT_INVERT, "Invert", true, false);

  Tab<int> selection(tmpmem);

  for (int i = 0; i < vals.size(); ++i)
    for (int j = 0; j < mSels->size(); ++j)
      if (vals[i] == (*mSels)[j])
        selection.push_back(i);

  _panel->setSelection(ID_LIST, selection);
}


bool MultiListDialog::onOk()
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "MultiListDialog: NO PANEL FOUND!!!");

  Tab<int> sel(tmpmem);
  Tab<String> list(tmpmem);

  _panel->getSelection(ID_LIST, sel);
  _panel->getStrings(ID_LIST, list);
  clear_and_shrink(*mSels);

  for (int i = 0; i < sel.size(); ++i)
    mSels->push_back(list[sel[i]]);

  if (mSelsIndTab)
    *mSelsIndTab = sel;

  return true;
}


void MultiListDialog::setSelectionTab(Tab<int> *sels) { mSelsIndTab = sels; }


void MultiListDialog::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  Tab<int> sels(tmpmem);
  panel->getSelection(ID_LIST, sels);
  int _count = panel->getInt(ID_LIST);

  switch (pcb_id)
  {
    case ID_SELECT_NONE:
      clear_and_shrink(sels);
      panel->setSelection(ID_LIST, sels);
      break;

    case ID_SELECT_ALL:
      clear_and_shrink(sels);
      for (int i = 0; i < _count; ++i)
        sels.push_back(i);
      panel->setSelection(ID_LIST, sels);
      break;

    case ID_SELECT_INVERT:
    {
      bool *flags = new bool[_count];

      for (int i = 0; i < _count; ++i)
        flags[i] = true;

      for (int i = 0; i < sels.size(); ++i)
        flags[sels[i]] = false;

      clear_and_shrink(sels);
      for (int i = 0; i < _count; ++i)
        if (flags[i])
          sels.push_back(i);

      delete[] flags;
      panel->setSelection(ID_LIST, sels);
    }
    break;
  }
}


void MultiListDialog::onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_LIST)
    clickDialogButton(DIALOG_ID_OK);
}

} // namespace PropPanel