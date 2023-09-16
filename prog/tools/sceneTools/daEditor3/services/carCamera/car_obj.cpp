#include "car_phys.h"
#include "car_obj.h"

#include "../../de_appwnd.h"

#include <oldEditor/de_workspace.h>

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetExporter.h>
#include <libTools/util/makeBindump.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <EditorCore/ec_workspace.h>
#include <gameRes/dag_gameResources.h>
#include <de3_entityFilter.h>

#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_cpuFreq.h>

#include <gui/dag_stdGuiRenderEx.h>

#define USE_BULLET_PHYSICS 1
#include <phys/dag_vehicle.h>
#include <vehiclePhys/physCar.h>
#include "carDriver.h"
#undef USE_BULLET_PHYSICS

//------------------------------------------------------------

static unsigned collisionMask = 0;
static unsigned rendGeomMask = 0;

static inline unsigned getRendSubTypeMask() { return IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER); }
static inline int testBits(int in, int bits) { return (in & bits) == bits; }


//------------------------------------------------------------


VehicleViewer::VehicleViewer() : car(NULL), dynPhysObjData(NULL), vpw(NULL)
{
  assetName = "";
  dasset = NULL;
  nowSimulated = false;

  driver = new TestDriver();
  physType = carphyssimulator::getDefaultPhys();

  collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
  rendGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");

  carphyssimulator::init();
}


VehicleViewer::~VehicleViewer()
{
  driver->disable();
  del_it(driver);
  carphyssimulator::close();
}


bool VehicleViewer::begin(IGenViewportWnd *_vpw)
{
  physType = carphyssimulator::getDefaultPhys();

  vpw = _vpw;
  entity = NULL;
  steady = false;
  rwd = fwd = rsa = fsa = false;

  DataBlock appBlk(DAGORED2->getWorkspace().getAppPath());

  const char *cpBlkPath = appBlk.getBlockByNameEx("game")->getStr("car_params", "/game/config/car_params.blk");
  load_car_params_block_from(String(260, "%s%s", DAGORED2->getWorkspace().getAppDir(), cpBlkPath));

  ::dagor_game_time_scale = 1.0;
  car = NULL;

  int vehicle_type = DAEDITOR3.getAssetTypeId("vehicle");
  assetName = DAEDITOR3.selectAssetX(assetName, "Select vehicle asset", "vehicle");
  dasset = DAEDITOR3.getAssetByName(assetName, vehicle_type);

  if (!dasset)
  {
    DAEDITOR3.conError("can't find '%s' asset", assetName.str());
    carphyssimulator::end();
    return false;
  }

  if (!restartSim())
    return false;

  if (!car)
  {
    DAEDITOR3.conError("can't create vehicle from '%s' asset", assetName.str());
    carphyssimulator::end();
    return false;
  }

  const DataBlock &susp = *dasset->props.getBlockByNameEx("suspension");
  fwd = susp.getBlockByNameEx("front")->getBool("powered", true);
  rwd = susp.getBlockByNameEx("rear")->getBool("powered", false);
  fsa = susp.getBlockByNameEx("front")->getBool("rigid_axle", false);
  rsa = susp.getBlockByNameEx("rear")->getBool("rigid_axle", false);

  car->enableAssist(car->CA_STEER, true);

  actObjects(0.01);
  return true;
}


bool VehicleViewer::end()
{
  destroy_it(car);
  carphyssimulator::end();

  nowSimulated = false;
  simulationTime = 0.f;
  ::dagor_game_time_scale = 1.0;

  dasset = NULL;
  destroy_it(entity);

  if (dynPhysObjData)
  {
    release_game_resource((GameResource *)dynPhysObjData);
    dynPhysObjData = NULL;
  }

  return true;
}


bool VehicleViewer::getSelectionBox(BBox3 &box) const
{
  if (!car)
    return false;

  box = car->getBoundBox();
  return true;
}


void VehicleViewer::actObjects(float dt)
{
  if (!car)
    return;


  if (nowSimulated)
  {
    int64_t ref = ref_time_ticks();
    dt *= ::dagor_game_time_scale;
    simulationTime += dt;

    driver->update(car, dt);
    car->updateBeforeSimulate(dt);
    carphyssimulator::simulate(dt);
    car->updateAfterSimulate(dt, simulationTime);

    if (physType == carphyssimulator::PHYS_BULLET)
      IPhysCar::applyCarPhysModeChangesBullet();

    int microsec = get_time_usec(ref);
    static unsigned simTime = 0;
    simTime += microsec;

    bool moveless = car->getVel().length() + car->getAngVel().length() * 2 < 0.03;
    if (moveless && !steady)
      steady = true;
    else
      steady = moveless;
  }
}


void VehicleViewer::beforeRenderObjects()
{
  IPhysCarLegacyRender *carRender = car ? car->getLegacyRender() : nullptr;
  if (!carRender)
    return;
  carphyssimulator::beforeRender();
  carRender->beforeRender();
}


void VehicleViewer::renderObjects()
{
  IPhysCarLegacyRender *carRender = car ? car->getLegacyRender() : nullptr;
  if (!carRender)
    return;
  if (testBits(getRendSubTypeMask(), rendGeomMask))
    carRender->render(carRender->RP_FULL);
}


void VehicleViewer::renderTransObjects()
{
  IPhysCarLegacyRender *carRender = car ? car->getLegacyRender() : nullptr;
  if (!carRender)
    return;
  if (testBits(getRendSubTypeMask(), rendGeomMask))
    carRender->renderTrans(carRender->RP_FULL);
  carphyssimulator::renderTrans(testBits(getRendSubTypeMask(), collisionMask), false, false);
}


void VehicleViewer::renderInfo()
{
  if (!vpw || !car)
    return;

  int w, h;
  vpw->getViewportSize(w, h);

  StdGuiRender::start_render(w, h);

  StdGuiRender::set_font(0);

  renderInfoTest(String(256, "Speed: %.0f km/h", car->getVel().length() * 3.6).str(), Point2(w - 50, h - 100), COLOR_WHITE);

  if (car->getMachinery())
    renderInfoTest(String(256, "Gear step: %d", car->getMachinery()->getGearStep()).str(), Point2(w - 50, h - 75), COLOR_WHITE);

  if (driver)
    renderInfoTest(String(256, "Acceleration: %s", driver->getAccelState() ? "on" : "off").str(), Point2(w - 50, h - 50),
      driver->getAccelState() ? E3DCOLOR_MAKE(255, 75, 75, 255) : COLOR_WHITE);

  StdGuiRender::end_render();
}


void VehicleViewer::renderInfoTest(const char *text, Point2 pos, E3DCOLOR color)
{
  Point2 ts = pos - StdGuiRender::get_str_bbox(text).size();

  Point2 ts_float[4] = {Point2(-1, -1), Point2(-1, 1), Point2(1, -1), Point2(1, 1)};
  StdGuiRender::set_color(COLOR_BLACK);
  for (int i = 0; i < 4; ++i)
    StdGuiRender::draw_strf_to(ts.x + ts_float[i].x, ts.y + ts_float[i].y, text);

  StdGuiRender::set_color(color);
  StdGuiRender::draw_strf_to(ts.x, ts.y, text);
}


bool VehicleViewer::restartSim()
{
  if (!carphyssimulator::begin(NULL, physType))
  {
    nowSimulated = false;
    return false;
  }

  TMatrix camera = TMatrix::IDENT;
  if (vpw)
    vpw->getCameraTransform(camera);

  if (physType == carphyssimulator::PHYS_BULLET)
    car = create_bullet_raywheel_car(dasset->getName(), camera, carphyssimulator::getPhysWorld(), false, false);
  else
    car = NULL;

  if (!car)
  {
    carphyssimulator::end();
    nowSimulated = false;
    return false;
  }

  carphyssimulator::setTargetObj(car->getCarPhysBody(), dasset->props.getStr("physObj", NULL));

  car->setCarPhysMode(IPhysCar::CPM_ACTIVE_PHYS);
  car->getMachinery()->setAutomaticGearBox(true);
  car->setDriver(driver);
  car->postReset();
  driver->resetDriver(car);

  nowSimulated = true;
  return true;
}


void VehicleViewer::getCarTM(TMatrix &tm)
{
  if (car)
    car->getTm(tm);
}
