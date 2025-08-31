//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_math3d.h>
#include <math/dag_capsule.h>
#include <phys/dag_physDecl.h>
#include <3d/dag_texMgr.h>
#include <scene/dag_visibility.h>

class DynamicRenderableSceneInstance;
class DynamicRenderableSceneLodsResource;
struct Color4;

class ICarDriver;
class ICarController;
class ICarMachinery;
class DataBlock;
class IPhysCar;
class IPhysCarLegacyRender;
class GeomNodeTree;
class Point4;
class PhysCarParams;
class PhysCarGearBoxParams;
class PhysCarDifferentialParams;
class PhysCarCenterDiffParams;
class String;

//=======================================================================
// phys car lighting callback
//=======================================================================
class ILightingCB
{
public:
  virtual void setPos(const Point3 &pos, bool jump = false) = 0;
  virtual void destroy() = 0;
};


//=======================================================================
//
//=======================================================================
class ICarCollisionCB
{
public:
  virtual void onCollideWithCar(IPhysCar *self, IPhysCar *with, const Point3 &point, const Point3 &normal, const Point3 &impulse) = 0;
  virtual void onCollideWithPhObj(IPhysCar *self, PhysBody *with, const Point3 &point, const Point3 &normal,
    const Point3 &impulse) = 0;
};

//=======================================================================
//
//=======================================================================
class ICollideFxCB
{
public:
  virtual void spawnCollisionFx(IPhysCar *own_car, int wheel_id, const Point3 &pos, const Point3 &norm, const Point3 &tang_vel,
    float slip_vel, float norm_vel, int this_pmid, int other_pmid) = 0;
};

//=======================================================================
// abstract car
//=======================================================================
class IPhysCar
{
public:
  enum CarPhysMode
  {
    CPM_ACTIVE_PHYS,
    CPM_INACTIVE_PHYS,
    CPM_GHOST,
    CPM_DISABLED
  };
  enum CarAssist
  {
    CA_ABS,
    CA_ESP,
    CA_TRACTION,
    CA_STEER,
  };

  virtual void destroy() = 0;
  virtual void setDriver(ICarDriver *driver) = 0;
  virtual void addImpulse(const Point3 &point, const Point3 &force_x_dt, IPhysCar *from = NULL) = 0;
  virtual void setTm(const TMatrix &tm) = 0;
  virtual void setDirectSpeed(real speed) = 0; // apply to body & wheels
  virtual void disableCollisionDamping(real for_time) = 0;
  virtual void disableCollisionDampingFull(bool set) = 0;

  virtual real getMaxWheelAngle() = 0;
  virtual real getCurWheelAngle() = 0;
  virtual Point3 getWheelLocalPos(int wid) = 0;
  virtual Point3 getWheelLocalBottomPos(int wid) = 0;

  virtual void setMass(real m) = 0;
  virtual void applyPhysParams(const PhysCarParams &p) = 0;
  virtual void applyGearBoxParams(const PhysCarGearBoxParams &p) = 0;
  //! idx: 0 for fwd or rwd, 0=front, 1=rear for 4wd
  virtual void applyDifferentialParams(int idx, const PhysCarDifferentialParams &p) = 0;
  virtual void applyCenterDiffParams(const PhysCarCenterDiffParams &p) = 0;

  virtual void setCarPhysMode(CarPhysMode mode) = 0;
  virtual CarPhysMode getCarPhysMode() = 0;

  virtual bool traceRay(const Point3 &p, const Point3 &dir, float &mint, Point3 *normal, int &pmid) = 0;

  virtual void setExternalDraw(bool set) = 0;
  virtual void setExternalDrawTm(const TMatrix &tm) = 0;
  virtual void getVisualTm(TMatrix &tm) = 0;

  virtual void setCarCollisionCB(ICarCollisionCB *cb) = 0;
  virtual void setCollisionFxCB(ICollideFxCB *cb) = 0;

  virtual void detachWheel(int which, DynamicRenderableSceneLodsResource **model, TMatrix &pos) = 0;
  virtual void reloadCarParams() = 0;

  virtual void postReset() = 0;

  //== TODO: always treat gas factor from controller as gas and brake factor as break not depending of current gear instead of doing
  // gearbox magic in rayCar code
  //== TODO: remove this function
  virtual bool isBraking() = 0;

  //--------------------------------------------------------
  // get parameters
  virtual Point3 getPos() = 0;
  virtual Point3 getVel() = 0;
  virtual void setVel(const Point3 &vel) = 0;
  virtual Point3 getDir() = 0;
  virtual real getDirectSpeed() = 0;
  virtual Point3 getAngVel() = 0;
  virtual void setAngVel(const Point3 &wvel) = 0;
  virtual void getTm(TMatrix &tm) = 0;
  virtual void getTm44(TMatrix4 &tm) = 0;
  virtual void getITm(TMatrix &itm) = 0;
  virtual real getMass() = 0;
  virtual const Capsule &getBoundCapsule() = 0;
  virtual BBox3 getBoundBox() = 0;
  virtual BBox3 getLocalBBox() = 0;
  virtual Point3 getMassCenter() = 0;

  virtual int wheelsCount() = 0;
  virtual int wheelsOnGround() = 0;
  virtual int wheelsSlide(float delta) = 0;
  virtual real wheelNormalForce(int id) = 0;
  virtual bool hasBodyContact() = 0;
  virtual real getWheelAngVel(int id) = 0;
  virtual real getWheelTurnVel(int id) = 0; // returns wheel angular vel around Z axis

  virtual ICarDriver *getDriver() = 0;
  virtual ICarMachinery *getMachinery() = 0;

  //! enables/disables computation of values returned by next two methods
  virtual void enableSteerWheelFBTorqueComputation(bool enable) = 0;
  //! sum of Mz on steering wheels
  virtual real getSteerWheelFeedbackTorque() const = 0;
  //! instant equilibrium angle (where Mz=0)
  virtual real getSteerWheelEquilibriumAngle() const = 0;

  virtual void setWheelParamsPreset(const char *front_tyre, const char *rear_tyre = NULL) = 0;
  virtual const char *getWheelParamsPreset(bool front = true) = 0;
  virtual const char *getSteerPreset() = 0;

  virtual PhysBody *getCarPhysBody() = 0;
  virtual void setUserData(void *ud) = 0;
  virtual void *getUserData() = 0;

  virtual void calcEngineMaxPower(float &max_hp, float &rpm_max_hp, float &max_torque, float &rpm_max_torque) = 0;

  enum WheelDriveType
  {
    WDT_NONE = 0,
    WDT_FRONT = 1,
    WDT_REAR = 2,
    WDT_FULL = 3
  };
  virtual WheelDriveType getWheelDriveType() = 0;

  virtual void setVinylTex(TEXTUREID tex_id, bool manual_delete) = 0;
  virtual TEXTUREID getVinylTex() = 0;
  virtual void setPlanarVinylTex(TEXTUREID tex_id, bool manual_delete) = 0;
  virtual TEXTUREID getPlanarVinylTex() = 0;
  virtual void setPlanarVinylNorm(Color4 n) = 0;

  // driver assists
  virtual void enableAssist(CarAssist type, bool en, float pwr = 1.0) = 0;
  virtual bool isAssistEnabled(CarAssist type) = 0;
  virtual bool isAssistUsedLastAct(CarAssist type) = 0;

  virtual bool resetTargetCtrlDir(bool reset_deviation_also, float align_thres_deg) = 0;
  virtual void changeTargetCtrlDir(float r_rot_rad, float ret_rate) = 0;
  virtual void changeTargetCtrlDirDeviation(float r_rot_rad, float ret_rate, float align_wt) = 0;
  virtual void changeTargetCtrlDirWt(float wt, float change_strength = 1.0) = 0;
  virtual void changeTargetCtrlVelWt(float wt, float change_strength = 1.0) = 0;

  virtual IPhysCarLegacyRender *getLegacyRender() = 0;

  //--------------------------------------------------------
  virtual void updateBeforeSimulate(float dt) = 0;
  virtual void updateAfterSimulate(float dt, float at_time) = 0;

  static void applyCarPhysModeChangesBullet();
  static void applyCarPhysModeChangesJolt();

  static inline void applyCarPhysModeChanges()
  {
#if defined(USE_BULLET_PHYSICS)
    applyCarPhysModeChangesBullet();
#elif defined(USE_JOLT_PHYSICS)
    applyCarPhysModeChangesJolt();
#else
    DAG_FATAL("unsupported physics!");
#endif
  }
#ifndef NO_3D_GFX
  virtual void renderDebug() = 0;
#endif
};


class IPhysCarLegacyRender
{
public:
  virtual GeomNodeTree *getCarBodyNodeTree() const = 0;

  virtual bool getIsVisible() const = 0;
  virtual void setLightingK(real k) = 0;
  virtual void setLightingCb(ILightingCB *cb) = 0;
  virtual ILightingCB *getLightingCb() const = 0;
  virtual void getLogicToRender(TMatrix &tm) const = 0;

#ifndef NO_3D_GFX
  virtual void setWheelsModel(const char *model) = 0;
  virtual DynamicRenderableSceneInstance *getModel() = 0;
  virtual DynamicRenderableSceneInstance *getWheelModel(int id) = 0;

  enum PartsRenderFlags
  {
    RP_BODY = 0x01,
    RP_WHEELS = 0x02,
    RP_FULL = 0xFF,
    RP_DEBUG = 0x100
  };

  virtual void resetLastLodNo() = 0;
  virtual bool beforeRender(const Point3 &view_pos, const VisibilityFinder &vf) = 0;
  virtual void render(int parts_flags) = 0;
  virtual void renderTrans(int parts_flags) = 0;
  virtual bool getRenderWtms(TMatrix &wtm_body, TMatrix *wtm_wheels, const VisibilityFinder &vf) = 0;
#endif
};

//=======================================================================
// abstract driver
//=======================================================================
class ICarDriver
{
public:
  virtual ICarController *getController() = 0;
};


//=======================================================================
// abstract car controller
//=======================================================================
class ICarController
{
public:
  virtual void update(IPhysCar *car, real dt) = 0;
  virtual real getSteeringAngle() = 0;
  virtual real getGasFactor() = 0;   // 0..1
  virtual real getBrakeFactor() = 0; // 0..1
  virtual float getHandbrakeState() = 0;
};

//=======================================================================
// abstract car machinery
//=======================================================================
class ICarMachinery
{
public:
  virtual void setAutomaticGearBox(bool set) = 0;
  virtual bool automaticGearBox() = 0;

  // -1 - reverse, 0 - neutral, 1+ - forward
  virtual void setGearStep(int step, bool override_switch_in_progress = false) = 0;
  virtual int getGearStep() = 0;

  virtual real getEngineRPM() = 0;
  virtual real getEngineIdleRPM() = 0;
  virtual real getEngineMaxRPM() = 0;

  virtual void setNitro(float target_power) = 0;
  virtual void forceClutchApplication(float value) = 0;
};


//! creates Bullet implementation of RayCar
IPhysCar *create_bullet_raywheel_car(const char *car_name, PhysBody *car_body, const BBox3 &bbox, const BSphere3 &bsphere,
  const TMatrix &tm, bool simple_phys);
IPhysCar *create_bullet_raywheel_car(const char *res_name, const TMatrix &tm, void *phys_world, bool simple_phys, bool allow_deform);

//! creates Jolt implementation of RayCar
IPhysCar *create_jolt_raywheel_car(const char *car_name, PhysBody *car_body, const BBox3 &bbox, const BSphere3 &bsphere,
  const TMatrix &tm, bool simple_phys);
IPhysCar *create_jolt_raywheel_car(const char *res_name, const TMatrix &tm, void *phys_world, bool simple_phys, bool allow_deform);

//! creates phys-dependant implementation of RayCar (without IPhysCarLegacyRender support)
inline IPhysCar *create_raywheel_car(const char *car_name, PhysBody *car_body, const BBox3 &bbox, const BSphere3 &bsphere,
  const TMatrix &tm, bool simple_phys)
{
#if defined(USE_BULLET_PHYSICS)
  return create_bullet_raywheel_car(car_name, car_body, bbox, bsphere, tm, simple_phys);
#elif defined(USE_JOLT_PHYSICS)
  return create_jolt_raywheel_car(car_name, car_body, bbox, bsphere, tm, simple_phys);
#else
  return nullptr;
#endif
}

//! creates phys-dependant implementation of RayCar
inline IPhysCar *create_raywheel_car(const char *res_name, const TMatrix &tm, void *phys_world, bool simple_phys, bool allow_deform)
{
#if defined(USE_BULLET_PHYSICS)
  return create_bullet_raywheel_car(res_name, tm, phys_world, simple_phys, allow_deform);
#elif defined(USE_JOLT_PHYSICS)
  return create_jolt_raywheel_car(res_name, tm, phys_world, simple_phys, allow_deform);
#else
  return nullptr;
#endif
}

void load_car_params_block_from(const char *file_name);
void add_custom_car_params_block(const char *car_name, const DataBlock *blk);

bool load_raywheel_car_info(const char *res_name, const char *car_name, class PhysCarParams *info, class PhysCarGearBoxParams *gbox,
  class PhysCarDifferentialParams *diffs_front_and_rear[2], class PhysCarCenterDiffParams *center_diff);
void get_raywheel_car_def_wheels(const char *car_name, String *front_and_rear[2]);

void set_global_car_wheel_friction_multiplier(float friction_mul);
void set_global_car_wheel_friction_v_decay(float value);
