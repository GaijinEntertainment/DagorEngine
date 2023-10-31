#include "entityCopyDlg.h"

#include <oldEditor/de_interface.h>

enum
{
  PID_CLONE_PREFIX = 11000,
  PID_CLONE_COUNT,
  PID_CLONE_SEED,
};


//==============================================================================
CopyDlg::CopyDlg(String &clone_name, int &clone_count, bool &clone_seed) :
  cloneName(clone_name), cloneCount(clone_count), cloneSeed(clone_seed), mDialog(NULL)
{
  mDialog = DAGORED2->createDialog(hdpi::_pxScaled(290), hdpi::_pxScaled(200), "Clone object");

  PropPanel2 &panel = *mDialog->getPanel();

  panel.createEditBox(PID_CLONE_PREFIX, "Name of clone(s) (empty - automatic):", cloneName);
  panel.createEditInt(PID_CLONE_COUNT, "Count of clones:", 1);
  panel.createCheckBox(PID_CLONE_SEED, "Clone the same random seed", cloneSeed);
}


CopyDlg::~CopyDlg() { del_it(mDialog); }

//==================================================================================================

bool CopyDlg::execute()
{
  int ret = mDialog->showDialog();

  if (ret == DIALOG_ID_OK)
  {
    PropPanel2 &panel = *mDialog->getPanel();

    cloneName = panel.getText(PID_CLONE_PREFIX);
    cloneCount = panel.getInt(PID_CLONE_COUNT);
    cloneSeed = panel.getBool(PID_CLONE_SEED);

    if (cloneCount < 1)
      cloneCount = 1;

    return true;
  }

  return false;
}
