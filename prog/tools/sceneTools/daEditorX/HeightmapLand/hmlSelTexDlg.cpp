#include "hmlSelTexDlg.h"
#include <dllPluginCore/core.h>
#include <generic/dag_sort.h>
#include <debug/dag_debug.h>

using hdpi::_pxScaled;

enum
{
  PID_BPP_GROUP,
  PID_TEX_LIST,
  PID_IMPORT_TEX,
  PID_CREATE_MASK,
};

static inline int str_compare(const String *s1, const String *s2) { return stricmp(s1->str(), s2->str()); }

//==================================================================================================
HmapLandPlugin::HmlSelTexDlg::HmlSelTexDlg(HmapLandPlugin &plug, const char *selected, int bpp) :
  plugin(plug), selTex(selected), reqBpp(bpp), texNamesCur(NULL)
{
  rebuildTexList(selected);
}

void HmapLandPlugin::HmlSelTexDlg::rebuildTexList(const char *sel)
{
  plugin.updateScriptImageList();
  texNames1.clear();
  texNames8.clear();
  texNames1.push_back() = "<-- reset tex -->";
  texNames8.push_back() = "<-- reset tex -->";
  for (int i = 0; i < plugin.scriptImages.size(); i++)
  {
    switch (HmapLandPlugin::self->getScriptImageBpp(i))
    {
      case 1: texNames1.push_back() = plugin.scriptImages[i]->getName(); break;
      case 8: texNames8.push_back() = plugin.scriptImages[i]->getName(); break;
    }
    if (stricmp(plugin.scriptImages[i]->getName(), sel) == 0)
      reqBpp = HmapLandPlugin::self->getScriptImageBpp(i);
  }
  sort(texNames1, &str_compare);
  sort(texNames8, &str_compare);
  switch (reqBpp)
  {
    case 1: texNamesCur = &texNames1; break;
    case 8: texNamesCur = &texNames8; break;
    default:
      reqBpp = 1;
      texNamesCur = &texNames1;
      break;
  }
}

bool HmapLandPlugin::HmlSelTexDlg::execute()
{
  CDialogWindow *dlg = EDITORCORE->createDialog(_pxScaled(250), _pxScaled(510), "Select mask");
  PropertyContainerControlBase *_panel = dlg->getPanel();
  G_ASSERT(_panel && "HmlSelTexDlg: NO PANEL FOUND!!!");
  _panel->setEventHandler(this);

  PropertyContainerControlBase &bpp_panel = *_panel->createRadioGroup(PID_BPP_GROUP, "Mask depth, bits per pixel");
  bpp_panel.createRadio(1, "1 bit");
  bpp_panel.createRadio(8, "8 bits");
  _panel->createSeparator();

  _panel->createList(PID_TEX_LIST, "Masks", *texNamesCur, selTex);
  //_panel->createButton(PID_IMPORT_TEX, "Import...", true, true);
  _panel->createButton(PID_CREATE_MASK, "Create mask...");
  _panel->setFocusById(PID_TEX_LIST);
  _panel->getById(PID_TEX_LIST)->setHeight(_pxScaled(300));
  _panel->createSeparator();

  _panel->setInt(PID_BPP_GROUP, reqBpp);
  int ret = dlg->showDialog();
  EDITORCORE->deleteDialog(dlg);
  return ret == DIALOG_ID_OK;
}

void HmapLandPlugin::HmlSelTexDlg::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case PID_BPP_GROUP:
      reqBpp = panel->getInt(pcb_id);
      switch (panel->getInt(pcb_id))
      {
        case 1: texNamesCur = &texNames1; break;
        case 8: texNamesCur = &texNames8; break;
        default: texNamesCur = NULL;
      }
      panel->setStrings(PID_TEX_LIST, *texNamesCur);
      panel->setInt(PID_TEX_LIST, selTex.empty() ? 0 : find_value_idx(*texNamesCur, selTex));
      break;
    case PID_TEX_LIST: selTex = panel->getInt(pcb_id) <= 0 ? "" : texNamesCur->at(panel->getInt(pcb_id)); break;
  }
}
void HmapLandPlugin::HmlSelTexDlg::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case PID_IMPORT_TEX:
      if (plugin.importScriptImage(&selTex))
      {
        rebuildTexList(selTex);
        panel->setInt(PID_BPP_GROUP, reqBpp);
        panel->setStrings(PID_TEX_LIST, *texNamesCur);
        panel->setInt(PID_TEX_LIST, selTex.empty() ? 0 : find_value_idx(*texNamesCur, selTex));
      }
      break;

    case PID_CREATE_MASK:
      if (plugin.createMask(reqBpp, &selTex))
      {
        rebuildTexList(selTex);
        panel->setInt(PID_BPP_GROUP, reqBpp);
        panel->setStrings(PID_TEX_LIST, *texNamesCur);
        panel->setInt(PID_TEX_LIST, selTex.empty() ? 0 : find_value_idx(*texNamesCur, selTex));
      }
      break;
  }
}
void HmapLandPlugin::HmlSelTexDlg::onDoubleClick(int pcb_id, PropertyContainerControlBase *panel) {}
