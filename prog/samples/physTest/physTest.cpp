// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <util/dag_globDef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_dynSceneRes.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug3d.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <workCycle/dag_workCycle.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_restart.h>
#include <startup/dag_loadSettings.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_startupModules.h>
#include <gui/dag_guiStartup.h>
#include <gui/dag_baseCursor.h>
#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <startup/dag_startupTex.h>
#include <math/dag_capsule.h>
#include <math/dag_math3d.h>
#include <scene/dag_frtdumpMgr.h>
#include <heightmap/heightmapHandler.h>
#include <scene/dag_physMat.h>
#include <perfMon/dag_visClipMesh.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_fileIo.h>
#include <obsolete/dag_cfg.h>
#include <gui/dag_stdGuiRender.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <util/dag_threadPool.h>

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>

#if USE_MEQON_PHYSICS
// #include <meq_remotedebug.h>
#endif

#if USE_TRUEAXIS_PHYSICS
#include <Physics/PhysicsRender.h>

#include <Physics/CollisionObjectAABBMesh.h>
#endif

#include <debug/dag_debug.h>

#include <phys/dag_physics.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physDebug.h>

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#endif
#include "../commonFramework/de3_freeCam.h"
#include "../commonFramework/de3_splashScreen.h"
#include "../commonFramework/de3_dbgPhys.h"
#include <phys/dag_vehicle.h>

static PhysWorld *physWorld = NULL;

#define ALLOW_STATIC_SCENE_DUMP 0
#define ALLOW_FRT_DUMP          0
#define ALLOW_HMAP_DUMP         0
#define ALLOW_DYNMODEL_RENDER   0 // obsolete and requres shaders
#define ALLOW_ZERO_PLANE        1
#define ALLOW_PHYS_RES          1
#define ALLOW_PHYS_CAR          1
#define ALLOW_BUILD_DYNMODEL    0 // obsolete and not working

#define SIM_DT_MUL    1.0
// #define SIM_DT_MUL 0.1
// #define START_CAM_POS 5, 2, -4
#define START_CAM_POS 13, -17, 10


PhysBody *capsuleBody = NULL;


static bool draw_phys_debug = true;

IPhysVehicle *__car = NULL;
PhysBody *__car_body = NULL;

struct RbCarControl
{
  unsigned left : 1, right : 1, up : 1, down : 1, rear : 1;
  float steer, gas, brake;
  float maxSteer, maxGas, maxBrake;

  void update(float dt)
  {
    const float wheel_ret_v = 90;
    const float wheel_v = 30;
    const float gas_v = maxGas / 2.0;
    const float brake_v = maxBrake / 0.5;

    left = HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_NUMPAD4);
    right = HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_NUMPAD6);
    up = HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_NUMPAD8);
    down = HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_NUMPAD5);
    rear = HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_R);

    float st = float(left) - float(right);
    if (left || right)
    {
      steer += wheel_v * st * dt;
      if (steer > maxSteer)
        steer = maxSteer;
      else if (steer < -maxSteer)
        steer = -maxSteer;
    }
    else if (fabs(steer) < wheel_ret_v * dt)
      steer = 0;
    else
      steer += steer > 0 ? -wheel_ret_v * dt : wheel_ret_v * dt;

    if (up)
    {
      gas += gas_v * dt;
      if (gas > maxGas)
        gas = maxGas;
    }
    else
    {
      gas -= gas_v * 8 * dt;
      if (gas < 0)
        gas = 0;
    }

    if (down)
    {
      brake += brake_v * dt;
      if (brake > maxBrake)
        brake = maxBrake;
    }
    else
    {
      brake -= brake_v * 4 * dt;
      if (brake < 0)
        brake = 0;
    }
    TMatrix tm;
    __car_body->getTm(tm);

    __car->setWheelSteering(0, steer);
    __car->setWheelSteering(1, steer);
    __car->addWheelTorque(0, rear ? -gas : gas);
    __car->addWheelTorque(1, rear ? -gas : gas);
    //__car->addWheelTorque(2, gas );
    //__car->addWheelTorque(3, gas );
    for (int i = 0; i < 4; i++)
      __car->setWheelBrakeTorque(i, brake);
  }
} __car_ctrl = {0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 40.0, 1000.0, 1000.0};

class SampleWheelForces : public IPhysVehicle::ICalcWheelContactForces
{
  virtual void calcWheelContactForces(int wid, float norm_force, float load_rad, const Point3 wheel_basis[3],
    const Point3 &ground_norm, float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force,
    float &out_long_force)
  {
    cpt_friction = 1.0;
    {
      // side force

      float slip_angle = 0.0f;
      float x = fabs(wheel_basis[B_FWD] * ground_vel);
      if (x > 1e-6)
        slip_angle = atan2(wheel_basis[B_SIDE] * ground_vel, x);
      else
      {
        slip_angle = wheel_basis[B_SIDE] * ground_vel;
        if (slip_angle > 0.0f)
          slip_angle = PI / 2.0f;
        else if (slip_angle < 0.0f)
          slip_angle = -PI / 2.0f;
      }
      slip_angle *= RAD_TO_DEG;

      float side_force = -slip_angle * cpt_friction / 3.0f;
      if (side_force > cpt_friction)
        side_force = cpt_friction;
      if (side_force < -cpt_friction)
        side_force = -cpt_friction;
      side_force *= norm_force * .25;

      float side_velocity = fabs(wheel_basis[B_SIDE] * ground_vel);
      if (side_velocity < 1.0f)
        side_force *= side_velocity * cpt_friction;

      out_lat_force = side_force;
      // debug("%p: x=%g slip_angle=%g side_velocity=%g lat_force=%.4f  fwd=%g/side=%g",
      //       this, x, slip_angle, side_velocity, out_lat_force, wheel_basis[B_FWD]*ground_vel, wheel_basis[B_SIDE]*ground_vel);
    }

    {
      // forward force

      float slip_ratio = 0.0f;
      float numerator = wheel_ang_vel * load_rad;
      float denomenator = ground_vel * wheel_basis[B_FWD];
      Point3 wheel_velocity = ground_vel - wheel_basis[B_FWD] * numerator;

      if (fabs(denomenator) > 1e-6)
        slip_ratio = (numerator - denomenator) / fabs(denomenator);
      else
        slip_ratio = numerator;
      // debug("%p: gnd_vel=%~p3 wheel_fwd=%~p3, num=%.7f / denom=%.7f slip=%.7f  normF=%.4f",
      //       this, ground_vel, wheel_basis[B_FWD], numerator, denomenator, slip_ratio, norm_force);

      slip_ratio *= 100.0f;

      float forward_force = slip_ratio * cpt_friction / 6.0f;
      if (forward_force > cpt_friction)
        forward_force = cpt_friction;
      if (forward_force < -cpt_friction)
        forward_force = -cpt_friction;
      forward_force *= norm_force;

      float forward_velocity = fabs(wheel_basis[B_FWD] * wheel_velocity);
      if (forward_velocity < 1.0f)
        forward_force *= forward_velocity * forward_velocity * 1.0f;

      //--forcePt = body->getTransform() * spring.forcePt;

      out_long_force = forward_force;
    }
  }
} calc_cwcf;

static inline Point3 calc_box_moi(float mass, const Point3 &sz)
{
  return (mass / 12.0f) * Point3(sqr(sz.y) + sqr(sz.z), sqr(sz.x) + sqr(sz.z), sqr(sz.x) + sqr(sz.y));
}

#if ALLOW_PHYS_CAR
float _slip[4], _nforce[4];

#if USE_BULLET_PHYSICS

IPhysVehicle *constructCar(const TMatrix &car_tm, float mass, float wheel_mass, const Point3 &dim, float mc_ofs, const Point3 &spr_ofs,
  float wheel_rad, float wheel_frict, float wheel_dump)
{
  PhysBody *wheel;
  TMatrix tm;
  int wid;

  PhysCompoundCollision compColl;
  tm.identity();
  tm.setcol(3, Point3(0, mc_ofs, 0));
  compColl.addChildCollision(new PhysBoxCollision(dim.x, dim.y, dim.z, true), tm);

  PhysBodyCreationData pbcd;
  pbcd.momentOfInertia = calc_box_moi(mass, dim);
  pbcd.allowFastInaccurateCollTm = false;
  __car_body = new PhysBody(physWorld, mass, &compColl, car_tm, pbcd);
  compColl.clear();

  IPhysVehicle *car = IPhysVehicle::createRayCarBullet(__car_body);
  tm.identity();

  PhysSphereCollision coll(wheel_rad);
  wheel = new PhysBody(physWorld, 0, &coll);
  wid = car->addWheel(wheel, wheel_rad, Point3(0, 0, -1), 0);
  car->setWheelParams(wid, wheel_mass, wheel_frict);
  car->setSpringPoints(wid, Point3(dim.x / 2 - spr_ofs.x, -spr_ofs.y, dim.z / 2 - spr_ofs.z), Point3(0, -1, 0), 0.25, 0.75, 0.8, -1,
    -1);
  car->setSpringHardness(wid, 19500.0, 1000.0, 50.0);
  car->setWheelDamping(0, wheel_dump);

  wheel = new PhysBody(physWorld, 0, &coll);
  wid = car->addWheel(wheel, wheel_rad, Point3(0, 0, -1), 0);
  car->setWheelParams(wid, wheel_mass, wheel_frict);
  car->setSpringPoints(wid, Point3(dim.x / 2 - spr_ofs.x, -spr_ofs.y, -dim.z / 2 + spr_ofs.z), Point3(0, -1, 0), 0.25, 0.75, 0.8, -1,
    -1);
  car->setSpringHardness(wid, 19500.0, 1000.0, 50.0);
  car->setWheelDamping(0, wheel_dump);

  wheel = new PhysBody(physWorld, 0, &coll);
  wid = car->addWheel(wheel, wheel_rad, Point3(0, 0, -1), 0);
  car->setWheelParams(wid, wheel_mass, wheel_frict);
  car->setSpringPoints(wid, Point3(-dim.x / 2 + spr_ofs.x, -spr_ofs.y, dim.z / 2 - spr_ofs.z), Point3(0, -1, 0), 0.25, 0.75, 0.8, -1,
    -1);
  car->setSpringHardness(wid, 19500.0, 1000.0, 50.0);
  car->setWheelDamping(0, wheel_dump);

  wheel = new PhysBody(physWorld, 0, &coll);
  wid = car->addWheel(wheel, wheel_rad, Point3(0, 0, -1), 0);
  car->setWheelParams(wid, wheel_mass, wheel_frict);
  car->setSpringPoints(wid, Point3(-dim.x / 2 + spr_ofs.x, -spr_ofs.y, -dim.z / 2 + spr_ofs.z), Point3(0, -1, 0), 0.25, 0.75, 0.8, -1,
    -1);
  car->setSpringHardness(wid, 19500.0, 1000.0, 50.0);
  car->setWheelDamping(0, wheel_dump);
  car->setCwcf(0xFFFFFFFF, &calc_cwcf);

  return car;
}

#else
IPhysVehicle *constructCar(const TMatrix &, float mass, float wheel_mass, const Point3 &dim, float mc_ofs, const Point3 &spr_ofs,
  float wheel_rad, float wheel_frict, float wheel_dump)
{
  DEBUG_CTX("not implemented");
  return NULL;
}
#endif

void placeRayCar()
{
  TMatrix tm;
  // tm = rotyTM ( PI/8 );
  tm.identity();
  tm.setcol(3, Point3(15, -17, 12));

  IPhysVehicle *car = constructCar(tm, 1300, 10, Point3(5, 0.4, 2.2), 0.0, Point3(0.3, -0.15, 0.05), 0.33, 0.9, 0.01);
  if (!car)
    return;

  car->setWheelSteering(0, -5.0);
  car->setWheelSteering(1, -4.5);

  __car = car;
}
#endif

PhysBody *createBox(const Point3 &pos, const Point3 &size, unsigned int mask = 0x3F, float mass = 10.0f)
{
  TMatrix tm;
  tm.identity();
  tm.m[3][0] = pos.x;
  tm.m[3][1] = pos.y;
  tm.m[3][2] = pos.z;

  PhysBodyCreationData pbcd;
  pbcd.momentOfInertia = calc_box_moi(mass, size);
  pbcd.autoMask = false, pbcd.group = pbcd.mask = mask;
  PhysBoxCollision coll(size.x, size.y, size.z, true);
  PhysBody *body = new PhysBody(physWorld, mass, &coll, tm, pbcd);

  return body;
}

PhysBody *createCapsule(const Point3 &pos, real radius, real height, int ax = 0)
{
  TMatrix tm;
  tm.identity();
  tm.m[3][0] = pos.x;
  tm.m[3][1] = pos.y;
  tm.m[3][2] = pos.z;
  PhysCapsuleCollision coll(radius, height, ax);
  return new PhysBody(physWorld, 10, &coll, tm);
}

PhysBody *createCylinder(const Point3 &pos, real radius, real height, int ax = 0)
{
#if defined(USE_BULLET_PHYSICS)
  TMatrix tm;
  tm.identity();
  tm.m[3][0] = pos.x;
  tm.m[3][1] = pos.y;
  tm.m[3][2] = pos.z;
  PhysCylinderCollision coll(radius, height, ax);
  return new PhysBody(physWorld, 10, &coll, tm);
#else
  return NULL;
#endif
}


PhysBody *createCompound(const Point3 &pos)
{
  PhysCompoundCollision collision;

  //  collision.addChildCollision(new PhysBoxCollision(5, 2, 2, true), TMatrix::IDENT);

  TMatrix tmcol;
  tmcol.identity();
  tmcol.rotyTM(PI / 8);
  tmcol.setcol(3, Point3(.2, .2, .2));

  //  collision.addChildCollision(new PhysCapsuleCollision(2, 2), tmcol);
  //  collision.addChildCollision(new PhysBoxCollision(10, 2, 2, true), tmcol);


  collision.addChildCollision(new PhysBoxCollision(3.3, 3.3, 1.2, true), tmcol);
  //  collision.addChildCollision(new PhysBoxCollision(1, 1, 1.2, true), tmcol);
  tmcol.rotyTM(PI / 4);
  collision.addChildCollision(new PhysBoxCollision(3.2, 3.3, 1.2, true), tmcol);
  //  collision.addChildCollision(new PhysBoxCollision(1.2, 1, 1, true), tmcol);

  TMatrix tm;
  tm.identity();
  tm = rotzTM(PI / 2);
  tm.setcol(3, pos);
  return new PhysBody(physWorld, 10, &collision, tm);
}


/*
class PhysTestVisual
{
public:
  virtual ~PhysTestVisual()
  {
  }

  virtual void render(const TMatrix &wtm)=0;
};


class PhysTestFakeVisual:public PhysTestVisual
{
public:
  real radius;


  PhysTestFakeVisual(real r=1) : radius(r)
  {
  }


  virtual void render(const TMatrix &wtm)
  {
    d3d::settm(TM_WORLD, (TMatrix&)wtm);
    draw_debug_sph(Point3(0,0,0), radius, E3DCOLOR(255, 255, 255));
  }
};


class PhysTestMeshVisual:public PhysTestVisual
{
public:
  DynamicRenderableSceneInstance *scene;


  ~PhysTestMeshVisual()
  {
    del_it(scene);
  }


  PhysTestMeshVisual(Mesh &mesh, Material &mat) : scene(NULL)
  {
    ShaderMeshData md;

    int numMat=mat.numsubmat();
    if (numMat<1) numMat=1;

    int i;
    for(i=0;i<mesh.face.size();++i)
      if(mesh.face[i].mat>=numMat) mesh.face[i].mat=numMat-1;
    G_VERIFY(mesh.sort_faces_by_mat());

    mesh.optimize_tverts();

    //G_VERIFY(mesh.calc_ngr());
    //G_VERIFY(mesh.calc_vertnorms());


    PtrTab<ShaderMaterial> shmats(tmpmem);

    shmats.resize(numMat);

    for (i=0; i<shmats.size(); ++i)
    {
      Material *subMat=mat.getsubmat(i);
      if (!subMat)
        DAG_FATAL("invalid sub-material #%d of '%s'", i+1, mat.name);

      shmats[i]=new_shader_material(*subMat);
      G_ASSERT(shmats[i]);
    }


    md.build(mesh, (ShaderMaterial**)&shmats[0], shmats.size(), NULL);


    Ptr<ShaderMeshResource> shmesh=new ShaderMeshResource;
    shmesh->buildMesh(&md);

    scene=new DynamicRenderableSceneInstance(
      new DynamicRenderableSceneLodsResource(
        new DynamicRenderableSceneResource(shmesh, "body")));
  }


  virtual void render(const TMatrix &wtm)
  {
    if (!scene) return;

    scene->setNodeWtm(0, wtm);

    scene->beforeRender();
    scene->render();
    scene->renderTrans();
  }
};
*/


/*
class PhysTestBody;


static Tab<PhysTestBody*> testBodies(midmem_ptr());


class PhysTestBody
{
public:
  PhysBody *body;
  //PhysTestVisual *visual;


  PhysTestBody(float mass, const Point3 &momj, const PhysCollision *coll, const TMatrix &tm, PhysTestVisual *v) : body(NULL)//,
visual(v)
  {
    PhysBodyCreationData pbcd; pbcd.momentOfInertia = momj;
    body = new PhysBody(physWorld, mass, coll, tm, pbcd);

    int i;
    for (i=0; i<testBodies.size(); ++i)
      if (!testBodies[i]) break;

    if (i>=testBodies.size())
      i=testBodies.append(1, NULL, 100);

    testBodies[i]=this;
  }


  ~PhysTestBody()
  {
    for (int i=0; i<testBodies.size(); ++i)
      if (testBodies[i]==this)
      {
        testBodies[i]=NULL;
        break;
      }

    //del_it(visual);
  }


  void render()
  {
    if (visual)
    {
      TMatrix wtm;
      body->getTm(wtm);
      visual->render(wtm);
    }
  }
};
*/


/*
static void loadPhysTestMesh(Node &node, Mesh &mesh)
{
  TMatrix bodyTm=node.wtm;

  bodyTm.setcol(0, normalize(bodyTm.getcol(0)));
  bodyTm.setcol(1, normalize(bodyTm.getcol(1)));
  bodyTm.setcol(2, normalize(bodyTm.getcol(2)));

  if (bodyTm.det()<0)
  {
    Point3 v=bodyTm.getcol(1);
    bodyTm.setcol(1, bodyTm.getcol(2));
    bodyTm.setcol(2, v);
  }

  TMatrix meshTm=inverse(bodyTm)*node.wtm;

  DataBlock blk;
  blk.loadText(node.script, node.script?strlen(node.script):0, node.name);

  real mass=blk.getReal("mass", 1);
  Point3 momj;

  PhysCollision *coll=NULL;

  if (strnicmp(node.name, "Sphere", 6)==0)
  {
    double radSq=0;

    for (int i=0; i<mesh.vert.size(); ++i)
    {
      double d=lengthSq(meshTm*mesh.vert[i]);
      if (d>radSq) radSq=d;
    }

    real radius=sqrt(radSq);

    coll = new PhysSphereCollision(radius);

    momj=blk.getPoint3("momj", Point3(1,1,1)*(mass*0.4f*radius*radius));
  }
  else if (strnicmp(node.name, "Box", 3)==0)
  {
    BBox3 box;

    for (int i=0; i<mesh.vert.size(); ++i)
      box+=meshTm*mesh.vert[i];

    bodyTm.setcol(3, bodyTm*box.center());

    Point3 sz=box.width();

    coll = new PhysBoxCollision(sz.x, sz.y, sz.z);

    momj=blk.getPoint3("momj",
      Point3(sz.y*sz.y+sz.z*sz.z, sz.x*sz.x+sz.z*sz.z, sz.y*sz.y+sz.x*sz.x)*(mass*(1.0f/12)));
  }
  else if (strnicmp(node.name, "Capsule", 7)==0)
  {
    BBox3 box;

    for (int i=0; i<mesh.vert.size(); ++i)
      box+=meshTm*mesh.vert[i];

    bodyTm.setcol(3, bodyTm*box.center());

    Point3 sz=box.width();

    real r, h;

    int axis;

    if (sz.x>sz.y)
    {
      if (sz.x>sz.z) { h=sz.x; r=(sz.y+sz.z)/4; axis=0; }
      else           { h=sz.z; r=(sz.y+sz.x)/4; axis=2; }
    }
    else
    {
      if (sz.y>sz.z) { h=sz.y; r=(sz.x+sz.z)/4; axis=1; }
      else           { h=sz.z; r=(sz.x+sz.y)/4; axis=2; }
    }

    // rotate axis so that capsule is along X axis
    if (axis==1)
    {
      Point3 a=bodyTm.getcol(0);
      bodyTm.setcol(0, bodyTm.getcol(1));
      bodyTm.setcol(1, bodyTm.getcol(2));
      bodyTm.setcol(2, a);
    }
    else if (axis==2)
    {
      Point3 a=bodyTm.getcol(2);
      bodyTm.setcol(2, bodyTm.getcol(1));
      bodyTm.setcol(1, bodyTm.getcol(0));
      bodyTm.setcol(0, a);
    }

    coll = new PhysCapsuleCollision(r, h);

    real H=h-2*r;
    if (H<0) H=0;

    real sideJ=(32*r*r*r+45*H*r*r+20*r*H*H+5*H*H*H)/(80*r+60*H);

    momj=blk.getPoint3("momj",
      Point3(r*r*(16*r+15*H)/(40*r+30*H), sideJ, sideJ)*mass);
  }
  else
  {
    momj=blk.getPoint3("momj", Point3(1, 1, 1)*(mass*(2.0f/12)));
  }

  meshTm=inverse(bodyTm)*node.wtm;

  for (int i=0; i<mesh.vert.size(); ++i)
    mesh.vert[i]=meshTm*mesh.vert[i];

  G_VERIFY(mesh.calc_ngr());
  G_VERIFY(mesh.calc_vertnorms());

  if (meshTm.det()<0)
    for (int i=0; i<mesh.vertnorm.size(); ++i)
      mesh.vertnorm[i]=-mesh.vertnorm[i];

  PhysTestBody *b=new PhysTestBody(mass, momj, coll, bodyTm, new PhysTestMeshVisual(mesh, *node.mat));
  if (coll)
    delete coll;
  //==
}


static void loadPhysTestNode(Node &n)
{
  if (!&n) return;

  if (n.obj && n.obj->isSubOf(OCID_MESHHOLDER) && n.mat)
  {
    Mesh *mesh=((MeshHolderObj*)n.obj)->mesh;
    if (mesh) loadPhysTestMesh(n, *mesh);
  }

  for (int i=0; i<n.child.size(); ++i)
    loadPhysTestNode(*n.child[i]);
}


static void loadPhysTestScene(const char *filename)
{
  AScene sc;
  load_ascene((char*)filename, sc, LASF_NULLMATS);

  sc.root->calc_wtm();

  loadPhysTestNode(*sc.root);
}
*/


#if USE_TRUEAXIS_PHYSICS

void renderLineCallBack(const TA::Vec3 &v3PosA, const TA::Vec3 &v3PosB, TA::u32 nColour)
{
  draw_debug_line(toPoint3(v3PosA), toPoint3(v3PosB), nColour);
}

void renderArrowCallBack(const TA::Vec3 &v3Pos, const TA::Vec3 &v3Vector, TA::u32 nColour)
{
  draw_debug_line(toPoint3(v3Pos), toPoint3(v3Vector), nColour);
}

void renderPolygonCallBack(int nNumVertices, const TA::Vec3 *pv3VertexList, const TA::Vec3 *pv3NormalList)
{
  E3DCOLOR color = E3DCOLOR(0, 255, 0);
  for (int n = 0; n < nNumVertices - 1; n++)
  {
    Point3 p0 = toPoint3(pv3VertexList[n + 0]);
    Point3 p1 = toPoint3(pv3VertexList[n + 1]);
    draw_debug_line(p0, p1, color);
  }
}

#endif // USE_TRUEAXIS_PHYSICS


static void drawPhysDebug()
{
  physics_draw_debug(physWorld);

#ifdef USE_TRUEAXIS_PHYSICS
  TA::PhysicsRender::SetRenderLineCallback(renderLineCallBack);
  TA::PhysicsRender::SetRenderArrowCallback(renderArrowCallBack);
  TA::PhysicsRender::SetRenderPolygonCallback(renderPolygonCallBack);
  physWorld->getTrueAxisPhysics()->SetRenderCollisionsEnabled(true);
  physWorld->getTrueAxisPhysics()->Render();
#endif // USE_TRUEAXIS_PHYSICS

  ::begin_draw_cached_debug_lines();
  Point3 offset = Point3(0, 5, 0);

  draw_cached_debug_line(offset, offset + Point3(10, 0, 0), E3DCOLOR(255, 0, 0));
  draw_cached_debug_line(offset, offset + Point3(0, 10, 0), E3DCOLOR(0, 255, 0));
  draw_cached_debug_line(offset, offset + Point3(0, 0, 10), E3DCOLOR(0, 0, 255));

  ::end_draw_cached_debug_lines();
}

static Ptr<PhysicsResource> physResource;

static PhysSystemInstance *physSystem = NULL;

#if ALLOW_DYNMODEL_RENDER
static Tab<TMatrix *> tmHelpers(midmem_ptr());
static Ptr<DynamicRenderableSceneLodsResource> dynamicSceneResource;
static DynamicRenderableSceneInstance *dynamicScene = NULL;
#endif

#if ALLOW_FRT_DUMP
static FastRtDumpManager *frt_mgr = NULL;
#endif
#if ALLOW_HMAP_DUMP
static HeightmapHandler hmap;
#endif

#if ALLOW_BUILD_DYNMODEL
#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <libTools/shaderResBuilder/dynSceneWithTreeResSrc.h>
#include <shaders/dag_dynSceneWithTreeRes.h>
#endif

static void loadPhysTestScene(const char *filename)
{
  DataBlock mainBlk(filename);
  String resFilename(filename);

  //!!!!!!  const char *ext=get_extension(resFilename);
  const char *ext = dd_get_fname_ext(resFilename);

  if (ext)
  {
    int at = ext - (char *)resFilename;
    erase_items(resFilename, at, resFilename.length() - at);
  }

  resFilename.append(".dat");

#if ALLOW_BUILD_DYNMODEL
  DynamicRenderableSceneLodsResSrc::registerFactory();
  DynamicSceneWithTreeResSrc::registerFactory(DynamicRenderableSceneLodsResourceClassName, DynamicSceneWithTreeResourceCID);
  ResourceSourceFactory *factory = get_resource_source_factory("dynobj");
  if (!factory)
    DAG_FATAL("can't get resource source factory");

  ResourceSource *src = factory->create(filename);
  if (!src)
    DAG_FATAL("can't create resource source");
  src->build();
  src->saveToFile(resFilename, _MAKE4C('PC'));
  del_it(src);

  factory->endBuild();
#endif

#if ALLOW_DYNMODEL_RENDER
  DynamicRenderableSceneLodsResource *lods;
  {
    FullFileLoadCB crd(resFilename);
    G_VERIFY(crd.readInt() == DynamicRenderableSceneLodsResourceCID.id);
    lods = DynamicRenderableSceneLodsResource::loadResource(crd, 0);
  }
  G_ASSERT(lods);

  dynamicSceneResource = lods;

  dynamicScene = new DynamicRenderableSceneInstance(dynamicSceneResource);
#endif

#if ALLOW_ZERO_PLANE
  PhysBody *b = createBox(Point3(0, -21, 0), Point3(1000, 4, 1000), 3, 0.0f);
#endif
#if ALLOW_FRT_DUMP
  const char *frt_file = mainBlk.getStr("frt_dump", NULL);
  if (frt_file)
  {
    FullFileLoadCB crd(frt_file);
    if (!crd.fileHandle)
      DAG_FATAL("cannot open: <%s>", frt_file);

    frt_mgr = new FastRtDumpManager;
    if (frt_mgr->loadRtDump(crd, 0) < 0)
      DAG_FATAL("cannot load frtdump from: <%s>", frt_file);

    ::create_visclipmesh(CfgReader(""));
    ::set_vcm_rad(mainBlk.getReal("visClipMeshVisibilityRad", 200));
    ::set_vcm_visible(true);
    ::grs_draw_wire = true;
  }
#endif

#if ALLOW_STATIC_SCENE_DUMP
  const char *phys_file = mainBlk.getStr("phys_scene_dump", NULL);
  if (phys_file)
  {
    FullFileLoadCB crd(phys_file);
    if (!crd.fileHandle)
      DAG_FATAL("cannot open: <%s>", phys_file);

    crd.beginFullFileBlock();
#if defined(USE_BULLET_PHYSICS)
    if (!physWorld->loadSceneCollision(crd, -1))
      DAG_FATAL("cannot load static scene collision");
#else
    DAG_FATAL("ALLOW_STATIC_SCENE_DUMP not supported");
#endif
  }
#endif

#if ALLOW_HMAP_DUMP
  const char *hmap_file = mainBlk.getStr("hmap_dump", NULL);
  if (hmap_file)
  {
    FullFileLoadCB crd(hmap_file);
    if (!crd.fileHandle)
      logerr("file '%s' not found", hmap_file);
    else if (crd.beginTaggedBlock() == _MAKE4C('HM2'))
    {
      hmap.loadDump(crd, false, nullptr);

      PhysHeightfieldCollision coll(&hmap, Point2::xz(hmap.getHeightmapOffset()), hmap.getWorldBox(), 1.0f, 2.f, nullptr);
      PhysBodyCreationData pbcd;
      pbcd.useMotionState = false;
      pbcd.friction = 1.f;
      pbcd.restitution = 0.5;
      TMatrix tm;
      tm.identity();
      tm.setcol(3, mainBlk.getPoint3("hmap_pos", Point3(0, 0, 0)));
      new PhysBody(physWorld, 0.f, &coll, tm, pbcd);
      crd.endBlock();
    }
    else
      logerr("wrong '%s' file format, expecting HM2 binary field", hmap_file);
  }
#endif

#if ALLOW_PHYS_RES
  {
    FullFileLoadCB crd(mainBlk.getStr("physicsFile", "no-physics-file-specified"));
    G_VERIFY(crd.readInt() == PhysicsResourceCID.id);
    physResource = PhysicsResource::loadResource(crd, 0);
  }

  physSystem = new PhysSystemInstance(physResource, physWorld, &TMatrix::IDENT, nullptr);

#if ALLOW_DYNMODEL_RENDER
  const RoNameMapEx &nameMap = lods->getNames().node;

  tmHelpers.resize(nameMap.map.size());
  iterate_names(nameMap, [&](int id, const char *name) { tmHelpers[id] = physSystem->getTmHelper(name); });
#endif
#endif
}


static void initPhysTestScene()
{
  // loadPhysTestScene("develop/PhysTest/scenes/test1.dag");
  loadPhysTestScene("scenes/test_new1.blk");
}


static void initPhysics()
{
  DEBUG_CTX("init physics");
  init_physics_engine();

#if USE_MEQON_PHYSICS
  // MEQ_DBG_OPEN_FILE_STREAM("PhysTestMeqon.mqd", ~0);
#endif

  //  BBox3 worldSize = BBox3(Point3(-50, -50, -50), Point3(50, 50, 50));

  physWorld = new PhysWorld(0.9f, 0.7f, 0.5f, 1.0f); //, worldSize);
  physdbg::init<PhysWorld>();

  DEBUG_CTX("init test scene");
  initPhysTestScene();
  DEBUG_CTX("inited");

  physics_init_draw_debug(physWorld);
  debug_flush(false);
  /*
  {
    tm[3][0] = 10;
    tm[3][1] = 0;
    tm[3][2] = 0;

    PhysCapsuleCollision coll(0.5, 3);
    PhysBody* body = new PhysBody(physWorld, 0, &coll, tm);
  }
  */
}


static void closePhysics()
{
  /*
  for (int i=0; i<testBodies.size(); ++i)
    if (testBodies[i]) delete(testBodies[i]);
  testBodies.clear();
  */

#if ALLOW_DYNMODEL_RENDER
  del_it(dynamicScene);
  dynamicSceneResource = NULL;
  tmHelpers.clear();
#endif

  del_it(physSystem);
  physResource = NULL;

  del_it(physWorld);

#if USE_MEQON_PHYSICS
  // MEQ_DBG_CLOSE_STREAM();
#endif

  close_physics_engine();
  physdbg::term<PhysWorld>();
}


static real g_time = 0;
static real startTime = 0;

class PhysTestDagorGameScene : public DagorGameScene
{
public:
  PhysTestDagorGameScene()
  {
    freeCam = NULL;
    simCnt = 0;
    simTime = 0;
    avgSimCnt = 0;
    avgSimTime = 0;
  }

  virtual void actScene()
  {
    freeCam->act();
#if _TARGET_PC_MACOSX
    if (HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_LWIN) && HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_Q))
    {
      debug("request to close (Cmd+Q)");
      quit_game(0);
      return;
    }
#endif

    if (physWorld)
    {
      /*
      if (capsuleBody)
      {
        Point3 velocity = capsuleBody->getVelocity();

        if(fabs(velocity.z < 1))
          velocity.z -= .001f;


        TMatrix tm;
        capsuleBody->getTm(tm);
        Point3 pos = tm.getcol(3);
        tm.identity();
        tm = rotzTM(PI/2);
        tm.setcol(3, pos);
        capsuleBody->setTm(tm);

        capsuleBody->setVelocity(velocity);
      }
      */

      //      physWorld->simulate(.001f);
    }

    if (physWorld)
    {
      if (!g_time)
      {
        g_time = (float)get_time_msec() / 1000.0f;
        startTime = g_time;
      }

      float newTime = (float)get_time_msec() / 1000.0f;

      float delta = newTime - g_time;

      //      delta /= 10;

      g_time = newTime;

      if (g_time - startTime > 1)
        if (delta > 0)
        {
          __int64 reft = ::ref_time_ticks();
          while (delta > 0.0)
          {
#if ALLOW_PHYS_CAR
            if (__car)
            {
              __car->update(0.01 * SIM_DT_MUL);
              __car_ctrl.update(0.01 * SIM_DT_MUL);
            }
#endif
            physWorld->simulate(0.01 * SIM_DT_MUL);
            delta -= 0.01;
            simCnt++;
          }
          simTime += ::get_time_usec(reft);
        }
    }
  }


  //! called just before drawScene()
  virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
  {
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();
    freeCam->setView();
  }

  //! called to render scene
  virtual void drawScene()
  {
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0x0, 1, 0);

    /*
        TMatrix mat;
        mat = matrix_look_at_lh(Point3(20,40,0), Point3(0,10,0), Point3(0,1,0));
    //    mat = matrix_look_at_lh(Point3(40,-10,-20), Point3(0,-10,0), Point3(0,1,0));
        d3d::settm(TM_VIEW, mat);
        d3d::setpersp(1,1,.1,1000);
    */

    freeCam->setView();
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    if (draw_phys_debug)
      drawPhysDebug();
    if (__car)
      __car->debugRender();

      //!!!test ray cast

      //    uint32_t time = timeGetTime();

      /*    for(int n=0;n<100;n++)
          {
            Point3 from = Point3(n, 20, n);
            Point3 to = Point3(n, -40, n);

            Point3 diff = to - from;

            PhysRayCast rayCast(from, normalize(diff), diff.length(), physWorld);
            rayCast.setInteractionLayer(1);

            rayCast.forceUpdate();

            draw_debug_line(from, to);

            if (rayCast.hasContact())
            {
              Point3 point = rayCast.getPoint();

              draw_debug_line(from, point, E3DCOLOR(255,255,255,255));
            }

          }*/

      //    uint32_t newtime = timeGetTime();
      //    DAG_FATAL("time %d", (int)newtime - time);

#if ALLOW_DYNMODEL_RENDER
    if (dynamicScene)
    {
      physSystem->updateTms();

      for (int i = 0; i < tmHelpers.size(); ++i)
        if (tmHelpers[i])
          dynamicScene->setNodeWtm(i, *tmHelpers[i]);

      dynamicScene->beforeRender();
      dynamicScene->render();
      dynamicScene->renderTrans();
    }

    // for (int i=0; i<testBodies.size(); ++i)
    //   if (testBodies[i]) testBodies[i]->render();
#endif

#if ALLOW_HMAP_DUMP
    Point3 norm;
    Point3 hm(::grs_cur_view.pos.x + 5, 0, ::grs_cur_view.pos.z + 2);
    // Point3 hm ( 25, 0, 12 );

    if (false)
    {
      ::begin_draw_cached_debug_lines();
      ::set_cached_debug_lines_wtm(TMatrix::IDENT);

      ::draw_cached_debug_line(hm - Point3(0.5, 0, 0), hm + Point3(0.5, 0, 0), E3DCOLOR(0, 0, 255));
      ::draw_cached_debug_line(hm - Point3(0, 0, 0.5), hm + Point3(0, 0, 0.5), E3DCOLOR(0, 0, 255));
      ::draw_cached_debug_line(hm, hm + norm * 2, E3DCOLOR(0, 255, 255));

      ::end_draw_cached_debug_lines();
    }
#endif

#if ALLOW_FRT_DUMP
    render_visclipmesh(*frt_mgr, ::grs_cur_view.pos);
#endif
    if (!draw_phys_debug)
      physdbg::renderWorld(physWorld,
        physdbg::RenderFlag::BODIES | physdbg::RenderFlag::CONSTRAINTS | physdbg::RenderFlag::CONSTRAINT_LIMITS |
          physdbg::RenderFlag::CONSTRAINT_REFSYS |
          /*physdbg::RenderFlag::BODY_BBOX|physdbg::RenderFlag::BODY_CENTER|*/ physdbg::RenderFlag::CONTACT_POINTS,
        &::grs_cur_view.pos, 100);

    avgSimCnt += simCnt;
    avgSimTime += simTime;

    /**/
    bool old_wire = ::grs_draw_wire;
    ::grs_draw_wire = false;
    bool do_start = !StdGuiRender::is_render_started();
    if (do_start)
      StdGuiRender::start_render();

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(255, 0, 255, 255);
    StdGuiRender::set_ablend(true);

    StdGuiRender::goto_xy(10, 10);
    StdGuiRender::draw_str(String(128, "sim=%5.2f usec", simCnt ? double(simTime) / simCnt : 0.0));
    StdGuiRender::goto_xy(10, 30);
    StdGuiRender::draw_str(String(128, "avg=%5.2f usec", avgSimCnt ? double(avgSimTime) / avgSimCnt : 0.0));
    StdGuiRender::goto_xy(10, 600);
#if ALLOW_PHYS_CAR
    if (__car && __car_body)
    {
      TMatrix tm;
      __car_body->getTm(tm);
      real spd_kmh = (tm.getcol(0) * __car_body->getVelocity()) * 3.6;
      StdGuiRender::draw_str(String(128, "Vel=%5.1f (%7.1f:%7.1f:%7.1f:%7.1f:) side=%.3f km/h", spd_kmh,
        __car->getWheelAngularVelocity(0) * 0.35 * 3.6 - spd_kmh, __car->getWheelAngularVelocity(1) * 0.35 * 3.6 - spd_kmh,
        __car->getWheelAngularVelocity(2) * 0.35 * 3.6 - spd_kmh, __car->getWheelAngularVelocity(3) * 0.35 * 3.6 - spd_kmh,
        (tm.getcol(2) * __car_body->getVelocity()) * 3.6));
    }
#endif

    StdGuiRender::flush_data();
    if (do_start)
      StdGuiRender::end_render();
    ::grs_draw_wire = old_wire;
    /**/

    if (avgSimCnt > 300)
    {
      debug("avgSimTime=%.2f usec/sim (%.2f%% cpu)", double(avgSimTime) / avgSimCnt, double(avgSimTime) / avgSimCnt / 10000 * 100);
      avgSimCnt = 0;
      avgSimTime = 0;
      debug_flush(false);
    }
    simTime = 0;
    simCnt = 0;
  }
  virtual void sceneSelected(DagorGameScene * /*prev_scene*/)
  {
    freeCam = create_mskbd_free_camera();
    freeCam->scaleSensitivity(0.1, 0.5);
    TMatrix tm;
    tm.identity();
    tm.setcol(3, START_CAM_POS);
    freeCam->setInvViewMatrix(tm);
    debug_flush(false);
  }

private:
  IFreeCameraDriver *freeCam;
  int simCnt, simTime;
  int avgSimCnt, avgSimTime;
};


#include <math/dag_mesh.h>


void appendQuad(int x1, int x2, int x3, int x4, Tab<Face> &face, int &face_id, bool is_extern)
{
  if (!is_extern)
  {
    face[face_id++].set(x3, x4, x1);
    face[face_id++].set(x1, x2, x3);
  }
  else
  {
    face[face_id++].set(x1, x4, x3);
    face[face_id++].set(x3, x2, x1);
  }
}


void appendBoxRingVertex(real z_offset, const IPoint2 &count, Tab<Point3> &vert)
{
  const real offset = 0.5f;
  const real yStep = 1.0f / (real)(count.y - 1);
  const real xStep = 1.0f / (real)(count.x - 1);

  for (int x = 0; x < count.x; ++x)
    vert.push_back(Point3((real)x * xStep - offset, 0.0f, z_offset));
  for (int y = 1; y < count.y; ++y)
    vert.push_back(Point3(offset, (real)y * yStep, z_offset));
  for (int x = count.x - 2; x >= 0; --x)
    vert.push_back(Point3((real)x * xStep - offset, 1.0f, z_offset));
  for (int y = count.y - 2; y >= 1; --y)
    vert.push_back(Point3(-offset, (real)y * yStep, z_offset));
}


void appendBoxRingFaces(int ring1_vert_offset, int ring2_vert_offset, int count, Tab<Face> &face, int &face_id)
{
  int x1, x2, x3, x4;
  x1 = ring1_vert_offset + count - 1;
  x4 = ring2_vert_offset + count - 1;
  for (int i = 0; i < count; ++i)
  {
    x2 = ring1_vert_offset + i;
    x3 = ring2_vert_offset + i;
    appendQuad(x1, x2, x3, x4, face, face_id, false);
    x1 = x2;
    x4 = x3;
  }
}


void appendBoxPlaneFaces(const Point3 &f, int ring_vert_offset, int vert_in_ring_count, int plane_vert_offset, int vert_in_plane_count,
  Tab<Face> &face, int &face_id, bool is_extern)
{
  int x1, x2, x3, x4;

  if (f.y == 1)
  {
    x1 = ring_vert_offset;
    x4 = x1 + vert_in_ring_count - 1;
    for (int i = 0; i < f.x; ++i)
    {
      x2 = x1 + 1;
      x3 = x4 - 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
  }
  else
  {
    // first row
    x1 = ring_vert_offset;
    x4 = x1 + vert_in_ring_count - 1;
    for (int x = 0; x < (f.x - 1); ++x)
    {
      x2 = x1 + 1;
      x3 = plane_vert_offset + x;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
    x2 = ring_vert_offset + f.x;
    x3 = x2 + 1;
    appendQuad(x1, x2, x3, x4, face, face_id, is_extern);

    // other rows except first and last
    for (int y = 0; y < (f.y - 2); ++y)
    {
      x1 = ring_vert_offset + vert_in_ring_count - 1 - y;
      x4 = x1 - 1;
      for (int x = 0; x < (f.x - 1); ++x)
      {
        x2 = plane_vert_offset + x + (y * (f.x - 1));
        x3 = x2 + (f.x - 1);
        appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
        x1 = x2;
        x4 = x3;
      }
      x2 = ring_vert_offset + f.x + 1 + y;
      x3 = x2 + 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
    }

    // last row
    x1 = ring_vert_offset + vert_in_ring_count - 1 - (f.y - 2);
    x4 = x1 - 1;
    for (int x = 0; x < (f.x - 1); ++x)
    {
      x2 = plane_vert_offset + x + (vert_in_plane_count - (f.x - 1));
      x3 = x4 - 1;
      appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
      x1 = x2;
      x4 = x3;
    }
    x2 = x4 - 2;
    x3 = x2 + 1;
    appendQuad(x1, x2, x3, x4, face, face_id, is_extern);
  }
}


Mesh *generateBoxMesh(const IPoint3 &segments_count, Color4 col = Color4(1, 1, 1, 1))
{
  if ((segments_count.x <= 0) || (segments_count.y <= 0) || (segments_count.z <= 0))
    return NULL;

  Mesh *mesh = new (midmem) Mesh;
  if (!mesh)
    return NULL;

  const IPoint3 &f = segments_count;
  const int vertInRingCount = ((f.x + 1) * 2) + ((f.y - 1) * 2);
  const int vertInPlaneCount = (f.x - 1) * (f.y - 1);
  mesh->vert.reserve((vertInRingCount * (f.z + 1)) + (vertInPlaneCount * 2));
  mesh->face.resize((f.x * f.y * 4) + (f.y * f.z * 4) + (f.z * f.x * 4));

  // generate vertex
  const real offset = 0.5f;
  const real zStep = 1.0f / (real)f.z;
  for (int z = 0; z < (f.z + 1); ++z)
    appendBoxRingVertex((real)z * zStep - offset, IPoint2(f.x + 1, f.y + 1), mesh->vert);

  const real yStep = 1.0f / (real)f.y;
  const real xStep = 1.0f / (real)f.x;
  // near plane
  for (int y = 1; y < f.y; ++y)
    for (int x = 1; x < f.x; ++x)
      mesh->vert.push_back(Point3((real)x * xStep - offset, (real)y * yStep, -offset));
  // far plane
  for (int y = 1; y < f.y; ++y)
    for (int x = 1; x < f.x; ++x)
      mesh->vert.push_back(Point3((real)x * xStep - offset, (real)y * yStep, offset));

  mesh->tvert[0].resize(mesh->vert.size());
  for (int i = 0; i < mesh->vert.size(); ++i)
  {
    mesh->tvert[0][i].x = mesh->vert[i].x;
    mesh->tvert[0][i].y = mesh->vert[i].y;
  }


  // generate faces
  int faceId = 0;
  for (int z = 0; z < f.z; ++z)
    appendBoxRingFaces(vertInRingCount * z, vertInRingCount * (z + 1), vertInRingCount, mesh->face, faceId);

  // near plane
  const int nearRingVertOffset = 0;
  const int nearPlaneVertOffset = vertInRingCount * (f.z + 1);
  appendBoxPlaneFaces(f, nearRingVertOffset, vertInRingCount, nearPlaneVertOffset, vertInPlaneCount, mesh->face, faceId, true);
  // far plane
  const int farRingVertOffset = vertInRingCount * f.z;
  const int farPlaneVertOffset = (vertInRingCount * (f.z + 1)) + vertInPlaneCount;
  appendBoxPlaneFaces(f, farRingVertOffset, vertInRingCount, farPlaneVertOffset, vertInPlaneCount, mesh->face, faceId, false);

  G_ASSERT(mesh->face.size() == faceId);

  mesh->tface[0].resize(mesh->face.size());
  for (int i = 0; i < mesh->face.size(); ++i)
  {
    mesh->tface[0][i].t[0] = mesh->face[i].v[0];
    mesh->tface[0][i].t[1] = mesh->face[i].v[1];
    mesh->tface[0][i].t[2] = mesh->face[i].v[2];
  }
  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = col;

  mesh->cface.resize(mesh->face.size());
  for (int fi = 0; fi < mesh->face.size(); ++fi)
  {
    mesh->cface[fi].t[0] = 0;
    mesh->cface[fi].t[1] = 0;
    mesh->cface[fi].t[2] = 0;
  }

  return mesh;
}

Mesh *generatePlaneMesh(const Point2 &cell_size)
{
  if (!cell_size.x || !cell_size.y)
    return NULL;

  Mesh *mesh = new (midmem) Mesh;
  if (!mesh)
    return NULL;

  const Point2 cellSize(1.0f / cell_size.x, 1.0f / cell_size.y);
  const IPoint2 iCellCount(cell_size.x + 0.99999f, cell_size.y + 0.99999f);

  mesh->vert.resize((iCellCount.x + 1) * (iCellCount.y + 1));
  mesh->tvert[0].resize(mesh->vert.size());
  mesh->face.resize(iCellCount.x * iCellCount.y * 2);
  mesh->tface[0].resize(mesh->face.size());

  for (int y = 0; y <= iCellCount.y; ++y)
  {
    const unsigned vertOffset = y * (iCellCount.x + 1);
    const unsigned faceOffset = (y - 1) * iCellCount.x * 2;
    const real posY = (y == iCellCount.y) ? cellSize.y * cell_size.y : cellSize.y * (real)y;

    for (int x = 0; x <= iCellCount.x; ++x)
    {
      const unsigned vertId = vertOffset + x;
      const unsigned prevVertId = vertOffset - (iCellCount.x + 1) + x;
      real posX = (x == iCellCount.x) ? cellSize.x * cell_size.x : cellSize.x * (real)x;

      mesh->vert[vertId] = Point3(posX, 0, posY);
      mesh->tvert[0][vertId] = Point2(posX, posY);

      if (!x || !y)
        continue;

      const unsigned faceId = faceOffset + (x - 1) * 2;
      mesh->face[faceId].set(vertId - 1, prevVertId, prevVertId - 1);
      mesh->face[faceId + 1].set(prevVertId, vertId - 1, vertId);

      mesh->tface[0][faceId].t[0] = mesh->face[faceId].v[0];
      mesh->tface[0][faceId].t[1] = mesh->face[faceId].v[1];
      mesh->tface[0][faceId].t[2] = mesh->face[faceId].v[2];

      mesh->tface[0][faceId + 1].t[0] = mesh->face[faceId + 1].v[0];
      mesh->tface[0][faceId + 1].t[1] = mesh->face[faceId + 1].v[1];
      mesh->tface[0][faceId + 1].t[2] = mesh->face[faceId + 1].v[2];
    }
  }

  const Point3 pos(0.5f, 0, 0.5f);
  for (int i = 0; i < mesh->vert.size(); ++i)
    mesh->vert[i] -= pos;

  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = Color4(1, 1, 1, 1);

  return mesh;
}


static class PhysTestRestartProc : public SRestartProc
{
public:
  const char *procname() { return "PhysTest"; }
  PhysTestRestartProc() : SRestartProc(RESTART_GAME | RESTART_VIDEO) {}


  void startup()
  {
    initPhysics();


    PhysBody *body = createCapsule(Point3(-5, 40, 1), 0.1, 0.21);
    body->setContinuousCollisionMode(true);
    body->setVelocity(Point3(0, -100, 0));

    createCapsule(Point3(-5, 40, 5), 0.3, 2.2, 0);
    createCapsule(Point3(-5, 40, 10), 0.3, 2.2, 1);
    createCapsule(Point3(-5, 40, 15), 0.3, 2.2, 2);
    createCylinder(Point3(-5, 30, 5), 0.3, 2.2, 0);
    createCylinder(Point3(-5, 30, 10), 0.3, 2.2, 1);
    createCylinder(Point3(-5, 30, 15), 0.3, 2.2, 2);

    createBox(Point3(-1, 20, 1), Point3(10, 1.9 + 3, 1.9));
#if ALLOW_PHYS_CAR
    placeRayCar();
#endif

    // for ( float z = -20; z < 20; z+=2 )
    //   createBox(Point3(-10,10,z), Point3(10,1.9 + 3,1.9));


    //    capsuleBody = createCompound(Point3(0,0,20));
    //    capsuleBody->setMassMatrix(0,0,0,0);


    /*
        createBox(Point3(10,20,0), Point3(10,1,10));

        createBox(Point3(10,-4,-20), Point3(1,10,10));

        createCompound(Point3(20,30,0));
        */

#ifdef USE_TRUEAXIS_PHYSICS


    TA::DynamicObject *obj1 = TA::DynamicObject::CreateNew();
    obj1->InitialiseAsABox(TA::AABB(TA::Vec3(0, 0, 0), TA::Vec3(.5, .5, .5)));

    TA::DynamicObject *obj2 = TA::DynamicObject::CreateNew();
    obj2->InitialiseAsABox(TA::AABB(TA::Vec3(0, 0, 0), TA::Vec3(.5, .5, .5)));

    obj1->SetMass(10);
    obj2->SetMass(10);

    obj1->SetExtraStability(true);
    obj2->SetExtraStability(true);


    TA::MFrame objframe1;
    TA::MFrame objframe2;

    objframe1.v3Translation = TA::Vec3(0, 20, 5);
    //  objframe1.m33Rotation.SetToIdentity();
    objframe1.m33Rotation.SetToLookAt(TA::Vec3(cos(.8), 0, sin(.8)), TA::Vec3(sin(.8), 0, cos(.8)));

    objframe2.v3Translation = TA::Vec3(0, 20, 5 + 2);
    objframe2.m33Rotation.SetToLookAt(TA::Vec3(cos(.5), 0, sin(.5)), TA::Vec3(sin(.5), 0, cos(.5)));

    obj1->SetFrame(objframe1);
    obj2->SetFrame(objframe2);

    physWorld->getTrueAxisPhysics()->AddDynamicObject(obj1);
    physWorld->getTrueAxisPhysics()->AddDynamicObject(obj2);


    TA::Vec3 jointPos = (objframe1.v3Translation + objframe2.v3Translation) / 2;
    TA::Vec3 jointAxis = TA::Vec3(0, 0, 1);
    TA::Vec3 jointUpVector = TA::Vec3(0, 1, 0);

    TA::Mat33 jointOrientationGlobal;
    jointOrientationGlobal.SetToLookAt(jointAxis, jointUpVector);
    //  jointOrientationGlobal.SetToLookAt(jointUpVector, jointAxis);

    TA::Mat33 jointOrientationLocal =
      jointOrientationGlobal * obj2->GetFrame().m33Rotation * obj1->GetFrame().m33Rotation.GetTransposeAsInverse();

    /*
      Mat33 m33RotationB = physicsJoint.GetDefaultRotationOfObjectB() * pDynamicObjectB->GetFrame().m33Rotation;
      Mat33 m33RotationA = physicsJoint.GetDefaultRotationOfObjectB() * pDynamicObjectA->GetFrame().m33Rotation;

      Mat33 m33ConstrainedRotation = m33RotationB * m33RotationA.GetTransposeAsInverse();

      EulerAngles rotation = physicsJoint.GetPreviousOrientation();
    */

    TA::EulerAngles minAngles = TA::EulerAngles(0, 0, 0);
    TA::EulerAngles maxAngles = TA::EulerAngles(0, 0, 0);

    TA::Mat33 m;
    m = minAngles.GetAsMat33() * (obj2->GetFrame().m33Rotation/* * 
    obj1->GetFrame().m33Rotation.GetTransposeAsInverse()*/);
    minAngles = TA::EulerAngles(m);

    m = maxAngles.GetAsMat33() * (obj2->GetFrame().m33Rotation/* * 
    obj1->GetFrame().m33Rotation.GetTransposeAsInverse()*/);
    maxAngles = TA::EulerAngles(m);

    obj1->AddJoint(obj2, jointPos / objframe1, jointPos / objframe2, jointOrientationLocal, minAngles, maxAngles);

    /*
      obj1->AddJointTypeSquareSocket(
        obj2,
        jointPos / objframe1,
        jointPos / objframe2,
        jointOrientationLocal,
        0,
        0,
        0,
        0,
        0,
        0);
        */

    /*
        PhysBody* body1 = createBox(Point3(0,20,5), Point3(1,1,1));
        PhysBody* body2 = createBox(Point3(0,20,5 + 1.5), Point3(1,1,1));
        PhysBody* body3 = createBox(Point3(0,20,5 + 1.5 * 2), Point3(1,1,1));
        PhysBody* body4 = createBox(Point3(0,20 + 1.5,5 + 1.5 * 2), Point3(1,1,1));

        {
          TMatrix tm;
          body2->getTm(tm);

          TMatrix t;
          t.rotyTM(-PI/4);

          tm.setcol(0, t.getcol(0));
          tm.setcol(1, t.getcol(1));
          tm.setcol(2, t.getcol(2));

          body2->setTm(tm);
        }

        {
          TMatrix tm;
          body3->getTm(tm);

          TMatrix t;
          t.rotyTM(PI/4);

          tm.setcol(0, t.getcol(0));
          tm.setcol(1, t.getcol(1));
          tm.setcol(2, t.getcol(2));

          body3->setTm(tm);
        }


        TA::DynamicObject* obj1 = body1->getTrueAxisDynamicObject();
        TA::DynamicObject* obj2 = body2->getTrueAxisDynamicObject();
        TA::DynamicObject* obj3 = body3->getTrueAxisDynamicObject();
        TA::DynamicObject* obj4 = body4->getTrueAxisDynamicObject();

        obj1->SetRestTimeMultiplier(2);
        obj1->SetExtraStability(true);
        obj1->SetFriction(1);
        obj1->SetRigidFrictionDisabled(true);
        obj2->SetRestTimeMultiplier(2);
        obj2->SetExtraStability(true);
        obj2->SetFriction(1);
        obj2->SetRigidFrictionDisabled(true);
        obj3->SetRestTimeMultiplier(2);
        obj3->SetExtraStability(true);
        obj3->SetFriction(1);
        obj3->SetRigidFrictionDisabled(true);
        obj4->SetRestTimeMultiplier(2);
        obj4->SetExtraStability(true);
        obj4->SetFriction(1);
        obj4->SetRigidFrictionDisabled(true);

        Point3 jointpos = Point3(0,20,(5.0 + 5.15) / 2);

        TMatrix tm1;
        TMatrix tm2;
        TMatrix tm3;
        TMatrix tm4;
        body1->getTm(tm1);
        body2->getTm(tm2);
        body3->getTm(tm3);
        body4->getTm(tm4);

        TMatrix tm1rotation = tm1;
        tm1rotation.setcol(3, Point3(0,0,0));
        TMatrix tm2rotation = tm2;
        tm2rotation.setcol(3, Point3(0,0,0));
        TMatrix tm3rotation = tm3;
        tm3rotation.setcol(3, Point3(0,0,0));


        TA::Mat33 jointOrientationGlobal;

        Point3 axis = Point3(0,0,1);
        Point3 upvector = Point3(0,1,0);

        jointOrientationGlobal.SetToLookAt(toTrueAxisVec3(axis), toTrueAxisVec3(upvector));

        TA::Mat33 jointOrientationLocal = jointOrientationGlobal * obj2->GetFrame().m33Rotation *
          obj1->GetFrame().m33Rotation.GetTransposeAsInverse();

        TA::PhysicsJoint& joint = obj1->AddJoint(
          obj2,
          toTrueAxisVec3(jointpos * inverse(tm1)),
          toTrueAxisVec3(jointpos * inverse(tm2)),
          jointOrientationLocal,
          TA::EulerAngles(0,0,0),
          TA::EulerAngles(0,0,0));

          */

    /*
        {

          Point3 axis = Point3(0,0,1);
          Point3 upvector = Point3(0,1,0);

          TMatrix m;
          m = inverse(tm3rotation) * tm2rotation;

    //      jointOrientation.SetToLookAt(toTrueAxisVec3(upvector * m), toTrueAxisVec3(axis * m));
          jointOrientation.SetToLookAt(toTrueAxisVec3(axis * m), toTrueAxisVec3(upvector * m));

          obj2->AddJointTypeSocket(
            obj3,
            toTrueAxisVec3(jointpos * inverse(tm2)),
            toTrueAxisVec3(jointpos * inverse(tm3)),
            toTrueAxisVec3(-axis * inverse(tm2rotation)),
            toTrueAxisVec3(axis * inverse(tm3rotation)),
            0);
        }
      */


    /*
        jointOrientation.SetToLookAt(toTrueAxisVec3(Point3(0,1,0)),
          toTrueAxisVec3(Point3(0,0,1)));
    //    jointOrientation.SetToLookAt(toTrueAxisVec3(tm.getcol(0) * inverse(tm1)),
    //      toTrueAxisVec3(tm.getcol(2) * inverse(tm1)));
        obj3->AddJointTypeSquareSocket(
          obj4,
          toTrueAxisVec3(jointpos * inverse(tm3)),
          toTrueAxisVec3(jointpos * inverse(tm4)),
          jointOrientation,
          0,0,0,0,0,0);
    //      -1.5,1.5,-1.5,1.5,-1.5,1.5);

    */


    /*
        {
          Point3 pos = Point3(0,30,3);

          TMatrix tm;
          body1->getTm(tm);
          tm.setcol(3, pos);
          body1->setTm(tm);

          body2->getTm(tm);
          tm.setcol(3, pos + Point3(0,0,1.5));
          body2->setTm(tm);

          body3->getTm(tm);
          tm.setcol(3, pos + Point3(0,0,1.5 * 2));
          body3->setTm(tm);

          body4->getTm(tm);
          tm.setcol(3, pos + Point3(0,0,1.5 * 3));
          body4->setTm(tm);
        }
        */


    /*
        //terrain
        {
          TA::StaticObject* staticObject = TA::StaticObject::CreateNew();

          // Create a collision object to add to the static object.
          TA::CollisionObjectAABBMesh* staticCollisionObject =
            TA::CollisionObjectAABBMesh::CreateNew();
          staticCollisionObject->Initialise(
              4,              // Num vertices.
              1,              // Num polygons.
              4);             // Num polygon indices.

          float k_fGroundExtent = 30;

          staticCollisionObject->AddVertex(TA::Vec3(k_fGroundExtent, -10.0f, k_fGroundExtent));
          staticCollisionObject->AddVertex(TA::Vec3(-k_fGroundExtent, -10.0f, k_fGroundExtent));
          staticCollisionObject->AddVertex(TA::Vec3(-k_fGroundExtent, -10.0f, -k_fGroundExtent));
          staticCollisionObject->AddVertex(TA::Vec3(k_fGroundExtent, -10.0f, -k_fGroundExtent));


          int pnPolygonIndexList[4] = { 0, 1, 2, 3 };
          staticCollisionObject->AddPolygon(4, pnPolygonIndexList);
          staticCollisionObject->FinishedAddingGeometry();

          // Initialise the static object with the collision object.
          staticObject->Initialise(staticCollisionObject);
          staticCollisionObject->Release(); // We no long need the reference.
          staticCollisionObject = 0;

          // Add the static object to the simulation.
          TA::Physics::GetInstance().AddStaticObject(staticObject);
          staticObject->Release(); // We no long need the reference.
          staticObject = 0;
        }
        */

#endif

    /*
        PhysBody* body = createBox(Point3(10,-10,0), Point3(50,1,50));
        body->setMassMatrix(0,0,0,0);

        int material1 = physWorld->createNewMaterialId(0,0,.5,.5);
        int material2 = physWorld->createNewMaterialId(0,0,.5,.5);
        physWorld->setMaterialPairProperties(material1, material2, 3,3,.5,.5);

        body1->setMaterialId(material1);
        body2->setMaterialId(material1);
        body3->setMaterialId(material1);
        body4->setMaterialId(material1);

        body->setMaterialId(material2);
        */


    PhysTestDagorGameScene *gameScene = new PhysTestDagorGameScene;
    dagor_select_game_scene(gameScene);
  }


  void shutdown() { closePhysics(); }
} physTestRProc;


static void startupPhysTest() { add_restart_proc(&physTestRProc); }


void DagorWinMainInit(int nCmdShow, bool debugmode) {}

static void post_shutdown_handler()
{
  unload_splash();
  dagor_select_game_scene(NULL);
  DEBUG_CTX("shutdown!");
  shutdown_game(RESTART_INPUT);
  shutdown_game(RESTART_ALL);
  cpujobs::term(true);
}

#if _TARGET_PC_LINUX
#include <signal.h>
#include <util/dag_delayedAction.h>
static void quit_action(void *) { quit_game(0); }
static void __cdecl ctrl_break_handler(int) { add_delayed_callback(quit_action, nullptr); }
#endif

int DagorWinMain(int nCmdShow, bool debugmode)
{
  DataBlock::fatalOnMissingFile = false;
  ::dgs_post_shutdown_handler = post_shutdown_handler;

  dagor_install_dev_fatal_handler(nullptr);
  ::register_common_game_tex_factories();

  // init video
  ::measure_cpu_freq();
  cpujobs::init();
  dgs_load_settings_blk(true, "settings.blk");

  const_cast<DataBlock *>(::dgs_get_settings())->addBlock("video")->setStr("mode", "windowed");
  dagor_init_video("DagorWClass", nCmdShow, NULL /*LoadIcon((HINSTANCE*)win32_get_instance(),"GameIcon")*/, "Loading...");
  debug("initing threadpool for %d cores", cpujobs::get_core_count());
  threadpool::init(eastl::min(cpujobs::get_core_count(), 64), 2048, 256 << 10);
#if _TARGET_PC_LINUX
  signal(SIGINT, ctrl_break_handler);
#endif

  dagor_init_keyboard_win();
  dagor_init_mouse_win();

  startup_shaders("game");
  startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  dagor_common_startup();

  set_default_file_texture_factory();

  ::startup_gui_base("gui/fonts.blk");
  startup_game(RESTART_ALL);

  PhysMat::init("scenes/physmat.blk");
  startupPhysTest();

  startup_game(RESTART_ALL);

  dagor_reset_spent_work_time();
  for (;;)
    dagor_work_cycle();
}

#include <landMesh/lmeshHoles.h>
bool LandMeshHolesCell::check(const Point2 &, const HeightmapHandler *hmapHandler) const { return false; }

#include "dag_cur_view.h"
DagorCurView grs_cur_view;
