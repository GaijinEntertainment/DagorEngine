// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vehiclePhys/physCar.h>
#include <vehiclePhys/physCarGameRes.h>

#include <generic/dag_tab.h>
#include <phys/dag_vehicle.h>
#include <phys/dag_physics.h>
#include <phys/dag_simplePhysObject.h>

#include <scene/dag_visibility.h>
#include <math/dag_geomTreeMap.h>

#include "rayCarWheel.h"
#include <vehiclePhys/physCarParams.h>
#include <util/dag_simpleString.h>

class CarDynamicsModel;
class DataBlock;
class RayCarDamping;

//======================================================================================
//  ray car
//======================================================================================
class RayCar : public IPhysCar,
               public ICarMachinery,
               public IPhysBodyCollisionCallback,
               public IPhysVehicle::ICalcWheelsAngAcc,
               public IPhysVehicle::ICalcWheelContactForces,
               public IPhysVehicle::IWheelContactCB
{
public:
  RayCar(const char *car_name, PhysBody *body, const TMatrix &logic_tm, const BBox3 &bbox, const BSphere3 &bsphere, bool simple_phys,
    const PhysCarSuspensionParams &frontSusp, const PhysCarSuspensionParams &rearSusp);
  virtual ~RayCar();

  virtual void destroy() { delete this; }

  //--------------------------------------------------------
  // creation
  void setupWheel(int wid);
  void postCreateSetup();
  virtual void detachWheel(int wid, DynamicRenderableSceneLodsResource **model, TMatrix &pos);

  virtual void setTm(const TMatrix &tm);
  virtual void setCarPhysMode(CarPhysMode mode);
  virtual CarPhysMode getCarPhysMode() { return carPhysMode; }

  virtual void setDirectSpeed(real speed);
  virtual void disableCollisionDamping(real for_time);
  virtual void disableCollisionDampingFull(bool set);

  virtual real getMaxWheelAngle() { return params.maxWheelAng; }
  virtual real getCurWheelAngle() { return curWheelAng; }
  virtual Point3 getWheelLocalPos(int wid);
  virtual Point3 getWheelLocalBottomPos(int wid);

  virtual void setExternalDraw(bool set) { externalDraw = set; }
  virtual void setExternalDrawTm(const TMatrix &tm) { externalTm = tm; }
  virtual void getVisualTm(TMatrix &tm);

  virtual void setMass(real m);
  virtual void applyPhysParams(const PhysCarParams &p);
  virtual void applyGearBoxParams(const PhysCarGearBoxParams &p);
  virtual void applyDifferentialParams(int idx, const PhysCarDifferentialParams &p);
  virtual void applyCenterDiffParams(const PhysCarCenterDiffParams &p);

  virtual void setCarCollisionCB(ICarCollisionCB *cb);
  virtual void setCollisionFxCB(ICollideFxCB *cb);

  virtual void postReset();
  void postResetImmediate(bool full = true);

  virtual bool isBraking() { return resultingBrakeFactor > 0.1f; }

  virtual void reloadCarParams();

  IPhysCarLegacyRender *getLegacyRender() override { return nullptr; }

  //--------------------------------------------------------
  virtual void updateBeforeSimulate(float dt);
  virtual void updateAfterSimulate(float dt, float at_time);

  //--------------------------------------------------------
  // get parameters
  virtual Point3 getPos();
  virtual Point3 getVel();
  virtual void setVel(const Point3 &vel);
  virtual Point3 getAngVel();
  virtual void setAngVel(const Point3 &wvel);
  virtual void getTm(TMatrix &tm);
  virtual void getTm44(TMatrix4 &tm);
  virtual void getITm(TMatrix &itm);
  virtual real getMass() { return phBody->getMass(); }
  virtual Point3 getDir();
  virtual real getDirectSpeed() { return cachedDirectSpeed; }
  virtual const Capsule &getBoundCapsule();
  virtual BBox3 getBoundBox();
  virtual BBox3 getLocalBBox();
  virtual ICarMachinery *getMachinery() { return this; }
  virtual Point3 getMassCenter();

  virtual int wheelsCount() { return wheels.size(); }
  virtual int wheelsOnGround();
  virtual int wheelsSlide(float delta);
  virtual real wheelNormalForce(int id);
  virtual bool hasBodyContact() { return wasBodyCollided; }
  virtual real getWheelAngVel(int id) { return vehicle->getWheelAngularVelocity(id); }
  virtual real getWheelTurnVel(int id) { return wheels[id].psiP; }

  virtual void calcEngineMaxPower(float &max_hp, float &rpm_max_hp, float &max_torque, float &rpm_max_torque);
  virtual WheelDriveType getWheelDriveType();

  virtual void enableSteerWheelFBTorqueComputation(bool enable) { enableComputeSteerMz = enable; }
  virtual real getSteerWheelFeedbackTorque() const { return steerMz; }
  virtual real getSteerWheelEquilibriumAngle() const { return steerMzEqAngle; }

  // driver assists
  virtual void enableAssist(CarAssist type, bool en, float pwr = 1.0);
  virtual bool isAssistEnabled(CarAssist type);
  virtual bool isAssistUsedLastAct(CarAssist type);

  virtual bool resetTargetCtrlDir(bool reset_deviation_also, float align_thres_deg);
  virtual void changeTargetCtrlDir(float r_rot_rad, float ret_rate);
  virtual void changeTargetCtrlDirDeviation(float r_rot_rad, float ret_rate, float align_wt);
  virtual void changeTargetCtrlDirWt(float wt, float change_strength = 1.0);
  virtual void changeTargetCtrlVelWt(float wt, float change_strength = 1.0);

  // TODO: move to better place, this texture used only by 'uber_car_paint' shader
  virtual void setVinylTex(TEXTUREID tex_id, bool manual_delete);
  virtual TEXTUREID getVinylTex() { return vinylTexId; }
  virtual void setPlanarVinylTex(TEXTUREID tex_id, bool manual_delete);
  virtual TEXTUREID getPlanarVinylTex() { return planarVinylTexId; }
  virtual void setPlanarVinylNorm(Color4 n) { planarNorm = n; }

  virtual ICarDriver *getDriver() { return driver; }

  virtual void setWheelParamsPreset(const char *front_tyre, const char *rear_tyre);
  virtual const char *getWheelParamsPreset(bool front);
  virtual const char *getSteerPreset() { return steerPreset; }

  virtual PhysBody *getCarPhysBody() { return getPhysBody(); }

  virtual bool traceRay(const Point3 &p, const Point3 &dir, float &mint, Point3 *normal, int &pmid);

  //--------------------------------------------------------
  // settings
  virtual void addImpulse(const Point3 &point, const Point3 &force_x_dt, IPhysCar *from = NULL);
  virtual void setDriver(ICarDriver *_driver) { driver = _driver; }

  virtual void calcWheelContactForces(int wid, float norm_force, float load_rad, const Point3 wheel_basis[3],
    const Point3 &ground_normal, float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force,
    float &out_long_force);
  virtual unsigned calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt);
  virtual void postCalcWheelsAcc(float wheel_acc[], int wheel_num, float dt);

  //----- ICarMachinery -------
  virtual void setAutomaticGearBox(bool set);
  virtual bool automaticGearBox();

  virtual void setGearStep(int step, bool override_switch_in_progress = false);
  virtual int getGearStep();

  virtual real getEngineRPM();
  virtual real getEngineIdleRPM();
  virtual real getEngineMaxRPM();
  virtual void setNitro(float target_power);
  virtual void forceClutchApplication(float value);

  //----- IPhysBodyCollisionCallback -----
  virtual void onCollision(PhysBody *other, const Point3 &pt, const Point3 &norm, int this_matId, int other_matId,
    const Point3 &norm_imp, const Point3 &friction_imp, const Point3 &this_old_vel, const Point3 &other_old_vel);

  //----- IWheelContactCB -----
  virtual void onWheelContact(int wheel_id, int pmid, const Point3 &pos, const Point3 &norm, const Point3 &move_dir,
    const Point3 &wheel_dir, float slip_speed);

  virtual void setUserData(void *ud) { userData = ud; }
  virtual void *getUserData() { return userData; }

  inline PhysBody *getPhysBody() { return phBody; }
#ifndef NO_3D_GFX
  void renderDebug() override;
#endif // NO_3D_GFX

protected:
  void loadParamsFromBlk(const DataBlock *b, const DataBlock &carsBlk);
  const DataBlock &loadCarDataBlock(const DataBlock *&car_blk);
  void checkIdle();
  void checkQuasiIdle();
  void setWheelRenderTm(int wid, const TMatrix &car_visual_tm);
  void calcWheelRenderTm(TMatrix &tm, int wid, const TMatrix &car_visual_tm);

  void loadWheelsParams(const DataBlock &blk, bool front);

  void physInactivate(bool set);
  void physDisable(bool set);
  void resetGears();

  void updateSteerMz();
  void updateGearShiftEnable();
  void putToSleep(float dt);
  void updateFakeBodyFeedbackAngle(float dt);
  void updateSteerAssist(float dt);

  float calcWheelBrakingTorque(int wid, float brake_factor);

  void calcCached(bool updateTm = true);

  void calculateBoundingCapsule();

  BSphere3 getBoundingSphere();

  void freeVinylTex();
  void freePlanarVinylTex();

protected:
  IPhysVehicle *vehicle;
  CarDynamicsModel *rcar;
  int wheelPhysMatId;
  PhysBody *phBody;

  Tab<RayCarWheel> wheels;

  TMatrix tmLogic2Phys;
  TMatrix tmPhys2Logic;
  TMatrix cachedTm, cachedITm;

  ICarDriver *driver;

  SimpleString carName;

  PhysCarParams params;

  ICarCollisionCB *carCollisionCb;
  ICollideFxCB *collisionFxCb;

  bool idle, quasiIdle;

  TMatrix virtualCarTm;
  Point3 virtualCarVel;
  real carMass;
  bool externalDraw;
  TMatrix externalTm;

  CarPhysMode carPhysMode, targetCarPhysMode;

  BBox3 bbox, cachedBbox;
  BSphere3 bsphere, cachedBsphere;
  Capsule capsule, cachedCapsule;

  enum CachedFlags
  {
    CF_ITM_VALID = 1 << 0,
    CF_CAPSULE_VALID = 1 << 1,
    CF_BBOX_VALID = 1 << 2,
    CF_BSPHERE_VALID = 1 << 3
  };
  int cachedFlags;

  bool isSimpleCar;
  bool wasBodyCollided, collidedDuringUpdate;
  bool lastSteerEnabled;

  int fbTorqueGraphId, fxGraphId, wheelAngVelGraphId, wheelGndVelGraphId, slipRatioGid;

  real resultingBrakeFactor, curWheelAng;

  RayCarDamping *damping;

  bool enableComputeSteerMz;
  real steerMz, steerMzEqAngle;

  String frontWheelPreset, rearWheelPreset;
  String steerPreset;

  void *userData;

  friend void IPhysCar::applyCarPhysModeChangesBullet();

  // cache for get() calls
  real cachedDirectSpeed;

  unsigned enabledAssistsWord, usedAssistsWord, usedAssistsDuringUpdateWord;

  TEXTUREID vinylTexId, planarVinylTexId;
  bool vinylTexManualDelete, planarVinylTexManualDelete;
  Color4 planarNorm;

  float fakeBodyEngineFeedbackAngle;
  TMatrix *fakeShakeTm;

  float tcdVelAng, tcdCarDevAng;
  float rdVelAng, rdCarAng;
  float tcdDirWt, tcdVelWt;
};
