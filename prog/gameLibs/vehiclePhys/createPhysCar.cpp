#include <vehiclePhys/physCarGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <debug/dag_debug.h>
#include "physCarData.h"
#include "legacyRenderableRayCar.h"
#include <phys/dag_physResource.h>

#if defined(USE_BULLET_PHYSICS)
IPhysCar *create_bullet_raywheel_car(const char *res_name, const TMatrix &tm, void *phys_world, bool simple_phys, bool allow_deform)
#else
!error !unsupported physics
#endif
{
  PhysCarCreationData *pccd = (PhysCarCreationData *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(res_name), VehicleGameResClassId);

  if (!pccd)
    DAG_FATAL("can't find %s, type=Vehicle", res_name);
  G_ASSERT(pccd);
  G_ASSERT(pccd->bodyPhObjData);

  TMatrix ident = TMatrix::IDENT;

  DynamicPhysObjectData &bodyPhObjData = *pccd->bodyPhObjData;
  if (allow_deform)
    for (int i = 0; i < bodyPhObjData.models.size(); ++i)
      bodyPhObjData.models[i] = bodyPhObjData.models[i]->clone();

  SimplePhysObject *body = new (midmem) SimplePhysObject;
  body->init(&bodyPhObjData, (PhysWorld *)phys_world);
  if (!body)
    DAG_FATAL("Can't create phys body for car '%s'", res_name);

  TMatrix physTm;
  body->getBody()->getTm(physTm);

  //  debug_ctx("car '%s' body material id = %d",
  //    res_name, body->getBody()->getMaterialId());

  TMatrix physToLogic = inverse(physTm) * pccd->carDataPtr->logicTm;

  BBox3 bbox;
  BSphere3 bsphere;
  bbox.setempty();
  bsphere.setempty();
  PhysicsResource *phRes = bodyPhObjData.physRes;
  const Tab<PhysicsResource::Body> &bodies = phRes->getBodies();

  TMatrix bodyTm, invLogicTm;
  invLogicTm = inverse(pccd->carDataPtr->logicTm);

  {
    const PhysicsResource::Body &b = bodies[0];
    body->getBody()->getTm(bodyTm);
    TMatrix btm = invLogicTm * bodyTm;

    for (int i = 0; i < b.sphColl.size(); i++)
    {
      BSphere3 sph = BSphere3(btm * b.sphColl[i].center, b.sphColl[i].radius);
      bbox += sph;
      bsphere += sph;
    }
    for (int i = 0; i < b.capColl.size(); i++)
    {
      const PhysicsResource::CapColl &cc = b.capColl[i];
      bbox += BSphere3(btm * (cc.center + cc.extent / 2), cc.radius);
      bbox += BSphere3(btm * (cc.center - cc.extent / 2), cc.radius);
      bsphere += BSphere3(btm * (cc.center - cc.extent / 2), cc.radius);
      bsphere += BSphere3(btm * (cc.center + cc.extent / 2), cc.radius);
    }
    for (int i = 0; i < b.boxColl.size(); i++)
    {
      const PhysicsResource::BoxColl &bc = b.boxColl[i];
      bbox += btm * bc.tm * BBox3(-bc.size / 2, +bc.size / 2);
      bsphere += BSphere3((btm * bc.tm).getcol(3), length(bc.size) * 0.5f);
    }
  }
  G_ASSERT(!bbox.isempty());
  G_ASSERT(!bsphere.isempty());

  LegacyRenderableRayCar *car = new LegacyRenderableRayCar(res_name, body, physToLogic, bbox, bsphere, simple_phys, allow_deform,
    pccd->carDataPtr->frontSusp, pccd->carDataPtr->rearSusp, pccd->frontWheelModel, pccd->rearWheelModel);

  car->postCreateSetup();
  car->setTm(tm);

  ::release_game_resource((GameResource *)pccd);

  return car;
}

#if defined(USE_BULLET_PHYSICS)
IPhysCar *create_bullet_raywheel_car(const char *car_name, PhysBody *car_body, const BBox3 &bbox, const BSphere3 &bsphere,
  const TMatrix &tm, bool simple_phys)
#else
!error !unsupported physics
#endif
{
  G_ASSERT_RETURN(!bbox.isempty(), nullptr);
  G_ASSERT_RETURN(!bsphere.isempty(), nullptr);

  String descResName(0, "%s_vehicleDesc", car_name);
  PhysCarSettings2 *carData =
    (PhysCarSettings2 *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(descResName), VehicleDescGameResClassId);
  G_ASSERTF_RETURN(carData, nullptr, "can't find %s, type=VehicleDesc", descResName);

  TMatrix physTm, invLogicTm;
  car_body->getTm(physTm);
  TMatrix physToLogic = inverse(physTm) * carData->logicTm;
  invLogicTm = inverse(carData->logicTm);
  TMatrix btm = invLogicTm * physTm;
  // debug_ctx("car '%s' body material id = %d", car_name, car_body->getMaterialId());

  RayCar *car =
    new RayCar(car_name, car_body, physToLogic, btm * bbox, btm * bsphere, simple_phys, carData->frontSusp, carData->rearSusp);

  car->postCreateSetup();
  car->setTm(tm);

  ::release_game_resource((GameResource *)carData);
  return car;
}
