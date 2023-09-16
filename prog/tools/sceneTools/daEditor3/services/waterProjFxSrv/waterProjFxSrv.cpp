#include <de3_waterProjFxSrv.h>
#include <de3_interface.h>
#include <oldEditor/de_interface.h>

#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <debug/dag_debug.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>
#include <sepGui/wndGlobal.h>
#include <oldEditor/de_util.h>
#include <dllPluginCore/core.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drvDecl.h>

#include <render/waterProjFx.h>


struct WwaterProjFxRenderHelper : IWwaterProjFxRenderHelper
{
  WwaterProjFxRenderHelper();
  ~WwaterProjFxRenderHelper();
  bool render_geometry();
};


WwaterProjFxRenderHelper::WwaterProjFxRenderHelper() {}

WwaterProjFxRenderHelper::~WwaterProjFxRenderHelper() {}

bool WwaterProjFxRenderHelper::render_geometry()
{
  IRenderingService *prefabMgr;
  IRenderingService *hmap = NULL;

  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
    IRenderingService *iface = p->queryInterface<IRenderingService>();
    if (stricmp(p->getInternalName(), "_prefabEntMgr") == 0)
    {
      prefabMgr = iface;
    }
    else if (stricmp(p->getInternalName(), "heightmapLand") == 0)
    {
      hmap = iface;
    }
  }

  if (prefabMgr)
    prefabMgr->renderGeometry(IRenderingService::STG_RENDER_WATER_PROJ);
  // Reset world tm after prefabMgr
  d3d::settm(TM_WORLD, TMatrix::IDENT);

  if (hmap)
    hmap->renderGeometry(IRenderingService::STG_RENDER_LAND_DECALS);

  return true;
}


class WaterProjFxService : public IWaterProjFxService
{
  WaterProjectedFx *renderer;
  WwaterProjFxRenderHelper renderHelper;
  int frameNo = 0;

public:
  bool srvDisabled;

  WaterProjFxService::WaterProjFxService() : renderer(NULL), srvDisabled(false) {}

  WaterProjFxService::~WaterProjFxService()
  {
    if (renderer != NULL)
    {
      delete renderer;
      renderer = NULL;
    }
  }

  bool initSrv()
  {
    if (!new_shader_material_by_name_optional("combined_water_proj_fx", ""))
      return false;
    init();
    return true;
  }

  virtual void init() { renderer = new WaterProjectedFx(1024, 1024, false); }


  virtual void beforeRender(Stage stage) {}

  virtual void renderGeometry(Stage stage)
  {
    if (!renderer)
      return;
  }


  virtual void render(float waterLevel, float significantWaveHeight)
  {
    if (!renderer)
      return;

    renderer->setTexture();
    TMatrix4 globTm, projTm;
    TMatrix viewTm;
    d3d::gettm(TM_VIEW, viewTm);
    d3d::gettm(TM_PROJ, &projTm);
    d3d::getglobtm(globTm);
    renderer->prepare(viewTm, projTm, globTm, waterLevel, significantWaveHeight, frameNo++);
    renderer->render(&renderHelper);
  }
};


static WaterProjFxService srv;

void setup_water_proj_fx_service(const DataBlock &app_blk)
{
  // srv.srvDisabled = app_blk.getBlockByNameEx("projectDefaults")->getBool("disableGrass", false);
}
void *get_generic_water_proj_fx_service()
{
  if (srv.srvDisabled)
    return NULL;

  static bool is_inited = false;
  if (!is_inited)
  {
    is_inited = true;
    if (!srv.initSrv())
    {
      srv.srvDisabled = true;
      return NULL;
    }
  }
  return &srv;
}
