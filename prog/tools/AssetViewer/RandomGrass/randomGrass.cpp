#include "../av_plugin.h"
#include "../av_appwnd.h"
#include <assets/asset.h>
#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_control_event_handler.h>
#include <libTools/renderUtil/dynRenderBuf.h>

#include <render/editorGrass.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <generic/dag_initOnDemand.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>


class RandomGrassViewPlugin : public IGenEditorPlugin, public ControlEventHandler
{
public:
  RandomGrassViewPlugin() : rndGrass(NULL), rbuf("editor_opaque_geom_helper_no_tex")
  {
    DataBlock *b = levelGrassBlk.addBlock("layer");

    // alps_village_a:t = "alps_village_a_tex_m"
    // alps_fields_a:t = "alps_fields_a_tex_m"
    // alps_forest_a:t = "alps_forest_a_tex_m"
    b->setPoint4("maskChannel", Point4(0, 0, 1, 0));
    b->setReal("density", 1);
    b->setReal("horScale", 3);
    b->setReal("verScale", 3);
    b->setReal("windMul", 0.05);

    b->setE3dcolor("color_mask_r_from", E3DCOLOR(233, 220, 245, 255));
    b->setE3dcolor("color_mask_r_to", E3DCOLOR(50, 40, 134, 255));

    b->setE3dcolor("color_mask_b_from", E3DCOLOR(106, 121, 45, 127));
    b->setE3dcolor("color_mask_b_to", E3DCOLOR(151, 126, 62, 127));
    b->setE3dcolor("color_mask_g_from", E3DCOLOR(2, 24, 54, 255));
    b->setE3dcolor("color_mask_g_to", E3DCOLOR(247, 242, 186, 255));
  }
  ~RandomGrassViewPlugin() { del_it(rndGrass); }

  virtual const char *getInternalName() const { return "RandomGrassViewer"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    levelGrassBlk.getBlockByName("layer")->setStr("res", asset->getName());
    rndGrass = new EditorGrass(levelGrassBlk, *::dgs_get_game_params()->getBlockByNameEx("GrassSettingsPlanes"));
    rndGrass->isDissolve = true;

    static int water_level_gvid = get_shader_variable_id("water_level", true);
    float waterLevel = ShaderGlobal::get_real_fast(water_level_gvid);
    rndGrass->setWaterLevel(waterLevel);

    return true;
  }
  virtual bool end()
  {
    del_it(rndGrass);
    rbuf.addFaces(BAD_TEXTUREID);
    rbuf.clearBuf();
    return true;
  }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    box[0].set(-20.f, -0.5f, -20.f);
    box[1].set(+20.f, 2.0f, +20.f);
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}
  virtual void renderGeometry(Stage stage)
  {
    if (stage == STG_BEFORE_RENDER)
    {
      struct Helper : IRandomGrassRenderHelper
      {
        virtual bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &tm) { return true; }
        virtual void endRender() {}
        virtual void renderHeight(float min_h, float max_h) { d3d::clearview(CLEAR_ZBUFFER, 0, (0 - min_h) / (max_h - min_h), 0); }
        virtual void renderColor() {}
        virtual void renderMask() { d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 0, 0); }
        virtual void renderExplosions() {}
        virtual bool getHeightmapAtPoint(float x, float y, float &out)
        {
          out = 0;
          return true;
        }
        virtual bool isValid() const { return true; }
      } hlp;
      Point3 viewDir = ::grs_cur_view.itm.getcol(2);
      TMatrix viewTm;
      TMatrix4 projTm, globTm;
      d3d::gettm(TM_VIEW, viewTm);
      d3d::gettm(TM_PROJ, &projTm);
      d3d::calcglobtm(viewTm, projTm, globTm);
      rndGrass->beforeRender(grs_cur_view.pos, hlp, viewDir, viewTm, projTm, Frustum(globTm));
    }
    else if (stage == STG_RENDER_STATIC_OPAQUE)
    {
      rbuf.drawQuad(Point3(-200, 0, -200), Point3(-200, 0, +200), Point3(+200, 0, +200), Point3(+200, 0, -200), E3DCOLOR(8, 12, 5, 0),
        1, -1);

      rbuf.addFaces(BAD_TEXTUREID);
      rbuf.flush();
      rbuf.clearBuf();
    }
    else if (stage == STG_RENDER_DYNAMIC_OPAQUE)
      rndGrass->renderOpaque(false);
    else if (stage == STG_RENDER_DYNAMIC_TRANS)
      rndGrass->renderTrans();
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "rndGrass") == 0; }

  virtual void fillPropPanel(PropertyContainerControlBase &panel) { panel.setEventHandler(this); }

  virtual void postFillPropPanel() {}

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel) {}
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel) {}

private:
  DataBlock levelGrassBlk;
  EditorGrass *rndGrass;
  DynRenderBuffer rbuf;
};

static InitOnDemand<RandomGrassViewPlugin> plugin;


void init_plugin_rndgrass()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
