#include "plugin_occ.h"
#include "occluder.h"
#include <de3_interface.h>
#include <scene/dag_occlusionMap.h>
#include <3d/dag_render.h>
#include <dllPluginCore/core.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_plane3.h>

#include <propPanel2/c_panel_base.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <oldEditor/de_cm.h>


// commands
enum
{
  CM_SET_VIS_DIST = CM_PLUGIN_BASE + 1,
};

enum
{
  PID_PLUGIN_BASE = 300,
};

occplugin::Plugin *occplugin::Plugin::self = NULL;


occplugin::Plugin::Plugin() : disPlugins(midmem), occlBoxes(midmem), occlQuads(midmem)
{
  self = this;
  renderExternalOccluders = false;
  renderExternalOccludersDist = 500;
}


occplugin::Plugin::~Plugin() { self = NULL; }


//==============================================================================
bool occplugin::Plugin::begin(int toolbar_id, unsigned menu_id)
{
  IMenu *mainMenu = DAGORED2->getWndManager()->getMainMenu();
  mainMenu->addItem(menu_id, CM_SET_VIS_DIST, "Implicit occluders visibility...");

  PropertyContainerControlBase *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  objEd.initUi(toolbar_id);

  return true;
}


bool occplugin::Plugin::end()
{
  objEd.closeUi();
  return true;
}


void occplugin::Plugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  objEd.showLocalOccluders = local_data.getBool("showLocalOccluders", objEd.showLocalOccluders);
  objEd.showGlobalOccluders = local_data.getBool("showGlobalOccluders", objEd.showGlobalOccluders);
  renderExternalOccluders = local_data.getBool("renderExternalOccluders", false);
  renderExternalOccludersDist = local_data.getReal("renderExternalOccludersDist", 500);

  const DataBlock &cb = *blk.getBlockByNameEx("disabled_plugins");

  for (int i = 0; i < cb.paramCount(); ++i)
  {
    const char *name = cb.getStr(i);

    if (name && *name)
    {
      bool known = false;

      for (int j = 0; j < disPlugins.size(); ++j)
        if (!stricmp(disPlugins[j], name))
        {
          known = true;
          break;
        }

      if (!known)
        disPlugins.push_back() = name;
    }
  }

  objEd.load(blk);
  objEd.updateToolbarButtons();
}


void occplugin::Plugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  local_data.setBool("showLocalOccluders", objEd.showLocalOccluders);
  local_data.setBool("showGlobalOccluders", objEd.showGlobalOccluders);
  local_data.setBool("renderExternalOccluders", renderExternalOccluders);
  local_data.setReal("renderExternalOccludersDist", renderExternalOccludersDist);

  DataBlock &cb = *blk.addNewBlock("disabled_plugins");

  for (int i = 0; i < disPlugins.size(); ++i)
    cb.addStr("name", disPlugins[i]);

  objEd.save(blk);
}


void occplugin::Plugin::clearObjects()
{
  objEd.removeAllObjects(false);
  objEd.reset();
  renderExternalOccluders = false;
  renderExternalOccludersDist = 500;
}


void occplugin::Plugin::actObjects(float dt)
{
  if (!isVisible)
    return;

  if (DAGORED2->curPlugin() == self)
    objEd.update(dt);
}


void occplugin::Plugin::beforeRenderObjects(IGenViewportWnd *vp)
{
  if (!isVisible)
    return;
  objEd.beforeRender();
}


void occplugin::Plugin::renderObjects()
{
  if (isVisible)
    objEd.objRender(occlBoxes, occlQuads);
}


//==============================================================================
void occplugin::Plugin::renderTransObjects()
{
  if (isVisible)
  {
    objEd.objRenderTr(occlBoxes, occlQuads);
    if (renderExternalOccluders)
    {
      Point3 cpos = dagRender->curView().pos;
      for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
      {
        IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

        if (IOccluderGeomProvider *ogp = plugin->queryInterface<IOccluderGeomProvider>())
          ogp->renderOccluders(cpos, renderExternalOccludersDist);
      }
    }
  }
}

void *occplugin::Plugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  RETURN_INTERFACE(huid, IOccluderGeomProvider);
  return NULL;
}

bool occplugin::Plugin::onPluginMenuClick(unsigned id)
{
  if (id == CM_SET_VIS_DIST)
  {
    CDialogWindow *dlg = DAGORED2->createDialog(hdpi::_pxScaled(300), hdpi::_pxScaled(130), "Set visibility for implicit occluders");
    PropPanel2 &panel = *dlg->getPanel();
    panel.createCheckBox(1, "Show implicit occluders", renderExternalOccluders);
    panel.createEditFloat(2, "Implicit occluders visibility range", renderExternalOccludersDist);

    int ret = dlg->showDialog();
    if (ret == DIALOG_ID_OK)
    {
      renderExternalOccluders = panel.getBool(1);
      renderExternalOccludersDist = panel.getFloat(2);
      DAGORED2->invalidateViewportCache();
    }

    del_it(dlg);
    return true;
  }
  return false;
}

void occplugin::Plugin::fillExportPanel(PropPanel2 &params)
{
  params.createStatic(0, "Get occluders from:");

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

    if (plugin->queryInterface<IOccluderGeomProvider>())
    {
      bool sel = true;
      const char *plugName = plugin->getMenuCommandName();

      for (int j = 0; j < disPlugins.size(); ++j)
        if (!stricmp(disPlugins[j], plugName))
        {
          sel = false;
          break;
        }

      params.createCheckBox(PID_PLUGIN_BASE + i, plugName, sel);
    }
  }
  params.setBoolValue(true);
}

bool occplugin::Plugin::validateBuild(int target, ILogWriter &rep, PropPanel2 *params)
{
  if (!params)
    return true;
  occlBoxes.clear();
  occlQuads.clear();

  disPlugins.clear();
  bool success = true;

  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
    IOccluderGeomProvider *ogp = plugin->queryInterface<IOccluderGeomProvider>();

    if (!ogp)
      continue;
    if (!params->getBool(PID_PLUGIN_BASE + i))
    {
      disPlugins.push_back() = plugin->getMenuCommandName();
      continue;
    }

    int st_box = occlBoxes.size(), st_quad = occlQuads.size();
    ogp->gatherOccluders(occlBoxes, occlQuads);

    if (st_box == occlBoxes.size() && st_quad == occlQuads.size())
    {
      DAEDITOR3.conWarning("no occluders from %s", plugin->getMenuCommandName());
      continue;
    }

    for (TMatrix *m = &occlBoxes[st_box]; st_box < occlBoxes.size(); st_box++, m++)
    {
      if (lengthSq(m->getcol(0)) < 1e-6 || lengthSq(m->getcol(1)) < 1e-6 || lengthSq(m->getcol(2)) < 1e-6)
      {
        DAEDITOR3.conError("degenerate occluder box " FMT_TM " from %s", TMD(*m), plugin->getMenuCommandName());
        success = false;
        erase_items(occlBoxes, st_box, 1);
        st_box--;
        m--;
        continue;
      }
      TMatrix im;
      im = inverse(*m);
      if (im.det() <= 0)
      {
        DAEDITOR3.conError("bad t_box_space.det()=%.3f %~tm (occluder box %~tm from %s)", im.det(), im, *m,
          plugin->getMenuCommandName());
        success = false;
        erase_items(occlBoxes, st_box, 1);
        st_box--;
        m--;
        continue;
      }
    }
    for (Quad *q = &occlQuads[st_quad]; st_quad < occlQuads.size(); st_quad++, q++)
    {
      Plane3 p(q->v[0], q->v[1], q->v[2]);
      if (p.getNormal().length() < 1e-6)
      {
        p.set(q->v[0], q->v[1], q->v[3]);
        if (p.getNormal().length() < 1e-6)
        {
          DAEDITOR3.conError("degenerate occluder quad " FMT_P3 "," FMT_P3 "," FMT_P3 "," FMT_P3 " from %s", P3D(q->v[0]),
            P3D(q->v[1]), P3D(q->v[2]), P3D(q->v[3]), plugin->getMenuCommandName());
          success = false;
          erase_items(occlQuads, st_quad, 1);
          st_quad--;
          q--;
          continue;
        }
      }
      else if (p.distance(q->v[3]) > 1e-3 * p.getNormal().length()) // with normalization for big quads
      {
        DAEDITOR3.conError("non-planar occluder quad " FMT_P3 "," FMT_P3 "," FMT_P3 "," FMT_P3 " dist=%.7f from %s", P3D(q->v[0]),
          P3D(q->v[1]), P3D(q->v[2]), P3D(q->v[3]), p.distance(q->v[3]), plugin->getMenuCommandName());
        success = false;
        erase_items(occlQuads, st_quad, 1);
        st_quad--;
        q--;
        continue;
      }

      Point3 c = (q->v[0] + q->v[1] + q->v[2] + q->v[3]) * 0.25;
      real sm1 = ((q->v[0] - c) % (q->v[1] - c)) * p.getNormal();
      real sm2 = ((q->v[1] - c) % (q->v[2] - c)) * p.getNormal();
      real sm3 = ((q->v[2] - c) % (q->v[3] - c)) * p.getNormal();
      real sm4 = ((q->v[3] - c) % (q->v[0] - c)) * p.getNormal();
      if ((sm1 <= 0 && sm2 <= 0 && sm3 <= 0 && sm4 <= 0) || (sm1 >= 0 && sm2 >= 0 && sm3 >= 0 && sm4 >= 0))
        ; // ok, quad is convex
      else
      {
        DAEDITOR3.conError("non-convex occluder quad " FMT_P3 "," FMT_P3 "," FMT_P3 "," FMT_P3 " "
                           "(%.3f %.3f %.3f %.3f) from %s",
          P3D(q->v[0]), P3D(q->v[1]), P3D(q->v[2]), P3D(q->v[3]), sm1, sm2, sm3, sm4, plugin->getMenuCommandName());
        erase_items(occlQuads, st_quad, 1);
        st_quad--;
        q--;
        success = false;
      }
    }
  }
  DAEDITOR3.conNote("Occlusion data size: %d bytes (%d boxes, %d quads)",
    (sizeof(OcclusionMap::OccluderInfo) + sizeof(OcclusionMap::Occluder)) * (occlBoxes.size() + occlQuads.size()) + 16,
    occlBoxes.size(), occlQuads.size());

  if (!success)
    return false;

  return true;
}

static void cvt_face_le2be(Face *data, int cnt)
{
  using namespace mkbindump;

  while (cnt > 0)
  {
    le2be32_s(&data->v, &data->v, 4);
    le2be16_s(&data->mat, &data->mat, 1);
    data++;
    cnt--;
  }
}

bool occplugin::Plugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *pp)
{
  if (!occlBoxes.size() && !occlQuads.size())
  {
    DAEDITOR3.conWarning("no occluders for export");
    return true;
  }

  if (!occlBoxes.size() && !occlQuads.size())
    return true;

  SmallTab<OcclusionMap::OccluderInfo, TmpmemAlloc> oinfoStorage;
  OcclusionMap::OccluderInfo *__restrict oci;
  OcclusionMap::Occluder occl;
  Plane3 plane;
  TMatrix tmBoxSpace;

  clear_and_resize(oinfoStorage, occlBoxes.size() + occlQuads.size());
  oci = oinfoStorage.data();

  cwr.beginTaggedBlock(_MAKE4C('OCCL'));
  cwr.writeInt32e(0x20080514);
  cwr.writeInt32e(occlBoxes.size() + occlQuads.size());

  for (int bi = 0; bi < occlBoxes.size(); bi++, oci++)
  {
    TMatrix &m = occlBoxes[bi];

    oci->occType = oci->BOX;
    oci->radius2 = m.getcol(0).lengthSq() + m.getcol(1).lengthSq() + m.getcol(2).lengthSq() * 0.25;
    oci->bbox.setempty();
    for (int j = 0; j < 8; j++)
    {
      Point3 p = m * Occluder::normBox.point(j);
      oci->bbox += p;
      memcpy(&occl.box.boxCorners_[j * 3], &p, sizeof(Point3));
    }

    tmBoxSpace = inverse(m);
    memcpy(occl.box.boxSpace_, &tmBoxSpace, sizeof(TMatrix));

    cwr.write32ex(&occl, sizeof(occl));
  }

  for (int qi = 0; qi < occlQuads.size(); qi++, oci++)
  {
    Point3 *v = occlQuads[qi].v;

    oci->occType = oci->QUAD;
    oci->radius2 = 0;
    oci->bbox.setempty();
    for (int j = 0; j < 4; j++)
      oci->bbox += v[j];
    memcpy(&occl.quad.corners_[0 * 3], &v[0], sizeof(Point3));
    memcpy(&occl.quad.corners_[1 * 3], &v[1], sizeof(Point3));
    memcpy(&occl.quad.corners_[2 * 3], &v[3], sizeof(Point3));
    memcpy(&occl.quad.corners_[3 * 3], &v[2], sizeof(Point3));

    Point3 c = oci->bbox.center();
    for (int j = 0; j < 4; j++)
    {
      float d2 = (v[j] - c).lengthSq();
      if (d2 > oci->radius2)
        oci->radius2 = d2;
    }

    plane.set(v[0], v[1], v[2]);
    if (plane.getNormal().length() < 1e-6)
      plane.set(v[0], v[1], v[3]);
    plane.normalize();
    memcpy(occl.quad.plane_, &plane, sizeof(Plane3));

    cwr.write32ex(&occl, sizeof(occl));
  }

  cwr.writeTabData32ex(oinfoStorage);

  cwr.align8();
  cwr.endBlock();
  return true;
}

void occplugin::Plugin::gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<IOccluderGeomProvider::Quad> &occl_quads)
{
  Quad q;

  for (int i = 0, cnt = objEd.objectCount(); i < cnt; i++)
  {
    Occluder *o = RTTI_cast<Occluder>(objEd.getObject(i));
    if (!o)
      continue;
    if (o->getQuad(q.v))
      occl_quads.push_back(q);
    else
      occl_boxes.push_back(o->getWtm());
  }
}
