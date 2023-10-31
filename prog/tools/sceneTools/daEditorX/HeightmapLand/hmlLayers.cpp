#include "hmlObjectsEditor.h"
#include "hmlPlugin.h"
#include <de3_interface.h>
#include <de3_hmapService.h>
#include <dllPluginCore/core.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <windows.h>
#undef ERROR
#include "hmlCm.h"
#include <math/dag_adjpow2.h>

using hdpi::_pxScaled;

enum
{
  PIDSTEP_LAYER_PROPS = 7,
  MAX_LAYER_COUNT = EditLayerProps::MAX_LAYERS,
};

enum
{
  PID_ENT_GRP = 1000,
  PID_SPL_GRP,
  PID_PLG_GRP,
  PID_ENT_LAYER,
  PID_SPL_LAYER,
  PID_PLG_LAYER,

  PID_ADD_ENT_LAYER,
  PID_ADD_SPL_LAYER,
  PID_ADD_PLG_LAYER,

  PID_ENT_LAYER_NAME,
  PID_ENT_LAYER_ACTIVE,
  PID_ENT_LAYER_LOCK,
  PID_ENT_LAYER_HIDE,
  PID_ENT_LAYER_RTM,
  PID_ENT_LAYER_SEL,
  PID_ENT_LAYER_ADD,

  PID_SPL_LAYER_NAME = PID_ENT_LAYER_NAME + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT,
  PID_SPL_LAYER_ACTIVE,
  PID_SPL_LAYER_LOCK,
  PID_SPL_LAYER_HIDE,
  PID_SPL_LAYER_RTM,
  PID_SPL_LAYER_SEL,
  PID_SPL_LAYER_ADD,

  PID_PLG_LAYER_NAME = PID_SPL_LAYER_NAME + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT,
  PID_PLG_LAYER_ACTIVE,
  PID_PLG_LAYER_LOCK,
  PID_PLG_LAYER_HIDE,
  PID_PLG_LAYER_RTM,
  PID_PLG_LAYER_SEL,
  PID_PLG_LAYER_ADD,
};

HmapLandObjectEditor::LayersDlg::LayersDlg()
{
  G_STATIC_ASSERT(sizeof(wr) == sizeof(RECT));
  dlg = EDITORCORE->createDialog(_pxScaled(1000), _pxScaled(1080), "Object Layers");
  ::GetWindowRect((HWND)dlg->getHandle(), (RECT *)&wr);
  dlg->setCloseHandler(this);
  dlg->showButtonPanel(false);
}
HmapLandObjectEditor::LayersDlg::~LayersDlg()
{
  EDITORCORE->deleteDialog(dlg);
  dlg = NULL;
}

bool HmapLandObjectEditor::LayersDlg::isVisible() const { return dlg->isVisible(); };
void HmapLandObjectEditor::LayersDlg::show()
{
  const FastNameMapEx &layerNames = EditLayerProps::layerNames;
  const Tab<EditLayerProps> &layerProps = EditLayerProps::layerProps;
  const int *activeLayerIdx = EditLayerProps::activeLayerIdx;

  ::SetWindowPos((HWND)dlg->getHandle(), NULL, wr[0], wr[1], 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
  dlg->show();
  PropertyContainerControlBase &panel = *dlg->getPanel();
  panel.setEventHandler(this);

#define BUILD_LP_CONTROLS(GRP, LPTYPE, PIDPREFIX)                                                                           \
  for (int i = 0; i < layerProps.size(); i++)                                                                               \
  {                                                                                                                         \
    if (layerProps[i].type != LPTYPE)                                                                                       \
      continue;                                                                                                             \
    GRP.createStatic(PIDPREFIX##LAYER_NAME + i * PIDSTEP_LAYER_PROPS, layerProps[i].name());                                \
    GRP.createRadio(PIDPREFIX##LAYER_ACTIVE + i * PIDSTEP_LAYER_PROPS, "active", true, false);                              \
    if (activeLayerIdx[LPTYPE] == i)                                                                                        \
      panel.setInt(PIDPREFIX##LAYER, PIDPREFIX##LAYER_ACTIVE + i * PIDSTEP_LAYER_PROPS);                                    \
    GRP.createCheckBox(PIDPREFIX##LAYER_LOCK + i * PIDSTEP_LAYER_PROPS, "lock", layerProps[i].lock, true, false);           \
    GRP.createCheckBox(PIDPREFIX##LAYER_HIDE + i * PIDSTEP_LAYER_PROPS, "hide", layerProps[i].hide, true, false);           \
    GRP.createCheckBox(PIDPREFIX##LAYER_RTM + i * PIDSTEP_LAYER_PROPS, "to mask", layerProps[i].renderToMask, true, false); \
    GRP.createButton(PIDPREFIX##LAYER_SEL + i * PIDSTEP_LAYER_PROPS, "select objects", true, false);                        \
    GRP.createButton(PIDPREFIX##LAYER_ADD + i * PIDSTEP_LAYER_PROPS, "add to layer", true, false);                          \
    for (; i + 1 < layerProps.size(); i++)                                                                                  \
      if (layerProps[i + 1].type == LPTYPE)                                                                                 \
      {                                                                                                                     \
        GRP.createSeparator(-1);                                                                                            \
        break;                                                                                                              \
      }                                                                                                                     \
  }

  PropertyContainerControlBase &pEntM = *panel.createGroup(PID_ENT_GRP, "Entities");
  PropertyContainerControlBase &pEnt = *pEntM.createRadioGroup(PID_ENT_LAYER, "");
  BUILD_LP_CONTROLS(pEnt, EditLayerProps::ENT, PID_ENT_);
  pEntM.createButton(PID_ADD_ENT_LAYER, "add new layer");

  PropertyContainerControlBase &pSplM = *panel.createGroup(PID_SPL_GRP, "Spline");
  PropertyContainerControlBase &pSpl = *pSplM.createRadioGroup(PID_SPL_LAYER, "");
  BUILD_LP_CONTROLS(pSpl, EditLayerProps::SPL, PID_SPL_);
  pSplM.createButton(PID_ADD_SPL_LAYER, "add new layer");

  PropertyContainerControlBase &pPlgM = *panel.createGroup(PID_PLG_GRP, "Polygons");
  PropertyContainerControlBase &pPlg = *pPlgM.createRadioGroup(PID_PLG_LAYER, "");
  BUILD_LP_CONTROLS(pPlg, EditLayerProps::PLG, PID_PLG_);
  pPlgM.createButton(PID_ADD_PLG_LAYER, "add new layer");

#undef BUILD_LP_CONTROLS
}
void HmapLandObjectEditor::LayersDlg::hide()
{
  if (dlg->getPanel()->getChildCount())
  {
    panelState.reset();
    dlg->getPanel()->saveState(panelState);
    panelState.setInt("pOffset", dlg->getScrollPos());
    dlg->getPanel()->clear();
  }
  dlg->hide();
}
void HmapLandObjectEditor::LayersDlg::refillPanel()
{
  if (dlg->isVisible())
  {
    ::GetWindowRect((HWND)dlg->getHandle(), (RECT *)&wr);
    panelState.reset();
    dlg->getPanel()->saveState(panelState);
    panelState.setInt("pOffset", dlg->getScrollPos());

    dlg->getPanel()->clear();
    show();
  }
}

static void change_active(dag::ConstSpan<EditLayerProps> layerProps, int *activeLayerIdx, int active_idx, unsigned lptype)
{
  if (active_idx < 0 || active_idx >= layerProps.size() || layerProps[active_idx].type != lptype)
    active_idx = lptype;
  activeLayerIdx[lptype] = active_idx;
}

void HmapLandObjectEditor::LayersDlg::onChange(int pid, PropertyContainerControlBase *panel)
{
  int lidx = -1;
  int pid0 = 0;

  if (pid >= PID_ENT_LAYER_ACTIVE && pid < PID_ENT_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    lidx = (pid - PID_ENT_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    pid0 = pid - lidx * PIDSTEP_LAYER_PROPS;
  }
  else if (pid >= PID_SPL_LAYER_ACTIVE && pid < PID_SPL_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    lidx = (pid - PID_SPL_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    pid0 = pid - lidx * PIDSTEP_LAYER_PROPS;
  }
  else if (pid >= PID_PLG_LAYER_ACTIVE && pid < PID_PLG_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    lidx = (pid - PID_PLG_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    pid0 = pid - lidx * PIDSTEP_LAYER_PROPS;
  }
  else if (pid == PID_ENT_LAYER)
  {
    lidx = (panel->getInt(pid) - PID_ENT_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    change_active(EditLayerProps::layerProps, EditLayerProps::activeLayerIdx, lidx, EditLayerProps::ENT);
  }
  else if (pid == PID_SPL_LAYER)
  {
    lidx = (panel->getInt(pid) - PID_SPL_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    change_active(EditLayerProps::layerProps, EditLayerProps::activeLayerIdx, lidx, EditLayerProps::SPL);
  }
  else if (pid == PID_PLG_LAYER)
  {
    lidx = (panel->getInt(pid) - PID_PLG_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    Tab<EditLayerProps> &layerProps = EditLayerProps::layerProps;
    change_active(EditLayerProps::layerProps, EditLayerProps::activeLayerIdx, lidx, EditLayerProps::PLG);
  }

  if (lidx != -1)
  {
    Tab<EditLayerProps> &layerProps = EditLayerProps::layerProps;
    EditLayerProps &l = layerProps[lidx];
    switch (pid0)
    {
      case PID_ENT_LAYER_ACTIVE:
      case PID_SPL_LAYER_ACTIVE:
      case PID_PLG_LAYER_ACTIVE: break;

      case PID_ENT_LAYER_LOCK:
      case PID_SPL_LAYER_LOCK:
      case PID_PLG_LAYER_LOCK:
        if (panel->getBool(pid))
        {
          l.lock = 1;
          HmapLandPlugin::self->selectLayerObjects(lidx, false);
        }
        else
          l.lock = 0;
        break;

      case PID_ENT_LAYER_HIDE:
      case PID_SPL_LAYER_HIDE:
      case PID_PLG_LAYER_HIDE:
        if (panel->getBool(pid))
        {
          l.hide = 1;
          DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << lidx));
        }
        else
        {
          l.hide = 0;
          DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << lidx));
        }
        HmapLandPlugin::hmlService->invalidateClipmap(false);
        break;

      case PID_ENT_LAYER_RTM:
      case PID_SPL_LAYER_RTM:
      case PID_PLG_LAYER_RTM: l.renderToMask = panel->getBool(pid) ? 1 : 0; break;
    }
  }
}
void HmapLandObjectEditor::LayersDlg::onClick(int pid, PropertyContainerControlBase *panel)
{
  FastNameMapEx &layerNames = EditLayerProps::layerNames;
  Tab<EditLayerProps> &layerProps = EditLayerProps::layerProps;

  if (pid == -DIALOG_ID_CLOSE)
  {
    ::GetWindowRect((HWND)dlg->getHandle(), (RECT *)&wr);
    HmapLandPlugin::self->onLayersDlgClosed();
    if (dlg->getPanel()->getChildCount())
    {
      panelState.reset();
      dlg->getPanel()->saveState(panelState);
      panelState.setInt("pOffset", dlg->getScrollPos());
      dlg->getPanel()->clear();
    }
  }
  else if (pid == PID_ADD_ENT_LAYER || pid == PID_ADD_SPL_LAYER || pid == PID_ADD_PLG_LAYER)
  {
    if (EditLayerProps::layerProps.size() >= EditLayerProps::MAX_LAYERS)
    {
      wingw::message_box(wingw::MBS_OK, "Add new layer", "Too many layers created (%d), cannot create one more",
        EditLayerProps::layerProps.size());
      return;
    }
    int lptype = 0;
    if (pid == PID_ADD_ENT_LAYER)
      lptype = EditLayerProps::ENT;
    else if (pid == PID_ADD_SPL_LAYER)
      lptype = EditLayerProps::SPL;
    else if (pid == PID_ADD_PLG_LAYER)
      lptype = EditLayerProps::PLG;

    CDialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(135), "Add layer");
    PropPanel2 *dpan = dialog->getPanel();
    dpan->createEditBox(1, "Enter layer name:");
    dpan->setFocusById(1);
    dpan->createSeparator(-1);
    for (;;) // infinite cycle with explicit break
    {
      int ret = dialog->showDialog();
      if (ret == DIALOG_ID_OK)
      {
        String nm(dpan->getText(1));
        bool bad_name = false;
        if (!nm.length())
          bad_name = true;
        for (const char *p = nm.str(); *p != 0; p++)
          if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
            continue;
          else
          {
            bad_name = true;
            break;
          }
        if (bad_name)
        {
          wingw::message_box(wingw::MBS_OK, "Add new layer",
            "Bad layer name <%s>\n"
            "Name must not be empty and is allowed to content only latin letters, digits or _",
            dpan->getText(1));
          continue;
        }

        int lidx = EditLayerProps::findLayerOrCreate(lptype, layerNames.addNameId(dpan->getText(1)));
        change_active(layerProps, EditLayerProps::activeLayerIdx, lidx, lptype);
        panel->setPostEvent(1);
      }
      break;
    }
    DAGORED2->deleteDialog(dialog);
  }
  else if (pid >= PID_ENT_LAYER_ACTIVE && pid < PID_ENT_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    int lidx = (pid - PID_ENT_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    if (pid == PID_ENT_LAYER_SEL + lidx * PIDSTEP_LAYER_PROPS && !(EditLayerProps::layerProps[lidx].lock))
      HmapLandPlugin::self->selectLayerObjects(lidx);
    else if (pid == PID_ENT_LAYER_ADD + lidx * PIDSTEP_LAYER_PROPS)
      HmapLandPlugin::self->moveObjectsToLayer(lidx);
  }
  else if (pid >= PID_SPL_LAYER_ACTIVE && pid < PID_SPL_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    int lidx = (pid - PID_SPL_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    if (pid == PID_SPL_LAYER_SEL + lidx * PIDSTEP_LAYER_PROPS && !(EditLayerProps::layerProps[lidx].lock))
      HmapLandPlugin::self->selectLayerObjects(lidx);
    else if (pid == PID_SPL_LAYER_ADD + lidx * PIDSTEP_LAYER_PROPS)
      HmapLandPlugin::self->moveObjectsToLayer(lidx);
  }
  else if (pid >= PID_PLG_LAYER_ACTIVE && pid < PID_PLG_LAYER_ACTIVE + PIDSTEP_LAYER_PROPS * MAX_LAYER_COUNT)
  {
    int lidx = (pid - PID_PLG_LAYER_ACTIVE) / PIDSTEP_LAYER_PROPS;
    if (pid == PID_PLG_LAYER_SEL + lidx * PIDSTEP_LAYER_PROPS && !(EditLayerProps::layerProps[lidx].lock))
      HmapLandPlugin::self->selectLayerObjects(lidx);
    else if (pid == PID_PLG_LAYER_ADD + lidx * PIDSTEP_LAYER_PROPS)
      HmapLandPlugin::self->moveObjectsToLayer(lidx);
  }
}
void HmapLandObjectEditor::LayersDlg::onDoubleClick(int pid, PropertyContainerControlBase *panel) {}

void HmapLandPlugin::onLayersDlgClosed() { objEd.setButton(CM_LAYERS_DLG, false); }


FastNameMapEx EditLayerProps::layerNames;
Tab<EditLayerProps> EditLayerProps::layerProps;
int EditLayerProps::activeLayerIdx[EditLayerProps::TYPENUM];

void EditLayerProps::resetLayersToDefauls()
{
  layerNames.reset();
  layerNames.addNameId("default");

  layerProps.resize(EditLayerProps::TYPENUM);
  mem_set_0(layerProps);

  for (int i = 0; i < EditLayerProps::TYPENUM; i++)
  {
    layerProps[i].nameId = 0;
    layerProps[i].type = i;
    layerProps[i].renderToMask = 1;

    activeLayerIdx[i] = i;
  }
}

static const char *lptype_name[] = {"ent", "spl", "plg"};
void EditLayerProps::loadLayersConfig(const DataBlock &blk, const DataBlock &local_data)
{
  const DataBlock *layersBlk = blk.getBlockByName("layers");
  const DataBlock &layersBlk_local = *local_data.getBlockByNameEx("layers");
  for (int i = 0; i < layersBlk->blockCount(); i++)
  {
    const DataBlock &b = *layersBlk->getBlock(i);
    int nid = layerNames.addNameId(b.getBlockName());
    if (nid < 0)
    {
      DAEDITOR3.conError("bad layer name <%s> in %s", b.getBlockName(), "heightmapLand.plugin.blk");
      continue;
    }
    int lidx = findLayerOrCreate(b.getInt("type", 0), nid);

    const DataBlock &b_local =
      *layersBlk_local.getBlockByNameEx(lptype_name[layerProps[lidx].type])->getBlockByNameEx(b.getBlockName());
    layerProps[lidx].lock = b_local.getBool("lock", false);
    layerProps[lidx].hide = b_local.getBool("hide", false);
    layerProps[lidx].renderToMask = b_local.getBool("renderToMask", true);
  }

  activeLayerIdx[0] = findLayer(ENT, layerNames.getNameId(layersBlk_local.getStr("ENT_active", "default")));
  activeLayerIdx[1] = findLayer(SPL, layerNames.getNameId(layersBlk_local.getStr("SPL_active", "default")));
  activeLayerIdx[2] = findLayer(PLG, layerNames.getNameId(layersBlk_local.getStr("PLG_active", "default")));
  for (int i = 0; i < EditLayerProps::TYPENUM; i++)
    if (activeLayerIdx[i] < 0)
      activeLayerIdx[i] = i;

  uint64_t lh_mask = 0;
  for (int i = 0; i < layerProps.size(); i++)
    if (layerProps[i].hide)
      lh_mask |= 1ull << i;
  DAEDITOR3.setEntityLayerHiddenMask(lh_mask);
}
void EditLayerProps::saveLayersConfig(DataBlock &blk, DataBlock &local_data)
{
  if (layerProps.size() != EditLayerProps::TYPENUM ||
      (layerProps[0].renderToMask & layerProps[1].renderToMask & layerProps[2].renderToMask) != 1 ||
      (layerProps[0].lock | layerProps[1].lock | layerProps[2].lock) != 0 ||
      (layerProps[0].hide | layerProps[1].hide | layerProps[2].hide) != 0)
  {
    DataBlock &layersBlk = *blk.addBlock("layers");
    DataBlock &layersBlk_local = *local_data.addBlock("layers");
    for (int i = 0; i < layerProps.size(); i++)
    {
      DataBlock &b = *layersBlk.addNewBlock(layerProps[i].name());
      DataBlock &b_local = *layersBlk_local.addBlock(lptype_name[layerProps[i].type])->addBlock(b.getBlockName());

      b.setInt("type", layerProps[i].type);
      b_local.setBool("lock", layerProps[i].lock);
      b_local.setBool("hide", layerProps[i].hide);
      b_local.setBool("renderToMask", layerProps[i].renderToMask);
    }

    layersBlk_local.setStr("ENT_active", layerProps[activeLayerIdx[0]].name());
    layersBlk_local.setStr("SPL_active", layerProps[activeLayerIdx[1]].name());
    layersBlk_local.setStr("PLG_active", layerProps[activeLayerIdx[2]].name());
  }
}
