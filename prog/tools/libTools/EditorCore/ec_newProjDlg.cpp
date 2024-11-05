// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_newProjDlg.h>
#include <libTools/util/strUtil.h>

#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_dialogs.h>

enum
{
  ID_NAME = 1,
  ID_LOCATION,
  ID_LABEL,
  ID_PATH,
};


NewProjectDialog::NewProjectDialog(void *phandle, const char *caption, const char *name_label, const char *_note) :
  DialogWindow(phandle, hdpi::_pxScaled(460), hdpi::_pxScaled(230), caption)
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "NewProjectDialog::NewProjectDialog: NO PANEL FOUND");

  _panel->createEditBox(ID_NAME, name_label, "");
  _panel->createFileEditBox(ID_LOCATION, "Location", "");
  _panel->setBool(ID_LOCATION, true);

  _panel->createStatic(ID_LABEL, _note);
  _panel->createStatic(ID_PATH, "");
}


void NewProjectDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case ID_NAME: mName = panel->getText(pcb_id); break;

    case ID_LOCATION: mLocation = panel->getText(pcb_id); break;
  }

  panel->setText(ID_PATH, String(1024, "%s/%s", mLocation.str(), mName.str()).str());
}


bool NewProjectDialog::onOk()
{
  if (mName.size() < 1)
  {
    wingw::message_box(0, "Invalid name", "You MUST specify name to proceed");
    return false;
  }

  return true;
}

void NewProjectDialog::setName(const char *_s)
{
  String s(_s);

  for (int i = s.length() - 1; i >= 0; i--)
    if (strchr("/\\", s[i]))
      erase_items(s, i, s.length() - 1 - i);
  mName = s;

  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "NewProjectDialog: NO PANEL FOUND");

  _panel->setText(ID_NAME, mName.str());
  _panel->setText(ID_PATH, String(1024, "%s/%s", mLocation.str(), mName.str()).str());
}

void NewProjectDialog::setLocation(const char *_s)
{
  String s(_s);
  for (int i = s.length() - 1; i >= 0; i--)
    if (strchr(" \n\r\t/\\", s[i]))
      erase_items(s, i, 1);
    else
      break;
  ::simplify_fname(s);
  mLocation = s;

  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "NewProjectDialog: NO PANEL FOUND");

  _panel->setText(ID_LOCATION, mLocation.str());
  _panel->setText(ID_PATH, String(1024, "%s/%s", mLocation.str(), mName.str()).str());
}

const char *NewProjectDialog::getName() { return mName.str(); }

const char *NewProjectDialog::getLocation() { return mLocation.str(); }
