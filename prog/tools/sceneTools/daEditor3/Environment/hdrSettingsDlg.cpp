#include "hdrSettingsDlg.h"

#include <oldEditor/de_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>


const float MIN_TEX_GAMMA_ADAPTATION = 0.01f;

enum
{
  HDR_RADIO_GROUP,
  HDR_RADIO_NONE,
  HDR_RADIO_FAKE,
  HDR_RADIO_REAL,
  HDR_RADIO_PS3,
  HDR_RADIO_XBLADES,
};


HdrViewSettingsDialog::HdrViewSettingsDialog(DataBlock *hdr_blk, bool if_aces_plugin) :
  hdrBlk(hdr_blk), levelReloadRequired(false), hdrModeNone(false), isAcesEnvironment(if_aces_plugin)
{
  dlg = DAGORED2->createDialog(230, 200, "HDR View Settings");
  G_ASSERT(dlg && "HDR dialog not created!");
  G_ASSERT(hdrBlk && "HDR blk is NULL!");

  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  const DataBlock &blk = *app_blk.getBlockByNameEx("hdr_mode");

  PropPanel2 *panel = dlg->getPanel();

  PropertyContainerControlBase *radio = panel->createRadioGroup(HDR_RADIO_GROUP, "HDR mode:");
  radio->createRadio(HDR_RADIO_NONE, "None");
  radio->createRadio(HDR_RADIO_FAKE, "Fake", blk.getBool("fake", false));
  radio->createRadio(HDR_RADIO_REAL, "Real", blk.getBool("real", false));
  radio->createRadio(HDR_RADIO_PS3, "PS3", blk.getBool("ps3", false));
  if (!isAcesEnvironment)
    radio->createRadio(HDR_RADIO_XBLADES, "X-Blades", blk.getBool("xblades", false));

  int radio_cur = HDR_RADIO_NONE;
  const char *hdrMode = hdrBlk->getStr("mode", "none");
  if (!stricmp(hdrMode, "fake"))
    radio_cur = HDR_RADIO_FAKE;
  else if (!stricmp(hdrMode, "real"))
    radio_cur = HDR_RADIO_REAL;
  else if (!stricmp(hdrMode, "ps3"))
    radio_cur = HDR_RADIO_PS3;
  else if (!stricmp(hdrMode, "xblades") && !isAcesEnvironment)
    radio_cur = HDR_RADIO_XBLADES;

  panel->setInt(HDR_RADIO_GROUP, radio_cur);
}


HdrViewSettingsDialog::~HdrViewSettingsDialog() { del_it(dlg); }


int HdrViewSettingsDialog::showDialog()
{
  G_ASSERT(dlg);
  int res = dlg->showDialog();

  if (res == DIALOG_ID_OK)
  {
    // Write local gameparams.
    PropPanel2 *panel = dlg->getPanel();

    int radio_cur = panel->getInt(HDR_RADIO_GROUP);
    String hdrMode("none");
    switch (radio_cur)
    {
      case HDR_RADIO_NONE: hdrMode = "none"; break;
      case HDR_RADIO_FAKE: hdrMode = "fake"; break;
      case HDR_RADIO_REAL: hdrMode = "real"; break;
      case HDR_RADIO_PS3: hdrMode = "ps3"; break;
      case HDR_RADIO_XBLADES: hdrMode = "xblades"; break;
    }

    hdrBlk->setStr("mode", hdrMode);

    // flags
    levelReloadRequired = stricmp(hdrMode, hdrBlk->getStr("mode", "none"));
    hdrModeNone = (panel->getInt(HDR_RADIO_GROUP) == HDR_RADIO_NONE);
  }

  return res;
}
