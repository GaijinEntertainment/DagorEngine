// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dagListDlg.h"

#include <oldEditor/de_interface.h>

#include <propPanel/control/container.h>
#include <util/dag_globDef.h>


//==============================================================================
DagListDlg::DagListDlg(Tab<String> &dag_files) : DialogWindow(NULL, hdpi::_pxScaled(240), hdpi::_pxScaled(240), "Clipping DAG files")
{
  mPanel = getPanel();
  G_ASSERT(mPanel && "No panel in CamerasConfigDlg");

  for (int i = 0; i < dag_files.size(); ++i)
    mPanel->createStatic(0, (const char *)dag_files[i]);
}

//==============================================================================
DagListDlg::~DagListDlg() {}
