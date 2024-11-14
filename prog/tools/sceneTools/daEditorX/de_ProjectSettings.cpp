// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_ProjectSettings.h"
#include "de_appwnd.h"
#include "de_plugindata.h"

#include <oldEditor/de_workspace.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_util.h>
#include <oldEditor/de_clipping.h>
#include <de3_entityFilter.h>

#include <libTools/staticGeom/geomObject.h>

#include <osApiWrappers/dag_files.h>
#include <libTools/util/strUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/dtx/dtxHeader.h>
#include <workCycle/dag_gameSettings.h>
#include <debug/dag_debug.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <sepGui/wndGlobal.h>


#define ID_PLUGIN_NAMES 300
#define ID_PLUGIN_PATHS 500


//==============================================================================
// ProjectSettingsDlg
//==============================================================================

ProjectSettingsDlg::ProjectSettingsDlg(void *phandle, bool &use_dir_light) :
  DialogWindow(phandle, hdpi::_pxScaled(550), hdpi::_pxScaled(750), "Project settings")
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "No panel in CamerasConfigDlg");

  PropPanel::ContainerPropertyControl *_tpanel = _panel->createTabPanel(DIALOG_TAB_PANEL, "");
  mTabPage = _tpanel;

  PropPanel::ContainerPropertyControl *_tpage = _tpanel->createTabPage(DIALOG_TAB_LIGHTING, "Lighting");
  lightTab = new PsLightTab(_tpage, use_dir_light);

  _tpage = _tpanel->createTabPage(DIALOG_TAB_COLLISION, "Collision");
  collisionTab = new PsCollisionTab(_tpage);

  _tpage = _tpanel->createTabPage(DIALOG_TAB_EXTERNAL_DATA, "External data source");
  externalTab = new PsExternalTab(_tpage);

  _tpage = _tpanel->createTabPage(DIALOG_TAB_MISC, "Misc");
  miscTab = new MiscTab(_tpage);
}

//==============================================================================

bool ProjectSettingsDlg::onOk()
{
  if (!lightTab->onOkPressed())
    return false;

  if (!collisionTab->onOkPressed())
    return false;

  if (!externalTab->onOkPressed())
    return false;

  if (!miscTab->onOkPressed())
    return false;

  return true;
}


//==============================================================================

long ProjectSettingsDlg::onChanging(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (externalTab)
    externalTab->fixPath(pcb_id);

  return 0;
}


void ProjectSettingsDlg::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (lightTab->handleEvent(pcb_id, panel))
    return;

  if (externalTab->handleEvent(pcb_id, panel))
    return;
}


void ProjectSettingsDlg::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (lightTab)
    lightTab->onApply();
}


//==============================================================================
// PsExternalTab
//==============================================================================

PsExternalTab::PsExternalTab(PropPanel::ContainerPropertyControl *tab_page) : mTabPage(tab_page), oldVals(tmpmem), nowEdit(false)
{
  IDagorEd2Engine *app = IDagorEd2Engine::get();
  Tab<String> filter(tmpmem);
  filter.push_back() = "DaEditorX level (*.level.blk)|*.level.blk";

  for (int i = 0; i < app->getPluginCount(); ++i)
  {
    tab_page->createCheckBox(ID_PLUGIN_NAMES + i, app->getPlugin(i)->getMenuCommandName(),
      (bool)app->getPluginData(i)->externalSource);

    tab_page->createFileEditBox(ID_PLUGIN_PATHS + i, "", app->getPluginData(i)->externalPath,
      (bool)app->getPluginData(i)->externalSource, false);
    tab_page->setStrings(ID_PLUGIN_PATHS + i, filter);

    oldVals.push_back(app->getPluginData(i)->externalPath);
  }
}


//==============================================================================
PsExternalTab::~PsExternalTab() {}

//==============================================================================

bool PsExternalTab::onOkPressed()
{
  IDagorEd2Engine *app = IDagorEd2Engine::get();

  bool changed = false;
  int i = 0;

  for (i = 0; i < oldVals.size(); ++i)
  {
    SimpleString path(mTabPage->getText(ID_PLUGIN_PATHS + i));

    if (stricmp(oldVals[i], path.str()))
    {
      String curProjPath = ::make_full_path(sgg::get_exe_path_full(), app->getSceneFileName());

      curProjPath = ::make_path_relative(curProjPath, de_get_sdk_dir());

      if (stricmp(curProjPath, path.str()))
      {
        changed = true;
        break;
      }
    }
  }

  // compare checks
  if (!changed)
    for (i = 0; i < app->getPluginCount(); i++)
      if ((bool)app->getPluginData(i)->externalSource != mTabPage->getBool(ID_PLUGIN_NAMES + i))
      {
        changed = true;
        break;
      }

  if (!changed)
    return true;

  bool needSave = true;

  switch (wingw::message_box(wingw::MBS_YESNOCANCEL, "Reload project", "Save changed and reload project?"))
  {
    case wingw::MB_ID_CANCEL: return true;

    case wingw::MB_ID_NO: needSave = false;
  }

  for (i = 0; i < app->getPluginCount(); i++)
  {
    if (!app->getPlugin(i)->acceptSaveLoad())
      continue;

    String fn(mTabPage->getText(ID_PLUGIN_PATHS + i));
    bool checked = mTabPage->getBool(ID_PLUGIN_NAMES + i);

    if ((bool)app->getPluginData(i)->externalSource != checked || app->getPluginData(i)->externalPath != fn)
    {
      const char *name = app->getPlugin(i)->getInternalName();
      String dir, fname;

      if (needSave)
        if (!app->getPluginData(i)->externalSource || !app->getPluginData(i)->externalPath.length())
        {
          DataBlock blk(DAGORED2->getPluginFilePath(app->getPlugin(i), name) + ".plugin.blk");
          DataBlock _1;
          app->getPlugin(i)->saveObjects(blk, _1, DAGORED2->getPluginFilePath(app->getPlugin(i), "."));
        }

      app->getPluginData(i)->externalSource = checked;
      app->getPluginData(i)->externalPath = fn;

      app->getPlugin(i)->clearObjects();

      String path(256, "%s/%s/", (const char *)dir, name);
      DataBlock blk(DAGORED2->getPluginFilePath(app->getPlugin(i), name) + ".plugin.blk");

      app->getPlugin(i)->loadObjects(blk, DataBlock(), DAGORED2->getPluginFilePath(app->getPlugin(i), "."));
    }
  }

  return true;
}


//==============================================================================

void PsExternalTab::fixPath(int pcb_id)
{
  IDagorEd2Engine *app = IDagorEd2Engine::get();

  if ((pcb_id >= ID_PLUGIN_PATHS) && (pcb_id < app->getPluginCount() + ID_PLUGIN_PATHS))
  {
    nowEdit = true;

    String path = ::make_full_path(de_get_sdk_dir(), mTabPage->getText(pcb_id).str());
    mTabPage->setText(pcb_id, path);

    nowEdit = false;
  }
}


int PsExternalTab::handleEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  IDagorEd2Engine *app = IDagorEd2Engine::get();
  G_ASSERT(app && "PsExternalTab::handleEvent: app is NULL!!!");

  int plugId = pcb_id - ID_PLUGIN_NAMES;
  int plugCount = app->getPluginCount();

  if ((plugId >= 0) && (plugId < plugCount))
  {
    if (!mTabPage->getBool(pcb_id))
      mTabPage->setEnabledById(ID_PLUGIN_PATHS + plugId, false);
    else
    {
      mTabPage->setEnabledById(ID_PLUGIN_PATHS + plugId, true);
      SimpleString plugPath(mTabPage->getText(ID_PLUGIN_PATHS + plugId));

      if (!nowEdit && !plugPath.length())
      {
        nowEdit = true;
        String path = ::make_full_path(sgg::get_exe_path_full(), app->getSceneFileName());

        path = ::make_path_relative(path, de_get_sdk_dir());

        mTabPage->setText(ID_PLUGIN_PATHS + plugId, path);
        nowEdit = false;
      }
    }

    return 1;
  }

  plugId = pcb_id - ID_PLUGIN_PATHS;

  if (!nowEdit && (plugId >= 0) && (plugId < plugCount))
  {
    nowEdit = true;
    SimpleString plugPath(mTabPage->getText(ID_PLUGIN_PATHS + plugId));

    String path(plugPath);
    path = ::make_path_relative(path, de_get_sdk_dir());

    if (String(plugPath) != path)
      mTabPage->setText(ID_PLUGIN_PATHS + plugId, path);

    nowEdit = false;
    return 1;
  }

  return 0;
}


//==============================================================================
// PsLightTab
//==============================================================================

PsLightTab::PsLightTab(PropPanel::ContainerPropertyControl *tab_page, bool &use_dir_light) // :
  :
  useDirLight(use_dir_light), mTabPage(tab_page)
{
  PropPanel::ContainerPropertyControl *useGr = tab_page->createRadioGroup(USE_RADIO_GROUP, "Light source:");

  useGr->createRadio(USE_DIR_RADIO_BUTTON, "Use direct light");
  useGr->createRadio(USE_LIGHT_RADIO_BUTTON, "Use light from \"Light\" plugin");
  tab_page->setInt(USE_RADIO_GROUP, (useDirLight) ? USE_DIR_RADIO_BUTTON : USE_LIGHT_RADIO_BUTTON);

  prevUseDirLight = useDirLight;

  tab_page->createSeparator(0);
  tab_page->createStatic(0, "Direct light settings:");

  real mul = 1;
  GeomObject::getSkyLight(ambient, mul);

  tab_page->createSimpleColor(AMBIENT_COLOR_ID, "Ambient color (%):", E3DCOLOR(ambient.r, ambient.g, ambient.b));
  tab_page->createTrackInt(AMBIENT_TRACK_ID, "", mul * 100, 0, 100, 1, true, false);

  GeomObject::getDirectLight(direct, mul);

  tab_page->createSimpleColor(DIRECT_COLOR_ID, "Direct color (%):", E3DCOLOR(direct.r, direct.g, direct.b));
  tab_page->createTrackFloat(DIRECT_TRACK_ID, "", mul * 100, 0, 1000.0, 1, true, false);

  real zenith, azimuth;
  GeomObject::getLightAngles(zenith, azimuth);

  tab_page->createStatic(0, "Light direction (zenith and azimuth):");
  tab_page->createPoint2(ZENITH_AZIMUTH_ID, "", Point2(RadToDeg(zenith), RadToDeg(azimuth)), 2, true, false);

  tab_page->createIndent();
  tab_page->createButton(APPLY_BUTTON_ID, "Apply");
  tab_page->createStatic(0, "", false);
  tab_page->createStatic(0, "", false);

  disableControls(mTabPage->getInt(USE_RADIO_GROUP) == USE_DIR_RADIO_BUTTON);
}


//==============================================================================
PsLightTab::~PsLightTab() {}

//==============================================================================
real PsLightTab::getAmbientMul() const { return 1.0 * mTabPage->getInt(AMBIENT_TRACK_ID) / 100.0; }


//==============================================================================
real PsLightTab::getDirectMul() const { return 1.0 * mTabPage->getFloat(DIRECT_TRACK_ID) / 100.0; }


//==============================================================================
Point2 PsLightTab::getLightDirection() const { return mTabPage->getPoint2(ZENITH_AZIMUTH_ID); }


//==============================================================================
void PsLightTab::disableControls(bool light)
{
  mTabPage->setEnabledById(AMBIENT_TRACK_ID, light);
  mTabPage->setEnabledById(AMBIENT_COLOR_ID, light);
  mTabPage->setEnabledById(DIRECT_TRACK_ID, light);
  mTabPage->setEnabledById(DIRECT_COLOR_ID, light);
  mTabPage->setEnabledById(ZENITH_AZIMUTH_ID, light);
}

//==============================================================================

bool PsLightTab::onOkPressed()
{
  useDirLight = (mTabPage->getInt(USE_RADIO_GROUP) == USE_DIR_RADIO_BUTTON);
  return true;
}

//==============================================================================

void PsLightTab::onApply()
{
  bool lightChanged = false;
  bool use_dir = (mTabPage->getInt(USE_RADIO_GROUP) == USE_DIR_RADIO_BUTTON);

  if (prevUseDirLight != use_dir)
  {
    lightChanged = true;
    prevUseDirLight = use_dir;
  }

  if (use_dir)
  {
    E3DCOLOR color;
    real mul;

    GeomObject::getSkyLight(color, mul);

    if (getAmbientColor() != color || fabsf(mul - getAmbientMul()) > 0.001)
      lightChanged = true;

    GeomObject::getDirectLight(color, mul);

    if (getDirectColor() != color || fabsf(mul - getDirectMul()) > 0.001)
      lightChanged = true;

    GeomObject::setAmbientLight(getAmbientColor(), getAmbientMul(), 0, 0);
    GeomObject::setDirectLight(getDirectColor(), getDirectMul());

    Point2 ang = getLightDirection();
    Point2 ang2;
    GeomObject::getLightAngles(ang2.x, ang2.y);

    if (ang2 != ang2)
      lightChanged = true;

    GeomObject::setLightAnglesDeg(ang.x, ang.y);
  }

  if (lightChanged)
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.startProgress();
    GeomObject::recompileAllGeomObjects(&con);
    con.endProgress();

    DAGORED2->repaint();

    for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      IGenEditorPlugin *plug = DAGORED2->getPlugin(i);

      if (!plug)
        continue;

      ILightingChangeClient *lc = plug->queryInterface<ILightingChangeClient>();

      if (lc)
        lc->onLightingChanged();
    }
  }
}


//==============================================================================
int PsLightTab::handleEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case AMBIENT_COLOR_ID: ambient = mTabPage->getColor(pcb_id); return 1;

    case DIRECT_COLOR_ID: direct = mTabPage->getColor(pcb_id); return 1;

    case USE_RADIO_GROUP: disableControls(mTabPage->getInt(USE_RADIO_GROUP) == USE_DIR_RADIO_BUTTON); return 1;
  }

  return 0;
}


//==============================================================================
// MiscTab
//==============================================================================

MiscTab::MiscTab(PropPanel::ContainerPropertyControl *tab_page) : mTabPage(tab_page)
{
  PropPanel::ContainerPropertyControl *texQualityGr = tab_page->createRadioGroup(TEXQUALITY_RADIO_GRP_ID, "Textures quality:");
  texQualityGr->createRadio(TEXQUALITY_RADIO_H_ID, "High");
  texQualityGr->createRadio(TEXQUALITY_RADIO_M_ID, "Medium");
  texQualityGr->createRadio(TEXQUALITY_RADIO_L_ID, "Low");
  texQualityGr->createRadio(TEXQUALITY_RADIO_UL_ID, "Ultra low");

  PropPanel::ContainerPropertyControl *lightmapQualityGr = tab_page->createRadioGroup(LT_RADIO_GRP_ID, "Lightmap quality:", false);
  lightmapQualityGr->createRadio(LT_RADIO_H_ID, "High");
  lightmapQualityGr->createRadio(LT_RADIO_M_ID, "Medium");
  lightmapQualityGr->createRadio(LT_RADIO_L_ID, "Low");
  lightmapQualityGr->createRadio(LT_RADIO_UL_ID, "Ultra low");

  PropPanel::ContainerPropertyControl *waterQualityGr = tab_page->createRadioGroup(WATER_RADIO_GRP_ID, "Water quality:");
  waterQualityGr->createRadio(WATER_RADIO_H_ID, "High");
  waterQualityGr->createRadio(WATER_RADIO_M_ID, "Medium");

  PropPanel::ContainerPropertyControl *staticBumpGr = tab_page->createRadioGroup(BUMP_RADIO_GRP_ID, "Static bump:", false);
  staticBumpGr->createRadio(BUMP_RADIO_ON_ID, "On");
  staticBumpGr->createRadio(BUMP_RADIO_OFF_ID, "Off");


  switch (::dgs_tex_quality)
  {
    case ddstexture::quality_High: tab_page->setInt(TEXQUALITY_RADIO_GRP_ID, TEXQUALITY_RADIO_H_ID); break;
    case ddstexture::quality_Medium: tab_page->setInt(TEXQUALITY_RADIO_GRP_ID, TEXQUALITY_RADIO_M_ID); break;
    case ddstexture::quality_Low: tab_page->setInt(TEXQUALITY_RADIO_GRP_ID, TEXQUALITY_RADIO_L_ID); break;
    case ddstexture::quality_UltraLow: tab_page->setInt(TEXQUALITY_RADIO_GRP_ID, TEXQUALITY_RADIO_UL_ID); break;
    default: wingw::message_box(wingw::MBS_EXCL, "Contact developer", "Unknown textures quality value %i.", dgs_tex_quality);
  }
}


bool MiscTab::onOkPressed()
{
  int old = ::dgs_tex_quality;

  switch (mTabPage->getInt(TEXQUALITY_RADIO_GRP_ID))
  {
    case TEXQUALITY_RADIO_H_ID: ::dgs_tex_quality = ddstexture::quality_High; break;

    case TEXQUALITY_RADIO_M_ID: ::dgs_tex_quality = ddstexture::quality_Medium; break;

    case TEXQUALITY_RADIO_L_ID: ::dgs_tex_quality = ddstexture::quality_Low; break;

    case TEXQUALITY_RADIO_UL_ID: ::dgs_tex_quality = ddstexture::quality_UltraLow; break;
  }

  bool setShaderVar = false;
  bool mbShown = false;

  if (old != ::dgs_tex_quality)
  {
    wingw::message_box(wingw::MBS_INFO, "Restart Editor", "Save project and restart Editor to changes take effect.");
    mbShown = true;
  }

  return true;
}


//==============================================================================
// PsCollisionTab
//==============================================================================

PsCollisionTab::PsCollisionTab(PropPanel::ContainerPropertyControl *tab_page) : mTabPage(tab_page), mColNames(midmem)
{
  tab_page->createStatic(0, "Custom colliders  ");
  const int colCnt = ::get_custom_colliders_count();
  int _cntr = 0;

  for (int i = 0; i < colCnt; ++i)
  {
    const IDagorEdCustomCollider *collider = ::get_custom_collider(i);

    if (collider)
    {
      const char *colName = collider->getColliderName();
      mColNames.push_back() = colName;

      tab_page->createCheckBox(COLLIDER_CHECK_BOX_ID + _cntr, colName, ::is_custom_collider_enabled(collider));

      ++_cntr;
    }
  }

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
    IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

    if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
    {
      const char *colName = plugin->getMenuCommandName();
      mColNames.push_back() = colName;

      tab_page->createCheckBox(COLLIDER_CHECK_BOX_ID + _cntr, colName,
        filter->isFilteringActive(IObjEntityFilter::STMASK_TYPE_COLLISION));

      ++_cntr;
    }
  }
}

//==================================================================================================

bool PsCollisionTab::onOkPressed()
{
  for (int i = 0; i < mColNames.size(); ++i)
    if (mTabPage->getBool(COLLIDER_CHECK_BOX_ID + i))
      ::enable_custom_collider(mColNames[i]);
    else
      ::disable_custom_collider(mColNames[i]);

  return true;
}
