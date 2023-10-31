#include "hmlObjectsEditor.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlEntity.h"
#include "hmlSnow.h"
#include "hmlLight.h"
#include <de3_objEntity.h>
#include <libTools/dagFileRW/splineShape.h>
#include <libTools/dagFileRW/dagFileShapeObj.h>
#include <libTools/dagFileRW/dagFileExport.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <dllPluginCore/core.h>
#include <ioSys/dag_memIo.h>
#include <de3_entityFilter.h>
#include <de3_interface.h>
#include <coolConsole/coolConsole.h>
#include <math/dag_bounds3.h>

#include <propPanel2/comWnd/dialog_window.h>
#include <winGuiWrapper/wgw_dialogs.h>

using hdpi::_pxScaled;

enum
{
  PID_STATIC = 1,
  PID_APPLYFORALL,
  PID_CHOISE,
  PID_CHOISE_1,
  PID_CHOISE_2,
  PID_CHOISE_3,

  PID_SEL_ONLY,
  PID_SPLINES,
  PID_POLYGONS,
  PID_ENTITIES,
  PID_GEOM,
  PID_PLUGINS,
};


class ExportImportDlg : public ControlEventHandler // : public IPropertyPanelDlgClient
{
public:
  ExportImportDlg(Tab<String> &sel_names, bool exp_) : selNames(sel_names), exp(exp_), selOnly(false), plugs(tmpmem)
  {
    geoms = 0;

    mDlg = DAGORED2->createDialog(_pxScaled(320), _pxScaled(exp ? 350 : 200), exp ? "Export objects" : "Import objects");
    mPanel = mDlg->getPanel();
    mPanel->setEventHandler(this);

    fillPanel();
  }

  ExportImportDlg::~ExportImportDlg() { DAGORED2->deleteDialog(mDlg); }

  bool showDialog()
  {
    // int ret = dlg->execute();
    // dagGui->deletePropPanelDlg(dlg);
    // return ret == CTL_IDOK;
    if (mDlg->showDialog() == DIALOG_ID_OK)
      onOkPressed();
    else
      return false;

    return true;
  }

  void fillPanel()
  {
    mPanel->createCheckBox(PID_SEL_ONLY, "Selected objects only", false);
    mPanel->createSeparator();
    mPanel->createCheckBox(PID_SPLINES, "Splines", true);
    mPanel->createCheckBox(PID_POLYGONS, "Polygons", true);
    mPanel->createCheckBox(PID_ENTITIES, "Entities", true);

    if (!exp)
      return;

    mPanel->createIndent();
    mPanel->createCheckBox(PID_GEOM, "Geometry:", true);

    IDaEditor3Engine::get().setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, 0);
    int i;
    for (i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
      if (plugin->queryInterface<IGatherStaticGeometry>())
        continue;

      IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

      if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
        filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT, true);
    }

    for (i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

      IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
      if (!geom)
        continue;

      mPanel->createCheckBox(PID_PLUGINS + geoms, plugin->getMenuCommandName(), true);
      plugs.push_back(i);
      geoms++;
    }
  }

  void onChange(int pcb_id, PropPanel2 *panel)
  {
    if (!exp)
      return;

    if (pcb_id == PID_GEOM)
    {
      bool chck = panel->getBool(pcb_id);
      for (int i = 0; i < geoms; i++)
      {
        panel->setBool(PID_PLUGINS + i, chck);
        panel->setEnabledById(PID_PLUGINS + i, chck);
      }
    }
  }

  void onOkPressed()
  {
    if (mPanel->getBool(PID_SPLINES))
      selNames.push_back() = "Splines";

    if (mPanel->getBool(PID_POLYGONS))
      selNames.push_back() = "Polygons";

    if (mPanel->getBool(PID_ENTITIES))
      selNames.push_back() = "Entities";

    selOnly = mPanel->getBool(PID_SEL_ONLY);

    if (exp)
      for (int i = 0; i < geoms; i++)
        if (mPanel->getBool(PID_PLUGINS + i) || mPanel->getBool(PID_GEOM)) // huck, without onPPChange
          selNames.push_back() = DAGORED2->getPlugin(plugs[i])->getMenuCommandName();
  }

public:
  bool selOnly;

private:
  Tab<String> &selNames;
  int geoms;
  bool exp;
  Tab<int> plugs;

  CDialogWindow *mDlg;
  PropertyContainerControlBase *mPanel;
};


void HmapLandObjectEditor::exportAsComposit()
{
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  DataBlock compositBlk;
  compositBlk.setStr("className", "composit");
  DataBlock &blk = *compositBlk.addBlock("node");
  blk.addTm("tm", TMatrix::IDENT);

  Point3 center;
  bool centerSetted = false;
  int exportedCnt = 0;

  int cnt = selection.size();
  for (int i = 0; i < cnt; i++)
  {
    LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(selection[i]);
    if (!o)
      continue;

    IObjEntity *entity = o->getEntity();
    int etype = entity ? entity->getAssetTypeId() : -1;

    if (etype == -1)
    {
      con.addMessage(ILogWriter::ERROR, "unauthorized entity type '%s'... skipping", o->getName());
      continue;
    }

    TMatrix tm = o->getTm();
    if (o->getProps().placeType)
      if (IObjEntity *e = o->getEntity())
        e->getTm(tm);
    if (!centerSetted)
    {
      center = tm.getcol(3);
      centerSetted = true;
    }

    DataBlock &sblk = *blk.addNewBlock("node");
    if (strchr(o->getProps().entityName, ':'))
      sblk.setStr("name", o->getProps().entityName);
    else
      sblk.setStr("name", String(0, "%s:%s", o->getProps().entityName, IDaEditor3Engine::get().getAssetTypeName(etype)));

    tm.setcol(3, tm.getcol(3) - center);
    sblk.setTm("tm", tm);
    if (o->getProps().placeType)
      sblk.setInt("place_type", o->getProps().placeType);

    exportedCnt++;
  }

  Tab<Point3> orig_pts;
  for (int i = 0; i < cnt; i++)
  {
    SplineObject *s = RTTI_cast<SplineObject>(selection[i]);
    if (!s)
      continue;
    if (!centerSetted)
    {
      center = s->points[0]->getPt();
      centerSetted = true;
    }
    int pt_cnt = s->points.size() - (s->isClosed() ? 1 : 0);
    orig_pts.resize(pt_cnt);
    for (int j = 0; j < pt_cnt; j++)
    {
      orig_pts[j] = s->points[j]->getPt();
      s->points[j]->setPos(s->points[j]->getPt() - center);
    }
    s->save(*blk.addNewBlock("node"));
    for (int j = 0; j < pt_cnt; j++)
      s->points[j]->setPos(orig_pts[j]);
  }

  if (!centerSetted)
  {
    wingw::message_box(wingw::MBS_EXCL, "Error", "No entities selected!");
    con.endLog();
    return;
  }

  if (lastAsCompositExportPath.length() <= 0)
    lastAsCompositExportPath.printf(256, "%s/assets/", DAGORED2->getSdkDir());

  String path = wingw::file_save_dlg(NULL, "Select composit file to export",
    "composit files (*.composit.blk)|*.composit.blk|All files (*.*)|*.*", "blk", lastAsCompositExportPath);

  if (path.length())
  {
    lastAsCompositExportPath = path;
    ::dd_mkpath(path);

    if (!compositBlk.saveToTextFile(path))
      con.addMessage(ILogWriter::ERROR, "error while saving file '%s'", path.str());
    else
      con.addMessage(ILogWriter::NOTE, "created composit with %d entities", exportedCnt);
  }

  con.endLog();
}

void HmapLandObjectEditor::exportToDag()
{
  Tab<String> exps(tmpmem);

  ExportImportDlg exportDlg(exps, true);
  if (!exportDlg.showDialog())
    return;

  if (!exps.size())
    return;

  static const int ST_EXPORTABLE = 30;
  PtrTab<RenderableEditableObject> sel_obj;
  Tab<SplineObject *> sel_spl;
  bool exp_selected = exportDlg.selOnly;
  if (exp_selected)
  {
    for (int i = 0; i < splines.size(); ++i)
      if (splines[i] && splines[i]->isSelected())
        sel_spl.push_back(splines[i]);
    sel_obj = selection;
    if (!sel_obj.size() && !sel_spl.size())
    {
      wingw::message_box(wingw::MBS_EXCL, "Error", "No entities selected!");
      return;
    }
  }

  String path = wingw::file_save_dlg(NULL, "Select DAG file to export", "DAG files (*.dag)|*.dag|All files (*.*)|*.*", "dag",
    lastExportPath.length() > 0 ? lastExportPath : DAGORED2->getSdkDir());

  if (path.length())
  {
    ::dd_mkpath(path);
    lastExportPath = path;

    DagSaver *save = dagGeom->newDagSaver(tmpmem);

    if (dagGeom->dagSaverStartSaveDag(*save, path) && dagGeom->dagSaverStartSaveNodes(*save))
    {
      StaticGeometryContainer *cont = dagGeom->newStaticGeometryContainer();

      PtrTab<RenderableEditableObject> &exp_obj = exp_selected ? sel_obj : objects;
      dag::ConstSpan<SplineObject *> exp_spl = exp_selected ? sel_spl : splines;
      Tab<int> sel_obj_st(tmpmem);
      int st_mask_exp = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
      int st_mask_ren = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
      if (exp_selected)
      {
        DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, 1 << ST_EXPORTABLE);
        DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, 1 << ST_EXPORTABLE);
        sel_obj_st.resize(exp_obj.size());
        mem_set_0(sel_obj_st);
        for (int i = 0; i < exp_obj.size(); i++)
          if (LandscapeEntityObject *ent_obj = RTTI_cast<LandscapeEntityObject>(exp_obj[i]))
            if (IObjEntity *e = ent_obj ? ent_obj->getEntity() : NULL)
            {
              sel_obj_st[i] = e->getSubtype();
              e->setSubtype(ST_EXPORTABLE);
            }
      }

      for (int expid = 0; expid < exps.size(); expid++)
      {
        if (exps[expid] == "Splines" || exps[expid] == "Polygons")
        {
          for (int i = 0; i < exp_spl.size(); ++i)
          {
            SplineObject *spline = exp_spl[i];
            if (!spline || spline->points.size() < 1 || !spline->isCreated())
              continue;

            if ((exp_spl[i]->isPoly() && exps[expid] == "Splines") || (!exp_spl[i]->isPoly() && exps[expid] == "Polygons"))
              continue;

            if (dagGeom->dagSaverStartSaveNode(*save, spline->getName(), TMatrix::IDENT))
            {
              DagSpline dagSpline;
              DagSpline *dagSplinePtr = &dagSpline;

              spline->getSpline(dagSpline);

              dagGeom->dagSaverSaveDagSpline(*save, &dagSplinePtr, 1);

              DataBlock blk;
              blk.addBool("isPoly", exp_spl[i]->isPoly());
              blk.addStr("blkGenName", exp_spl[i]->getBlkGenName());
              blk.addInt("modifType", exp_spl[i]->getModifType());
              blk.addBool("polyHmapAlign", exp_spl[i]->isPolyHmapAlign());
              blk.addPoint2("polyObjOffs", exp_spl[i]->getPolyObjOffs());
              blk.addReal("polyObjRot", exp_spl[i]->getPolyObjRot());
              blk.addBool("exportable", exp_spl[i]->isExportable());
              blk.addBool("altGeom", exp_spl[i]->polyGeom.altGeom);
              blk.addReal("bboxAlignStep", exp_spl[i]->polyGeom.bboxAlignStep);
              blk.addInt("cornerType", exp_spl[i]->getProps().cornerType);

              DataBlock *mblk = blk.addNewBlock("modifParams");
              mblk->addReal("width", exp_spl[i]->getProps().modifParams.width);
              mblk->addReal("smooth", exp_spl[i]->getProps().modifParams.smooth);
              mblk->addReal("offsetPow", exp_spl[i]->getProps().modifParams.offsetPow);
              mblk->addPoint2("offset", exp_spl[i]->getProps().modifParams.offset);

              DynamicMemGeneralSaveCB cb(tmpmem);
              if (blk.saveToTextStream(cb))
              {
                Tab<char> splineScript(tmpmem);
                const int buffSize = cb.size();

                append_items(splineScript, buffSize, (const char *)cb.data());
                splineScript.push_back(0);

                save->save_node_script(splineScript.data());
              }

              dagGeom->dagSaverEndSaveNode(*save);
            }
          }
        }
        else if (exps[expid] == "Entities")
        {
          for (int i = 0; i < exp_obj.size(); i++)
          {
            RenderableEditableObject *o = exp_obj[i];

            LandscapeEntityObject *ent_obj = RTTI_cast<LandscapeEntityObject>(o);
            SnowSourceObject *snow_obj = RTTI_cast<SnowSourceObject>(o);
            SphereLightObject *light_obj = RTTI_cast<SphereLightObject>(o);

            if (!ent_obj && !snow_obj && !light_obj)
              continue;

            if (dagGeom->dagSaverStartSaveNode(*save, o->getName(), o->getWtm()))
            {
              DataBlock blk;

              if (ent_obj)
              {
                blk.addStr("entity_name", ent_obj->getProps().entityName);
                blk.addInt("place_type", ent_obj->getProps().placeType);
              }
              else
              {
                DataBlock *props = blk.addBlock("props");

                if (snow_obj)
                {
                  blk.addStr("object_type", "snow");
                  snow_obj->save(*props);
                }
                else if (light_obj)
                {
                  blk.addStr("object_type", "light");
                  light_obj->save(*props);
                }

                props->removeParam("name");
                props->removeParam("tm");
              }

              DynamicMemGeneralSaveCB cb(tmpmem);
              if (blk.saveToTextStream(cb))
              {
                Tab<char> entScript(tmpmem);
                const int buffSize = cb.size();

                append_items(entScript, buffSize, (const char *)cb.data());
                entScript.push_back(0);

                save->save_node_script(entScript.data());
              }

              dagGeom->dagSaverEndSaveNode(*save);
            }
          }
        }
        else
        {
          for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
          {
            IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
            if (strcmp(plugin->getMenuCommandName(), exps[expid]) == 0)
            {
              IExportToDag *exportToDag = plugin->queryInterface<IExportToDag>();
              if (exportToDag)
              {
                exportToDag->gatherExportToDagGeometry(*cont);
                break;
              }

              IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
              if (!geom)
                continue;

              geom->gatherStaticVisualGeometry(*cont);
              break;
            }
          }
        }
      }

      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, st_mask_exp);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, st_mask_ren);
      if (exp_selected && sel_obj_st.size())
        for (int i = 0; i < exp_obj.size(); i++)
          if (LandscapeEntityObject *ent_obj = RTTI_cast<LandscapeEntityObject>(exp_obj[i]))
            if (IObjEntity *e = ent_obj ? ent_obj->getEntity() : NULL)
              e->setSubtype(sel_obj_st[i]);

      cont->writeNodes(save);

      dagGeom->dagSaverEndSaveNodes(*save);

      cont->writeMaterials(save, true);

      dagGeom->deleteStaticGeometryContainer(cont);
      dagGeom->dagSaverEndSaveDag(*save);
    }

    dagGeom->deleteDagSaver(save);
  }
}


void HmapLandObjectEditor::importFromDag()
{
  Tab<String> imps(tmpmem);

  ExportImportDlg importDlg(imps, false);
  if (!importDlg.showDialog())
    return;

  if (!imps.size())
    return;


  String filename = wingw::file_open_dlg(NULL, "Open DAG file", "DAG files (*.dag)|*.dag|All files (*.*)|*.*", "dag",
    lastImportPath.length() > 0 ? lastImportPath : DAGORED2->getSdkDir());


  if (filename.length())
  {
    lastImportPath = filename;

    AScene *sc = dagGeom->newAScene(tmpmem);

    if (dagGeom->loadAscene((char *)filename, *sc, LASF_MATNAMES | LASF_NOMATS | LASF_NOMESHES | LASF_NOLIGHTS, false))
    {
      dagGeom->nodeCalcWtm(*sc->root);
      bool applyForAll = false;
      int choosed = 0;
      getUndoSystem()->begin();
      importFromNode(sc->root, applyForAll, choosed, imps);
      getUndoSystem()->accept("Import splines");
    }
    else
      wingw::message_box(wingw::MBS_EXCL, "Couldn't open DAG file", "Couldn't open file\n\"%s\"", (const char *)filename);

    dagGeom->deleteAScene(sc);
    DAGORED2->repaint();
  }
}


void HmapLandObjectEditor::importFromNode(const Node *node, bool &for_all, int &choosed, Tab<String> &imps)
{
  if (node)
  {
    String newName(node->name);
    bool needToImport = true;

    DataBlock blk;
    blk.loadText(node->script, node->script.length());

    if (node->obj && node->obj->isSubOf(OCID_SHAPEHOLDER))
    {
      const char *typeName = blk.getBool("isPoly", false) ? "Polygons" : "Splines";
      bool is = false;
      for (int i = 0; i < imps.size(); i++)
        if (strcmp(imps[i], typeName) == 0)
        {
          is = true;
          break;
        }

      if (!is)
        needToImport = false;
    }
    else
    {
      bool is = false;
      for (int i = 0; i < imps.size(); i++)
        if (strcmp(imps[i], "Entities") == 0)
        {
          is = true;
          break;
        }

      if (!is)
        needToImport = false;
    }

    if (needToImport && getObjectByName(node->name))
    {
      newName = node->name;

      if (!for_all)
      {
        // ImportDlg importDlg(this, newName, choosed, for_all);
        // if (!importDlg.execute(DAGORED2->getMainWnd()))
        //   needToImport = false;
        CDialogWindow *myDlg = DAGORED2->createDialog(_pxScaled(320), _pxScaled(250), "Import objects");
        PropertyContainerControlBase *myPanel = myDlg->getPanel();

        // fill panel
        String text(260, "Spline object \"%s\" is already exists.", newName.str());
        myPanel->createIndent();
        myPanel->createStatic(PID_STATIC, text);

        PropertyContainerControlBase *gtRadio = myPanel->createRadioGroup(PID_CHOISE, "What do you want?");

        myPanel->createIndent();
        myPanel->createCheckBox(PID_APPLYFORALL, "Apply for all", false);


        String renTo(newName);
        while (getObjectByName(renTo))
          ::make_next_numbered_name(renTo, 3);

        String renToStr(260, "Rename new object to %s", (char *)renTo);
        gtRadio->createRadio(PID_CHOISE_1, renToStr);
        gtRadio->createRadio(PID_CHOISE_2, "Replace old object");
        gtRadio->createRadio(PID_CHOISE_3, "Skip");
        gtRadio->setInt(PID_CHOISE, PID_CHOISE_1);

        if (myDlg->showDialog() == DIALOG_ID_OK)
        {
          for_all = myPanel->getBool(PID_APPLYFORALL);
          choosed = myPanel->getInt(PID_CHOISE) - PID_CHOISE;
        }

        DAGORED2->deleteDialog(myDlg);
      }

      if (needToImport)
      {
        if (choosed == 1)
        {
          while (getObjectByName(newName))
            ::make_next_numbered_name(newName, 2);
        }
        if (choosed == 2)
          removeObject(getObjectByName(node->name));
        else if (choosed == 3)
          needToImport = false;
      }
    }

    if (needToImport)
    {
      if (node->obj && node->obj->isSubOf(OCID_SHAPEHOLDER))
      {
        SplineShape *spline = ((ShapeHolderObj *)node->obj)->shp;
        for (int si = 0; si < spline->spl.size(); ++si)
        {
          DagSpline *dagSpline = spline->spl[si];
          SplineObject *so = new SplineObject(blk.getBool("isPoly", false));
          so->setEditLayerIdx(EditLayerProps::activeLayerIdx[so->lpIndex()]);

          if (dagSpline)
          {
            so->setBlkGenName(blk.getStr("blkGenName", ""));
            so->setModifType(blk.getInt("modifType", 0));
            so->setPolyHmapAlign(blk.getBool("polyHmapAlign", false));
            so->setPolyObjOffs(blk.getPoint2("polyObjOffs", Point2(0, 0)));
            so->setPolyObjRot(blk.getReal("polyObjRot", 0));
            so->setExportable(blk.getBool("exportable", false));
            so->polyGeom.altGeom = blk.getBool("altGeom", false);
            so->polyGeom.bboxAlignStep = blk.getReal("bboxAlignStep", blk.getReal("minGridStep", 1.0f));
            so->setCornerType(blk.getInt("cornerType", 0));

            DataBlock *mblk = blk.getBlockByName("modifParams");
            if (mblk)
              so->loadModifParams(*mblk);

            so->setName(newName);
            addObject(so);

            for (int i = 0; i < dagSpline->knot.size(); ++i)
            {
              const DagSpline::SplineKnot &knot = dagSpline->knot[i];

              SplinePointObject *p = new SplinePointObject;

              Point3 sppoint = knot.p * node->tm;
              Point3 spin = knot.i * node->tm;
              Point3 spout = knot.o * node->tm;

              p->setPos(sppoint);
              p->setRelBezierIn(spin - sppoint);
              p->setRelBezierOut(spout - sppoint);

              p->arrId = i;
              p->spline = so;

              addObject(p);
            }

            so->onCreated(false);
          }
        }
      }
      else
      {
        {
          const char *objType = blk.getStr("object_type", NULL);

          if (!objType)
          {
            const char *entName = blk.getStr("entity_name", "");
            int coll = blk.getInt("place_type", LandscapeEntityObject::Props::PT_coll);

            LandscapeEntityObject *o = new LandscapeEntityObject(entName);
            o->setEditLayerIdx(EditLayerProps::activeLayerIdx[o->lpIndex()]);
            o->setWtm(node->wtm);
            LandscapeEntityObject::Props pr = o->getProps();
            pr.placeType = coll;
            o->setProps(pr);
            addObject(o);
          }
          else
          {
            DataBlock *props = blk.addBlock("props");
            props->setStr("name", node->name);
            props->setTm("tm", node->wtm);

            if (stricmp(objType, "snow") == 0)
            {
              SnowSourceObject *snow_obj = new SnowSourceObject();
              addObject(snow_obj);
              snow_obj->load(*props);
            }
            else if (stricmp(objType, "light") == 0)
            {
              SphereLightObject *light_obj = new SphereLightObject();
              addObject(light_obj);
              light_obj->load(*props);
            }
          }
        }
      }
    }

    for (int i = 0; i < node->child.size(); ++i)
      importFromNode(node->child[i], for_all, choosed, imps);
  }
}
