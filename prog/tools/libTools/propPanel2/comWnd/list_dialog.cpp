// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/list_dialog.h>
#include <propPanel2/c_util.h>

#include "../windowControls/w_window.h"
#include "../windowControls/w_boxes.h"
#include "../windowControls/w_simple_controls.h"


ListDialog::ListDialog(void *handle, const char *caption, const Tab<String> &vals, int width, int height) :

  CDialogWindow(handle, width, height, caption, true)
{
  PropertyContainerControlBase *_pp = getPanel();
  G_ASSERT(_pp && "SelectAssetDlg::SelectAssetDlg : No panel with controls");

  mContainer = new WContainer(this, mDialogWindow, 0, 0, _pp->getWidth(), _pp->getHeight());

  // prepare multiline text

  Tab<String> lines(tmpmem);
  for (int i = 0; i < vals.size(); ++i)
  {
    char *ptr, *buf = new char[vals[i].size() + 1];
    buf = strcpy(buf, vals[i].str());
    ptr = strtok(buf, "\r\n");
    while (ptr)
    {
      if (strlen(ptr))
        lines.push_back() = ptr;
      ptr = strtok(NULL, "\r\n");
    }

    delete[] buf;
  }

  // create

  mList = new WListBox(this, mContainer, 0, 0, _pp->getWidth(), _pp->getHeight(), lines, 0);

  mList->setFont(WindowBase::getNormalFont());
  mList->setSelectedIndex(-1);
}


ListDialog::~ListDialog()
{
  del_it(mList);
  del_it(mContainer);
}


void ListDialog::onWcDoubleClick(WindowBase *source) { click(DIALOG_ID_OK); }


int ListDialog::getSelectedIndex() const { return mList->getSelectedIndex(); }


const char *ListDialog::getSelectedText()
{
  if (getSelectedIndex() > -1)
    mList->getSelectedText(mLastSelectText, sizeof(mLastSelectText));
  else
    strcpy(mLastSelectText, "");

  return mLastSelectText;
}


void ListDialog::setSelectedIndex(int index) { mList->setSelectedIndex(index); }


//------------------------------------------------

enum
{
  ID_LIST,

  ID_SELECT_ALL,
  ID_SELECT_NONE,
  ID_SELECT_INVERT,
};


MultiListDialog::MultiListDialog(const char *caption, int width, int height, const Tab<String> &vals, Tab<String> &sels) :

  CDialogWindow(p2util::get_main_parent_handle(), width, height, caption),

  mSels(&sels),
  mSelsIndTab(NULL)
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "MultiListDialog: No panel found!");

  _panel->createMultiSelectList(ID_LIST, vals, height - 125);

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
  PropertyContainerControlBase *_panel = getPanel();
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


void MultiListDialog::onClick(int pcb_id, PropertyContainerControlBase *panel)
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


void MultiListDialog::onDoubleClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case ID_LIST: click(DIALOG_ID_OK); break;
  }
}
