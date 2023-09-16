#undef USE_BULLET_PHYSICS

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_critSec.h>

#include <vehiclePhys/physCarGameRes.h>
#include "physCarData.h"
#include <gameRes/dag_dumpResRefCountImpl.h>

float physcarprivate::globalFrictionMul = 1.0;
float physcarprivate::globalFrictionVDecay = 0.0;
DataBlock *physcarprivate::globalCarParamsBlk = NULL;

static const char *global_car_params_blk_fn = "config/car_params.blk";

class VehicleDescGameResFactory : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId, refCount;
    PhysCarSettings2 *carData;

    ResData() : resId(-1), refCount(0) { carData = new PhysCarSettings2; }
    ~ResData() { delete carData; }
  };

  Tab<ResData> resData;


  VehicleDescGameResFactory() : resData(midmem) {}


  int findRes(int res_id)
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameRes(GameResource *res)
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].carData == (PhysCarSettings2 *)res)
        return i;

    return -1;
  }


  virtual unsigned getResClassId() { return VehicleDescGameResClassId; }

  virtual const char *getResClassName() { return "VehicleDesc"; }

  virtual bool isResLoaded(int res_id) { return findRes(res_id) >= 0; }
  virtual bool checkResPtr(GameResource *res) { return findGameRes(res) >= 0; }

  virtual GameResource *getGameResource(int res_id)
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0)
      return NULL;

    resData[id].refCount++;

    return (GameResource *)resData[id].carData;
  }


  virtual bool addRefGameResource(GameResource *resource)
  {
    int id = findGameRes(resource);
    if (id < 0)
      return false;

    resData[id].refCount++;
    return true;
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    if (resData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(resData[id].resId, name);
      logerr("Attempt to release PhysCar resource %s with 0 refcount", (char *)name);
      return;
    }

    resData[id].refCount--;
  }

  virtual void releaseGameResource(int res_id)
  {
    int id = findRes(res_id);
    delRef(id);
  }

  virtual bool releaseGameResource(GameResource *resource)
  {
    int id = findGameRes(resource);
    delRef(id);
    return id >= 0;
  }


  virtual bool freeUnusedResources(bool force, bool /*once*/)
  {
    bool result = false;
    for (int i = resData.size() - 1; i >= 0; --i)
    {
      if (resData[i].refCount != 0)
        continue;
      if (get_refcount_game_resource_pack_by_resid(resData[i].resId) > 0)
        continue;

      erase_items(resData, i, 1);
      result = true;
    }

    return result;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    int id = findRes(res_id);
    if (id >= 0)
      return;

    ResData &rrd = resData.push_back();
    lock.unlockFinal();

    int version = cb.readInt();
    if (version != rrd.carData->RES_VERSION)
      fatal("Invalid Vehicle resource version %d (%d required)", version, rrd.carData->RES_VERSION);

    cb.read(rrd.carData, sizeof(*rrd.carData));
    rrd.resId = res_id;
  }


  void createGameResource(int /*res_id*/, const int * /*ref_ids*/, int /*num_refs*/) override {}
  void reset() override
  {
    if (!resData.empty())
    {
      logerr("%d leaked VehicleDesc", resData.size());
      String name;
      for (auto &r : resData)
      {
        get_game_resource_name(r.resId, name);
        logerr("    '%s', refCount=%d", name.str(), r.refCount);
      }
    }
    clear_and_shrink(resData);
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resData, resId, refCount)
};
DAG_DECLARE_RELOCATABLE(VehicleDescGameResFactory::ResData);

class VehicleGameResFactory : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId;
    int refCount;

    PhysCarCreationData *carCreationData;

    ResData() : resId(-1), refCount(0) { carCreationData = new PhysCarCreationData; }

    ~ResData()
    {
      if (carCreationData->carDataPtr)
        release_game_resource((GameResource *)carCreationData->carDataPtr);
      if (carCreationData->bodyPhObjData)
        release_game_resource((GameResource *)carCreationData->bodyPhObjData);
      if (carCreationData->frontWheelModel)
        release_game_resource((GameResource *)(carCreationData->frontWheelModel.get()));
      if (carCreationData->rearWheelModel)
        release_game_resource((GameResource *)(carCreationData->rearWheelModel.get()));

      delete carCreationData;
    }
  };

  Tab<ResData> resData;


  VehicleGameResFactory() : resData(midmem) {}


  int findRes(int res_id)
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameRes(GameResource *res)
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].carCreationData == (PhysCarCreationData *)res)
        return i;

    return -1;
  }


  virtual unsigned getResClassId() { return VehicleGameResClassId; }

  virtual const char *getResClassName() { return "Vehicle"; }

  virtual bool isResLoaded(int res_id) { return findRes(res_id) >= 0; }
  virtual bool checkResPtr(GameResource *res) { return findGameRes(res) >= 0; }

  virtual GameResource *getGameResource(int res_id)
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0)
      return NULL;

    resData[id].refCount++;

    return (GameResource *)resData[id].carCreationData;
  }


  virtual bool addRefGameResource(GameResource *resource)
  {
    int id = findGameRes(resource);
    if (id < 0)
      return false;

    resData[id].refCount++;
    return true;
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    if (resData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(resData[id].resId, name);
      logerr("Attempt to release PhysCar resource %s with 0 refcount", (char *)name);
      return;
    }

    resData[id].refCount--;
  }

  virtual void releaseGameResource(int res_id)
  {
    int id = findRes(res_id);
    delRef(id);
  }

  virtual bool releaseGameResource(GameResource *resource)
  {
    int id = findGameRes(resource);
    delRef(id);
    return id >= 0;
  }


  virtual bool freeUnusedResources(bool force, bool /*once*/)
  {
    bool result = false;
    for (int i = resData.size() - 1; i >= 0; --i)
    {
      if (resData[i].refCount != 0)
        continue;
      if (get_refcount_game_resource_pack_by_resid(resData[i].resId) > 0)
        continue;

      erase_items(resData, i, 1);
      result = true;
    }

    return result;
  }


  void loadGameResourceData(int /*res_id*/, IGenLoad & /*cb*/) override {}


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    if (findRes(res_id) >= 0)
      return;

    G_ASSERT(num_refs >= 4);

    int rdi = append_items(resData, 1);
    lock.unlockFinal();

    G_ASSERT((unsigned int)::get_game_res_class_id(ref_ids[3]) == VehicleDescGameResClassId);
    resData[rdi].carCreationData->carDataPtr = (PhysCarSettings2 *)::get_game_resource(ref_ids[3]);

    G_ASSERT((unsigned int)::get_game_res_class_id(ref_ids[0]) == PhysObjGameResClassId);
    resData[rdi].carCreationData->bodyPhObjData = (DynamicPhysObjectData *)::get_game_resource(ref_ids[0]);
    G_ASSERT(resData[rdi].carCreationData->bodyPhObjData);

#ifndef NO_3D_GFX
    G_ASSERT((unsigned int)::get_game_res_class_id(ref_ids[1]) == DynModelGameResClassId);
    resData[rdi].carCreationData->frontWheelModel = (DynamicRenderableSceneLodsResource *)::get_game_resource(ref_ids[1]);

    G_ASSERT((unsigned int)::get_game_res_class_id(ref_ids[2]) == DynModelGameResClassId);
    resData[rdi].carCreationData->rearWheelModel = (DynamicRenderableSceneLodsResource *)::get_game_resource(ref_ids[2]);
#endif
    resData[rdi].resId = res_id;
  }

  void reset() override
  {
    if (!resData.empty())
    {
      logerr("%d leaked Vehicle", resData.size());
      String name;
      for (auto &r : resData)
      {
        get_game_resource_name(r.resId, name);
        logerr("    '%s', refCount=%d", name.str(), r.refCount);
      }
    }
    clear_and_shrink(resData);
  }
  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resData, resId, refCount)
};
DAG_DECLARE_RELOCATABLE(VehicleGameResFactory::ResData);


static InitOnDemand<VehicleDescGameResFactory> factory;
static InitOnDemand<VehicleGameResFactory> factory2;

void register_phys_car_gameres_factory()
{
  factory.demandInit();
  factory2.demandInit();
  ::add_factory(factory);
  ::add_factory(factory2);
}


bool load_raywheel_car_info(const char *res_name, const char *car_name, class PhysCarParams *info, class PhysCarGearBoxParams *gbox,
  class PhysCarDifferentialParams *diffs_front_and_rear[2], class PhysCarCenterDiffParams *center_diff)
{
  const DataBlock *b, &carsBlk = physcarprivate::load_car_params_data_block(car_name, b);

  if (info)
  {
    String descResName = String(res_name) + "_vehicleDesc";
    GameResource *descRes = get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(descResName), VehicleDescGameResClassId);
    if (!descRes)
      fatal("Error loading vehicle description resource '%s'", descResName.str());

    PhysCarSettings2 *carSettings = (PhysCarSettings2 *)descRes;

    info->setDefaultValues(carSettings->frontSusp, carSettings->rearSusp);
    info->load(b, carsBlk);

    release_game_resource(descRes);
  }

  if (gbox)
  {
    const DataBlock *gbBlk = carsBlk.getBlockByNameEx("gearboxes")->getBlockByName(b->getStr("gearbox", "standard"));

    if (!gbBlk)
      fatal("can't load gearbox preset <%s>", b->getStr("gearbox", "standard"));

    gbox->load(gbBlk);
  }

  if (diffs_front_and_rear)
  {
    diffs_front_and_rear[0]->load(*b->getBlockByNameEx("diffFront"));
    diffs_front_and_rear[1]->load(*b->getBlockByNameEx("diffRear"));
  }

  if (center_diff)
    center_diff->load(*b->getBlockByNameEx("diffCenter"));

  return true;
}

void get_raywheel_car_def_wheels(const char *car_name, String *front_and_rear[2])
{
  const DataBlock *b, &carsBlk = physcarprivate::load_car_params_data_block(car_name, b);

  const char *tirePreset = b->getStr("tire", NULL);
  const char *ftirePreset = b->getStr("front_tire", tirePreset);
  const char *rtirePreset = b->getStr("rear_tire", tirePreset ? tirePreset : ftirePreset);
  G_ASSERT(ftirePreset);
  G_ASSERT(rtirePreset);
  (*front_and_rear[0]) = ftirePreset;
  (*front_and_rear[1]) = rtirePreset;
}

//== TODO: move the code below from this resource related source file to separate one

void set_global_car_wheel_friction_multiplier(float friction_mul) { physcarprivate::globalFrictionMul = friction_mul; }

void set_global_car_wheel_friction_v_decay(float value) { physcarprivate::globalFrictionVDecay = value; }

void load_car_params_block_from(const char *file_name)
{
  if (!physcarprivate::globalCarParamsBlk)
    physcarprivate::globalCarParamsBlk = new DataBlock;
  physcarprivate::globalCarParamsBlk->load(file_name);
}

void add_custom_car_params_block(const char *car_name, const DataBlock *blk)
{
  if (!physcarprivate::globalCarParamsBlk)
    physcarprivate::globalCarParamsBlk = new DataBlock(global_car_params_blk_fn);

  physcarprivate::globalCarParamsBlk->addBlock(car_name)->setFrom(blk);
}

namespace physcarprivate
{
const DataBlock &load_car_params_data_block(const char *car_name, const DataBlock *&car_blk)
{
  if (!globalCarParamsBlk)
    globalCarParamsBlk = new DataBlock(global_car_params_blk_fn);

  String blkName(car_name);
  dd_strlwr(blkName);
  car_blk = globalCarParamsBlk->getBlockByName(blkName);
  if (!car_blk)
  {
    car_blk = globalCarParamsBlk->getBlockByName("default");
    if (!car_blk)
      fatal("block <%s> not found, and <default> is also missing", blkName.str());

    G_ASSERT(car_blk);
    logerr_ctx("no block '%s' in file '%s', using default", blkName.str(), global_car_params_blk_fn);
  }
  return *globalCarParamsBlk;
}
} // namespace physcarprivate
