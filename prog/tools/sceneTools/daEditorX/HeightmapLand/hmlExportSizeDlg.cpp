// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlExportSizeDlg.h"

#include <oldEditor/de_interface.h>
#include <propPanel/control/container.h>

enum
{
  ID_MIN_HEIGHT = 1,
  ID_HEIGHT_RANGE,
  ID_GET_FROM_HEIGHTMAP
};


//==================================================================================================
HmapExportSizeDlg::HmapExportSizeDlg(float &min_height, float &height_range, float min_height_hm, float height_range_hm) :
  minHeight(min_height), heightRange(height_range), minHeightHm(min_height_hm), heightRangeHm(height_range_hm), mDialog(NULL)
{
  mDialog = DAGORED2->createDialog(hdpi::_pxScaled(240), hdpi::_pxScaled(170), "HeightMap dimensions");

  PropPanel::ContainerPropertyControl &panel = *mDialog->getPanel();
  panel.setEventHandler(this);

  panel.createEditFloat(ID_MIN_HEIGHT, "Minimal height:", minHeight);
  panel.createEditFloat(ID_HEIGHT_RANGE, "Height range:", heightRange);
  panel.createButton(ID_GET_FROM_HEIGHTMAP, "Get dimensions from HeightMap");
}


HmapExportSizeDlg::~HmapExportSizeDlg() { del_it(mDialog); }


//==================================================================================================

void HmapExportSizeDlg::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_GET_FROM_HEIGHTMAP)
  {
    minHeight = minHeightHm;
    heightRange = heightRangeHm;

    panel->setFloat(ID_MIN_HEIGHT, minHeight);
    panel->setFloat(ID_HEIGHT_RANGE, heightRange);
  }
}


//==================================================================================================

bool HmapExportSizeDlg::execute()
{
  int ret = mDialog->showDialog();

  if (ret == PropPanel::DIALOG_ID_OK)
  {
    PropPanel::ContainerPropertyControl &panel = *mDialog->getPanel();
    minHeight = panel.getFloat(ID_MIN_HEIGHT);
    heightRange = panel.getFloat(ID_HEIGHT_RANGE);

    return true;
  }

  return false;
}
