#include "dagListDlg.h"

#include <oldEditor/de_interface.h>

#include <util/dag_globDef.h>


#define DIALOG_WIDTH        240
#define DIALOG_CLIENT_WIDTH (DIALOG_WIDTH - 8)
#define STATIC_HEIGHT       21
#define BUTTON_WIDTH        100
#define BUTTON_HEIGHT       28


//==============================================================================
DagListDlg::DagListDlg(Tab<String> &dag_files) : CDialogWindow(NULL, DIALOG_WIDTH, DIALOG_WIDTH, "Clipping DAG files")
{
  mPanel = getPanel();
  G_ASSERT(mPanel && "No panel in CamerasConfigDlg");

  for (int i = 0; i < dag_files.size(); ++i)
    mPanel->createStatic(0, (const char *)dag_files[i]);
}

//==============================================================================
DagListDlg::~DagListDlg() {}
