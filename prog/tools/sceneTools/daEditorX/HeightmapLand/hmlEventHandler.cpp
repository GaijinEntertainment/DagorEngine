// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPanel.h"
#include "hmlCm.h"
#include "hmlHoleObject.h"

#include <de3_interface.h>
#include <de3_hmapService.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/panelWindow.h>

#include <debug/dag_debug.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <osApiWrappers/dag_direct.h>

#include <EASTL/string.h>

using hdpi::_pxScaled;

struct HmapBitLayerDesc;
class IHmapService;


struct BrushIncrementer
{
  real lim, value;
};

static BrushIncrementer brushIncTable[] = {{0, 1}, {10, 5}, {50, 10}, {100, 25}, {200, 50}, {300, 100}};


class ExportLayeesDlg : public PropPanel::ControlEventHandler
{
public:
  static const int PID_LAYER = 1;

  ExportLayeesDlg(HmapLandPlugin *p_, Tab<int> &enabled_) : p(p_), enabled(enabled_)
  {
    mDialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(300), "Export landClass layers");
    PropPanel::ContainerPropertyControl &panel = *mDialog->getPanel();

    dag::ConstSpan<HmapBitLayerDesc> landClsLayer = p->hmlService->getBitLayersList(p->getLayersHandle());
    for (int i = 0; i < landClsLayer.size(); i++)
    {
      panel.createCheckBox(PID_LAYER + i, p->hmlService->getBitLayerName(p->getLayersHandle(), i), true);
    }
  }

  ~ExportLayeesDlg() override { del_it(mDialog); }


  bool execute()
  {
    int ret = mDialog->showDialog();

    if (ret == PropPanel::DIALOG_ID_OK)
    {
      PropPanel::ContainerPropertyControl &panel = *mDialog->getPanel();

      dag::ConstSpan<HmapBitLayerDesc> landClsLayer = p->hmlService->getBitLayersList(p->getLayersHandle());
      for (int i = 0; i < landClsLayer.size(); i++)
      {
        if (panel.getBool(PID_LAYER + i))
          enabled.push_back(i);
      }

      return true;
    }

    return false;
  }

private:
  HmapLandPlugin *p;
  Tab<int> &enabled;
  PropPanel::DialogWindow *mDialog;
};


//==============================================================================

void HmapLandPlugin::refillBrush()
{
  if (propPanel)
    propPanel->fillPanel();
}


bool HmapLandPlugin::onPluginMenuClick(unsigned id)
{
  switch (id)
  {
    case CM_SHOW_PANEL:
      propPanel->showPropPanel(!propPanel->isVisible());
      objEd.setButton(CM_SHOW_PANEL, propPanel->isVisible());
      EDITORCORE->managePropPanels();
      return true;

    case CM_TOGGLE_PROPERTIES_AND_OBJECT_PROPERTIES:
    {
      const bool propertiesPanelShown = propPanel && propPanel->getPanelWindow();

      if (objEd.isPanelShown() && propertiesPanelShown)
      {
        bothPropertiesAndObjectPropertiesWereOpenLastTime = true;

        objEd.showPanel();
        onPluginMenuClick(CM_SHOW_PANEL);
      }
      else if (!objEd.isPanelShown() && !propertiesPanelShown)
      {
        if (bothPropertiesAndObjectPropertiesWereOpenLastTime)
          objEd.showPanel();

        onPluginMenuClick(CM_SHOW_PANEL);
      }
      else
      {
        bothPropertiesAndObjectPropertiesWereOpenLastTime = false;

        objEd.showPanel();
        onPluginMenuClick(CM_SHOW_PANEL);
      }

      return true;
    }

    case CM_CREATE_HEIGHTMAP: createHeightmap(); return true;

    case CM_IMPORT_HEIGHTMAP: importHeightmap(); break;

    case CM_REIMPORT: reimportHeightmap(); break;

    case CM_ERASE_HEIGHTMAP: eraseHeightmap(); break;

    case CM_EXPORT_HEIGHTMAP: exportHeightmap(); break;

    case CM_RESCALE_HMAP:
    {
      applyHmModifiers(false, true, false);

      PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(300), _pxScaled(400), "Rescale heightmap");
      PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
      float mMin = 0, mMax = 0, dMin = 0, dMax = 0;

      mMin = mMax = heightMap.getInitialData(0, 0);
      for (int y = 0, ye = heightMap.getMapSizeY(); y < ye; y++)
        for (int x = 0, xe = heightMap.getMapSizeX(); x < xe; x++)
        {
          float v = heightMap.getInitialData(x, y);
          inplace_min(mMin, v);
          inplace_max(mMax, v);
        }

      if (detDivisor)
      {
        dMin = dMax = heightMapDet.getInitialData(detRectC[0].x, detRectC[0].y);
        for (int y = detRectC[0].y; y < detRectC[1].y; y++)
          for (int x = detRectC[0].x; x < detRectC[1].x; x++)
          {
            float v = heightMapDet.getInitialData(x, y);
            inplace_min(dMin, v);
            inplace_max(dMax, v);
          }
      }
      else
        dMin = mMin, dMax = mMax;

      panel.createStatic(-1, String(0, "Main HMAP min: %.2f m", mMin));
      panel.createStatic(-1, String(0, "Main HMAP max: %.2f m", mMax));
      if (detDivisor)
      {
        panel.createStatic(-1, String(0, "Detailed HMAP min: %.2f m", dMin));
        panel.createStatic(-1, String(0, "Detailed HMAP max: %.2f m", dMax));
      }
      panel.createEditFloat(1, "New min height", min(mMin, dMin));
      panel.createEditFloat(2, "New max height", max(mMax, dMax));
      panel.createCheckBox(3, "Rescale main HMAP", true);
      if (detDivisor)
        panel.createCheckBox(4, "Rescale detailed HMAP", true);
      panel.createCheckBox(5, "Normalize main HMAP", false);
      // panel.createCheckBox(6, "Absolutize main HMAP", false);

      if (dlg->showDialog() == PropPanel::DIALOG_ID_OK)
      {
        float nMin = panel.getFloat(1), nMax = panel.getFloat(2);
        bool rescale_main = panel.getBool(3), rescale_det = panel.getBool(4), norm_main = panel.getBool(5);
        if (rescale_main || rescale_det)
        {
          float oMin = (rescale_main && rescale_det) ? min(mMin, dMin) : (rescale_main ? mMin : (rescale_det ? dMin : 0));
          float oMax = (rescale_main && rescale_det) ? max(mMax, dMax) : (rescale_main ? mMax : (rescale_det ? dMax : 0));
          bool good_range = oMax > oMin;

          if (rescale_det)
            for (int y = detRectC[0].y; y < detRectC[1].y; y++)
              for (int x = detRectC[0].x; x < detRectC[1].x; x++)
                heightMapDet.setInitialData(x, y,
                  good_range ? (heightMapDet.getInitialData(x, y) - oMin) / (oMax - oMin) * (nMax - nMin) + nMin : nMin);

          if (rescale_main)
          {
            if (good_range)
            {
              heightMap.heightScale *= (nMax - nMin) / (oMax - oMin);
              heightMap.heightOffset = nMin + (heightMap.heightOffset - oMin) * (nMax - nMin) / (oMax - oMin);
            }
            else
              heightMap.heightOffset += nMin - oMin;
          }
        }
        if (norm_main && nMax > nMin)
        {
          if (!rescale_main)
            nMin = mMin, nMax = mMax;
          float s = heightMap.heightScale, o = heightMap.heightOffset;
          heightMap.heightScale = nMax - nMin;
          heightMap.heightOffset = nMin;
          for (int y = 0, ye = heightMap.getMapSizeY(); y < ye; y++)
            for (int x = 0, xe = heightMap.getMapSizeX(); x < xe; x++)
              heightMap.setInitialData(x, y, heightMap.getInitialMap().getData(x, y) * s + o);
        }

        if (rescale_main || rescale_det || norm_main)
        {
          if (rescale_main || norm_main)
            refillPanel();
          heightMap.resetFinal();
          heightMapDet.resetFinal();
          applyHmModifiers(true, true, true);
          updateHeightMapTex(false);
          updateHeightMapTex(true);
          DAGORED2->invalidateViewportCache();
          invalidateDistanceField();
        }
      }
      del_it(dlg);
    }
    break;

    case CM_IMPORT_WATER_DET_HMAP: importWaterHeightmap(true); break;
    case CM_IMPORT_WATER_MAIN_HMAP: importWaterHeightmap(false); break;
    case CM_ERASE_WATER_HEIGHTMAPS: eraseWaterHeightmap(); break;

    case CM_EXPORT_COLORMAP: exportColormap(); break;

    case CM_HILL_UP:
    case CM_HILL_DOWN:
    case CM_ALIGN:
    case CM_SMOOTH:
    case CM_SHADOWS:
    case CM_SCRIPT:
      if (id == CM_SHADOWS && !hasLightmapTex)
        break;

      objEd.setEditMode(id);
      objEd.updateToolbarButtons();
      DAGORED2->setBrush((Brush *)brushes[currentBrushId = (id - CM_HILL_UP)]);
      refillBrush();
      DAGORED2->beginBrushPaint();
      DAGORED2->repaint();
      break;

    case CM_REBUILD:
      generateLandColors();
      calcFastLandLighting();
      resetRenderer();
      break;

    case CM_REBUILD_RIVERS: rebuildRivers(); break;

    case CM_BUILD_COLORMAP:
      generateLandColors();
      resetRenderer();
      break;

    case CM_BUILD_LIGHTMAP:
      calcFastLandLighting();
      resetRenderer();
      break;

    case CM_EXPORT_LAND_TO_GAME: exportLand(); break;

    case CM_EXPORT_LOFT_MASKS:
    {
      PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(350), _pxScaled(705), "Loft mask export props");
      PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
      panel.createCheckBox(11, "Build main HMAP loft masks", lastExpLoftMain);
      panel.createEditInt(12, "Main HMAP masks tex size", lastExpLoftMainSz);
      panel.createCheckBox(13, "Use main HMAP area", lastExpLoftUseRect[0]);
      panel.createPoint2(14, "Main Area origin:", lastExpLoftRect[0][0]);
      panel.createPoint2(15, "Main Area Size:", lastExpLoftRect[0].width());
      if (detDivisor)
      {
        panel.createSeparator(0);
        panel.createCheckBox(21, "Build det HMAP loft masks", lastExpLoftDet);
        panel.createEditInt(22, "Det HMAP masks tex size", lastExpLoftDetSz);
        panel.createCheckBox(23, "Use det HMAP area", lastExpLoftUseRect[1]);
        panel.createPoint2(24, "Det Area origin:", lastExpLoftRect[1][0]);
        panel.createPoint2(25, "Det Area Size:", lastExpLoftRect[1].width());
      }

      panel.createSeparator(0);
      float hmin = 0, hmax = 8192;
      if (lastMinHeight[0] < MAX_REAL / 2 && lastHeightRange[0] < MAX_REAL / 2)
        hmin = lastMinHeight[0], hmax = lastMinHeight[0] + lastHeightRange[0];
      if (detDivisor)
      {
        float hmin_det = 0, hmax_det = 8192;
        if (lastMinHeight[1] < MAX_REAL / 2 && lastHeightRange[1] < MAX_REAL / 2)
          hmin_det = lastMinHeight[1], hmax_det = lastMinHeight[1] + lastHeightRange[1];
        inplace_min(hmin, hmin_det);
        inplace_max(hmax, hmax_det);
      }
      panel.createEditFloat(31, "Minimal height", hmin);
      panel.createEditFloat(32, "Height range", hmax - hmin);

      panel.createSeparator(0);
      panel.createCheckBox(51, "Use prefabs to build masks", true);
      panel.createEditInt(52, "Prefabs layer index", 0);

      panel.createSeparator(0);
      panel.createCheckBox(61, "Honour layers' \"to mask\" properties", true);

      String prjLoft;
      DAGORED2->getProjectFolderPath(prjLoft);
      append_slash(prjLoft);
      prjLoft += "loft_masks";

      panel.createSeparator(0);
      panel.createFileEditBox(41, "Out dir", lastExpLoftFolder.empty() ? prjLoft : lastExpLoftFolder);
      panel.setBool(41, true);
      panel.createCheckBox(42, "Create subfolders when areas used", lastExpLoftCreateAreaSubfolders);
      if (dlg->showDialog() == PropPanel::DIALOG_ID_OK)
      {
        lastExpLoftFolder = panel.getText(41);
        lastExpLoftCreateAreaSubfolders = panel.getBool(42);
        lastExpLoftMain = panel.getBool(11);
        lastExpLoftMainSz = panel.getInt(12);
        lastExpLoftDet = panel.getBool(21);
        lastExpLoftDetSz = panel.getInt(22);
        lastExpLoftUseRect[0] = panel.getBool(13);
        lastExpLoftRect[0][0] = panel.getPoint2(14);
        lastExpLoftRect[0][1] = lastExpLoftRect[0][0] + panel.getPoint2(15);
        lastExpLoftUseRect[1] = panel.getBool(23);
        lastExpLoftRect[1][0] = panel.getPoint2(24);
        lastExpLoftRect[1][1] = lastExpLoftRect[1][0] + panel.getPoint2(25);

        G_STATIC_ASSERT(EditLayerProps::MAX_LAYERS <= 64);
        uint64_t prev_layers_hide_mask = DAEDITOR3.getEntityLayerHiddenMask();
        bool honor_layer_props = panel.getBool(61);
        if (!honor_layer_props)
          DAEDITOR3.conNote("%s: ignoring layers' \"to mask\" property", "Loft mask export");
        else
        {
          uint64_t layers_hide_mask = 0;
          for (unsigned i = 0; i < EditLayerProps::layerProps.size(); i++)
          {
            if (!EditLayerProps::layerProps[i].renderToMask)
              layers_hide_mask |= uint64_t(1) << i;
            if (EditLayerProps::layerProps[i].hide && EditLayerProps::layerProps[i].renderToMask)
              DAEDITOR3.conNote("%s: unhiding layer '%s' due to \"to mask\" is ON", "Loft mask export", EditLayerProps::layerName(i));
            else if (!EditLayerProps::layerProps[i].hide && !EditLayerProps::layerProps[i].renderToMask)
              DAEDITOR3.conNote("%s: hiding layer '%s' due to \"to mask\" is OFF", "Loft mask export", EditLayerProps::layerName(i));
          }
          DAEDITOR3.setEntityLayerHiddenMask(layers_hide_mask);
        }

        exportLoftMasks(lastExpLoftFolder, lastExpLoftMain ? lastExpLoftMainSz : 0, lastExpLoftDet ? lastExpLoftDetSz : 0,
          panel.getFloat(31), panel.getFloat(31) + panel.getFloat(32), panel.getBool(51) ? panel.getInt(52) : -1);

        DAEDITOR3.setEntityLayerHiddenMask(prev_layers_hide_mask);
      }
      del_it(dlg);
    }
    break;

    case CM_RESTORE_HM_BACKUP:
      if (hmBackupCanBeRestored())
      {
        if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
              "Do you really want to revert changes\n"
              "(restore backup copy of heightmap)?") == wingw::MB_ID_YES)
          restoreBackup();
      }
      else
      {
        wingw::message_box(wingw::MBS_INFO, "Cannot revert changes", "No backup created yet (maybe nothing changed?)");
      }
      break;

    case CM_COMMIT_HM_CHANGES: hmCommitChanges(); break;

    case CM_EXPORT_LAYERS:
    {
      Tab<int> enabledLayers(tmpmem);
      ExportLayeesDlg dlg(this, enabledLayers);
      if (!dlg.execute())
        break;

      int mapSizeX = landClsMap.getMapSizeX();
      int mapSizeY = landClsMap.getMapSizeY();

      for (int i = 0; i < enabledLayers.size(); i++)
      {
        int idx = enabledLayers[i];
        const char *lname = hmlService->getBitLayerName(landClsLayersHandle, idx);
        HmapBitLayerDesc layer = landClsLayer[idx];

        int bpp = layer.bitCount;
        if (bpp == 1 || bpp == 2)
          ; // leave as is
        else if (bpp <= 4)
          bpp = 4;
        else if (bpp <= 8)
          bpp = 8;
        else
          bpp = 32;

        IBitMaskImageMgr::BitmapMask img;
        if (bpp == 32)
          bitMaskImgMgr->createImage32(img, mapSizeX, mapSizeY);
        else
          bitMaskImgMgr->createBitMask(img, mapSizeX, mapSizeY, bpp);

#define BUILD_MAP(SET, SH)             \
  for (int y = 0; y < mapSizeY; y++)   \
    for (int x = 0; x < mapSizeX; x++) \
      img.SET(x, y, layer.getLayerData(landClsMap.getData(x, y)) << SH);
        switch (bpp)
        {
          case 1: BUILD_MAP(setMaskPixel1, 7); break;
          case 2: BUILD_MAP(setMaskPixel2, 6); break;
          case 4: BUILD_MAP(setMaskPixel4, 4); break;
          case 8: BUILD_MAP(setMaskPixel8, 0); break;
          case 32: BUILD_MAP(setImagePixel, 0); break;
        }
        bitMaskImgMgr->saveImage(img, ".", lname);
        bitMaskImgMgr->destroyImage(img);
#undef BUILD_MAP

        DAEDITOR3.conNote("landclass layer %s exported to %%LEVEL%%/%s.tif", lname, lname);

        if (idx == detLayerIdx && detTexIdxMap)
        {
          bool hasDet[256];
          memset(hasDet, 0, sizeof(hasDet));
          for (int y = 0; y < mapSizeY; y++)
            for (int x = 0; x < mapSizeX; x++)
            {
              uint64_t idx = detTexIdxMap->getData(x, y);
              uint64_t wt = detTexWtMap->getData(x, y);
              while (wt)
              {
                if (!hasDet[idx & 0xFF] && (wt & 0xFF))
                  hasDet[idx & 0xFF] = true;
                idx >>= 8;
                wt >>= 8;
              }
            }

          bitMaskImgMgr->createBitMask(img, mapSizeX, mapSizeY, 8);

          for (int dt = 0; dt < 256; dt++)
          {
            if (!hasDet[dt])
              continue;
            for (int y = 0; y < mapSizeY; y++)
              for (int x = 0; x < mapSizeX; x++)
              {
                uint64_t idx = detTexIdxMap->getData(x, y);
                uint64_t wt = detTexWtMap->getData(x, y);
                while (wt)
                {
                  if ((idx & 0xFF) == dt)
                  {
                    int w = wt & 0xFF;
                    if (w)
                      img.setMaskPixel8(x, y, w);
                    break;
                  }
                  idx >>= 8;
                  wt >>= 8;
                }
              }

            String fn(128, "%s_%d_%s", lname, dt, dd_get_fname(detailTexBlkName[dt].str()));
            bitMaskImgMgr->saveImage(img, ".", fn);
            DAEDITOR3.conNote("landclass layer %s:slot%d exported to %%LEVEL%%/%s.tif", lname, dt, fn.str());
            bitMaskImgMgr->clearBitMask(img);
          }

          bitMaskImgMgr->destroyImage(img);
        }
      }
      DAEDITOR3.conShow(true);
      break;
    }

    case CM_EXPORT_AS_COMPOSIT:
    case CM_SPLIT_COMPOSIT:
    case CM_INSTANTIATE_GENOBJ_INTO_ENTITIES:
    case CM_SPLINE_IMPORT_FROM_DAG:
    case CM_SPLINE_EXPORT_TO_DAG:
    case CM_COLLAPSE_MODIFIERS:
    case CM_MAKE_SPLINES_CROSSES:
    case CM_MOVE_OBJECTS:
    case CM_UNIFY_OBJ_NAMES:
    case CM_HIDE_UNSELECTED_SPLINES:
    case CM_UNHIDE_ALL_SPLINES: objEd.onClick(id, nullptr); return true;

    case CM_SET_PT_VIS_DIST:
    {
      PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(250), _pxScaled(100), "Set visibility range for points");
      PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
      panel.createEditFloat(1, "Visibility range", objEd.maxPointVisDist);
      int ret = dlg->showDialog();

      if (ret == PropPanel::DIALOG_ID_OK)
      {
        objEd.maxPointVisDist = panel.getFloat(1);
        DAGORED2->invalidateViewportCache();
      }

      del_it(dlg);

      break;
    }

    case CM_AUTO_ATACH:
    {
      objEd.autoAttachSplines();
      break;
    }

    case CM_MAKE_BOTTOM_SPLINES: objEd.makeBottomSplines(); break;

    case CM_TAB_PRESS:
    {
      IGenViewportWnd *_vp = DAGORED2->getCurrentViewport();

      if (!brushDlg->isVisible() && _vp && _vp->isActive())
        if (currentBrushId >= 0 && currentBrushId < brushes.size())
        {
          PropPanel::ContainerPropertyControl *_panel = brushDlg->getPanel();
          _panel->clear();
          brushes[currentBrushId]->fillParams(*_panel);
          brushDlg->autoSize();
          brushDlg->centerWindowToMousePos();
          brushDlg->show();
          return true;
        }
    }
    break;

    case CM_TAB_RELEASE:
    {
      if (brushDlg->isVisible())
      {
        // Take away the focus from the dialog so the spin edit can receive the focus lost event, and send onWcChange
        // before the dialog hides if it was edited by keyboard.
        // Make sure the hiding gets called at least one ImGui frame (updateImgui) later.
        // (If we had access to ImGui::GetFrameCount we would store that here and check for inequality in
        // HmapLandPlugin::updateImgui().)
        brushDlgHideRequestFrameCountdown = 2;

        IGenViewportWnd *_vp = DAGORED2->getCurrentViewport();
        if (_vp)
          _vp->activate();
      }
    }
    break;

    case CM_DECREASE_BRUSH_SIZE:
      if (DAGORED2->isBrushPainting() && DAGORED2->getBrush())
      {
        int cnt = sizeof(brushIncTable) / sizeof(BrushIncrementer);
        G_ASSERT(cnt > 0);

        real radius = DAGORED2->getBrush()->getRadius(), incr = 0;

        real cellSz = (real)HmapLandPlugin::self->getHeightmapCellSize() * (real)HmapLandPlugin::self->getHeightmapSizeX() /
                      (real)HmapLandPlugin::self->getlandClassMap().getMapSizeX();

        if (cellSz < 1e-3)
          return true;

        real landClsMapRadius = radius / cellSz;

        for (int i = cnt - 1; i >= 0; i--)
        {
          if (landClsMapRadius > brushIncTable[i].lim)
          {
            incr = brushIncTable[i].value;
            break;
          }
        }

        landClsMapRadius = floor(landClsMapRadius - incr);

        DAGORED2->getBrush()->setRadius(landClsMapRadius * cellSz);
        if (propPanel->getPanelWindow())
          DAGORED2->getBrush()->updateToPanel(*propPanel->getPanelWindow()); // refillBrush();
        DAGORED2->invalidateViewportCache();
      }
      return true;

    case CM_INCREASE_BRUSH_SIZE:
      if (DAGORED2->isBrushPainting() && DAGORED2->getBrush())
      {
        int cnt = sizeof(brushIncTable) / sizeof(BrushIncrementer);
        G_ASSERT(cnt > 0);

        real radius = DAGORED2->getBrush()->getRadius(), incr = 0;

        real cellSz = (real)HmapLandPlugin::self->getHeightmapCellSize() * (real)HmapLandPlugin::self->getHeightmapSizeX() /
                      (real)HmapLandPlugin::self->getlandClassMap().getMapSizeX();

        if (cellSz < 1e-3)
          return true;

        real landClsMapRadius = radius / cellSz;

        for (int i = 0; i < cnt; i++)
        {
          if (landClsMapRadius >= brushIncTable[i].lim)
            incr = brushIncTable[i].value;
        }

        landClsMapRadius = floor(landClsMapRadius + incr);

        DAGORED2->getBrush()->setRadius(landClsMapRadius * cellSz);
        if (propPanel->getPanelWindow())
          DAGORED2->getBrush()->updateToPanel(*propPanel->getPanelWindow()); // refillBrush();
        DAGORED2->invalidateViewportCache();
      }
      return true;
  }

  return false;
}

bool HmapLandPlugin::onSettingsMenuClick(unsigned id)
{
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();

  const unsigned normalized_id = id - settingsBaseId;
  switch (normalized_id)
  {
    case CM_PREFERENCES_PLACE_TYPE_DROPDOWN:
    case CM_PREFERENCES_PLACE_TYPE_RADIO:
    {
      mainMenu->setRadioById(id, settingsBaseId + CM_PREFERENCES_PLACE_TYPE_DROPDOWN,
        settingsBaseId + CM_PREFERENCES_PLACE_TYPE_RADIO);
      const bool isRadio = normalized_id == CM_PREFERENCES_PLACE_TYPE_RADIO;
      if (ObjectEditor::getPlaceTypeRadio() != isRadio)
      {
        ObjectEditor::setPlaceTypeRadio(isRadio);
        objEd.invalidateObjectProps();
      }
      return true;
    }
  }

  return false;
}

void HmapLandPlugin::handleViewportAcceleratorCommand(unsigned id)
{
  if (onPluginMenuClick(id))
    return;

  objEd.onClick(id, nullptr);
}
