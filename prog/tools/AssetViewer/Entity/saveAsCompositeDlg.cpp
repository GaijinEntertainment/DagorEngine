// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "saveAsCompositeDlg.h"

#include "../av_appwnd.h"

#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>


enum
{
  ID_NAME = 99,
  ID_NAME_ERR,
  ID_REPLACE,
  ID_WARN,
};

static const int DEFAULT_WIDTH = 390;
static const int DEFAULT_HEIGHT = 235;
static const int HEIGHT_WITHOUT_REPLACE_OPTION = 165;

static const char *DEFAULT_CAPTION = "Save selected nodes as a new composite";

static const char *NAME_ERR_EMPTY = "The name cannot be empty!";
static const char *NAME_ERR_IDENT = "Allowed characters: a-z, A-Z, 0-9 and _! Try the following:\n%s";
static const char *NAME_ERR_EXISTS = "Cmp already exists with this name. Try the following:\n%s";

SaveAsNewCompositeDialog::SaveAsNewCompositeDialog(DagorAsset *edited_asset, const char *caption, bool show_replace_option) :
  DialogWindow(nullptr, hdpi::_pxScaled(DEFAULT_WIDTH),
    hdpi::_pxScaled(show_replace_option ? DEFAULT_HEIGHT : HEIGHT_WITHOUT_REPLACE_OPTION), caption ? caption : DEFAULT_CAPTION),
  editedAsset(edited_asset)
{
  G_ASSERTF(edited_asset, "SaveAsNewCompositeDialog: No edited asset passed in!");

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERTF(panel, "SaveAsNewCompositeDialog: No panel found!");

  panel->createEditBox(ID_NAME, "Name");
  nameInfoPanel = panel->createContainer(ID_NAME_ERR);
  nameInfoPanel->createStatic(ID_NAME_ERR, NAME_ERR_EMPTY);

  if (show_replace_option)
  {
    panel->createCheckBox(ID_REPLACE, "Replace selection with new composit", true);
    panel->createStaticWithIcon(ID_WARN, "If enabled, the selected nodes will be replaced in the\nscene by the new composite.");
    panel->setButtonPictures(ID_WARN, "alert");
  }

  panel->setEventHandler(this);

  setDialogButtonText(PropPanel::DIALOG_ID_OK, "Save");
  setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, false);

  setInitialFocus(ID_NAME);
}

void SaveAsNewCompositeDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_NAME)
  {
    String name(panel->getText(pcb_id).c_str());
    trim(name);
    if (name.empty())
    {
      handleNameError(NAME_ERR_EMPTY);
      return;
    }

    String identName = name;
    make_ident_like_name(identName);
    if (identName != name)
    {
      String err(260, NAME_ERR_IDENT, identName.c_str());
      handleNameError(err.c_str());
      return;
    }

    const bool alreadyExists = get_app().getAssetMgr().findAsset(name);
    if (alreadyExists)
    {
      String suggestion;
      if (name.suffix("_cmp"))
      {
        suggestion.setStr(name.data(), name.size() - 5);
        suggestion += "_a_cmp";
      }
      else
        suggestion.setStrCat(name, "_a");

      String err(260, NAME_ERR_EXISTS, suggestion.c_str());
      handleNameError(err.c_str());
      return;
    }

    nameInfoPanel->setText(ID_NAME_ERR, "");

    setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, true);
  }
}

String SaveAsNewCompositeDialog::getFullPath()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERTF(panel, "SaveAsNewCompositeDialog::getFullPath() No panel found!");

  const char *folderPath = editedAsset->getFolderPath();
  String fullPath(512, "%s/%s.composit.blk", folderPath, panel->getText(ID_NAME).c_str());
  simplify_fname(fullPath);
  return fullPath;
}

String SaveAsNewCompositeDialog::getFileName()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERTF(panel, "SaveAsNewCompositeDialog::getFileName() No panel found!");

  String fileName(panel->getText(ID_NAME).c_str());
  fileName += ".composit.blk";
  return fileName;
}

bool SaveAsNewCompositeDialog::shouldReplaceSelection()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERTF(panel, "SaveAsNewCompositeDialog::shouldReplaceSelection() No panel found!");

  return panel->getBool(ID_REPLACE);
}

String SaveAsNewCompositeDialog::getAssetName()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERTF(panel, "SaveAsNewCompositeDialog::getAssetName() No panel found!");

  const char *name = panel->getText(ID_NAME).c_str();
  return String(name);
}

void SaveAsNewCompositeDialog::handleNameError(const char *text)
{
  nameInfoPanel->setText(ID_NAME_ERR, text);
  setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, false);
}
