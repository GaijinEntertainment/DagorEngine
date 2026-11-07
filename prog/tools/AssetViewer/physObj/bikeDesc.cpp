// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Preview of a bikeDesc asset: a BLK with a motorcycle{} block and the name of its animChar asset.
// IPhysMotorcycle has only a Jolt backend, so this file is compiled for Jolt, apart from the Bullet code in physObj.cpp.
#define USE_JOLT_PHYSICS 1

#include "../av_plugin.h"
#include "phys.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_animCtrl.h>
#include <EditorCore/ec_input.h>
#include <generic/dag_initOnDemand.h>
#include <animChar/dag_animCharacter2.h>
#include <math/dag_math3d.h>
#include <phys/dag_physics.h>
#include <phys/dag_vehicle.h>
#include <propPanel/control/container.h>
#include <debug/dag_debug.h>

class BikeDescViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  enum
  {
    PID_RESTART = 1,
    PID_TOGGLE_DRIVER,
    PID_DRAW_BODIES,
  };
  static constexpr const char *ENABLE_DRIVER_CAPTION = "enable driver (WAD, R+W back, S brake)";

  const char *getInternalName() const override { return "bikeDescViewer"; }

  void registered() override { physsimulator::init(); }
  void unregistered() override { physsimulator::close(); }

  bool begin(DagorAsset *asset) override
  {
    dasset = asset;
    driverEnabled = false;
    steer = 0;
    if (!loadResources() || !restartSim())
    {
      DAEDITOR3.conError("can't create motorcycle from '%s' asset", asset->getName());
      DAEDITOR3.conShow(true);
    }
    fillPluginPanel();
    return true;
  }
  bool end() override
  {
    destroySim();
    releaseResources();
    dasset = nullptr;
    return true;
  }

  // a rebuilt animChar may carry another skeleton, so the plugin restarts and loads the desc again
  bool reloadOnAssetChanged(const DagorAsset *changed) override
  {
    return dasset && changed->getType() == DAEDITOR3.getAssetTypeId("animChar") &&
           strcmp(changed->getName(), dasset->props.getStr("animChar", "")) == 0;
  }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
  {
    if (!entity)
      return false;
    TMatrix tm;
    entity->getTm(tm);
    box = tm * entity->getBbox();
    return true;
  }

  // dt is already time-scaled; under the DNG-based render the game has just started an async step of the shared world,
  // and simulate() does nothing
  void actObjects(float dt) override
  {
    if (!bike)
      return;
    ((PhysWorld *)physsimulator::getPhysWorld())->fetchSimRes(true);
    if (driverEnabled)
      applyDriverInput(dt);
    physsimulator::simulate(dt);
    if (AnimV20::AnimcharBaseComponent *animChar = getAnimChar())
      bike->syncNodes(animChar->getNodeTree());
    TMatrix chassisTm;
    chassis->getTm(chassisTm);
    // the entity renders itself; animChar places the root from it
    entity->setTm(chassisTm * inverse(bike->getBikeGeometrySetup().chassisInModel));
  }
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects(BaseTexture *) override
  {
    if (bike)
      physsimulator::renderTrans(false, false, drawBodies, false, false, false, true);
  }

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "bikeDesc") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override
  {
    panel.setEventHandler(this);
    panel.createButton(PID_RESTART, "restart simulation", entity != nullptr);
    panel.createButton(PID_TOGGLE_DRIVER, driverEnabled ? "disable driver" : ENABLE_DRIVER_CAPTION, bike != nullptr);
    panel.createCheckBox(PID_DRAW_BODIES, "draw bodies (collision shapes)", drawBodies);
    if (bike)
    {
      panel.createIndent();
      panel.createStatic(-1, String(128, "Mass: %.1f kg", chassis->getMass()));
      panel.createStatic(-1, String(128, "Spring frequency: %.2f/%.2f Hz (F/R)", frontSpringHz, rearSpringHz));
    }
  }
  void postFillPropPanel() override {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_DRAW_BODIES)
      drawBodies = panel->getBool(pcb_id);
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_RESTART)
    {
      restartSim();
      fillPluginPanel();
    }
    else if (pcb_id == PID_TOGGLE_DRIVER)
    {
      driverEnabled = !driverEnabled;
      panel->setCaption(pcb_id, driverEnabled ? "disable driver" : ENABLE_DRIVER_CAPTION);
      if (!driverEnabled && bike)
        bike->setDriverInput(0, 0, 0), steer = 0;
    }
    repaintView();
  }

private:
  // under the DNG-based render the component lives in ECS storage, which moves when entities are created, so it is never cached
  AnimV20::AnimcharBaseComponent *getAnimChar() const
  {
    IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : nullptr;
    return animCtrl && !animCtrl->getAnimGraph() ? animCtrl->getAnimCharBase() : nullptr;
  }

  // the vehicle poses the animChar's own node tree and reads the rest pose from its skeleton gameres, as game code would
  bool loadResources()
  {
    const char *animCharName = dasset->props.getStr("animChar", "");
    const DagorAsset *animCharAsset = DAEDITOR3.getAssetByName(animCharName, DAEDITOR3.getAssetTypeId("animChar"));
    entity = animCharAsset ? DAEDITOR3.createEntity(*animCharAsset) : nullptr;
    AnimV20::AnimcharBaseComponent *animChar = getAnimChar();
    if (!entity || !animChar || !animChar->getOriginalNodeTree())
    {
      DAEDITOR3.conError("bikeDesc <%s>: animChar:t=<%s> should name an animChar with a skeleton and no anim graph", dasset->getName(),
        animCharName);
      destroy_it(entity);
      return false;
    }
    entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
    entity->setTm(TMatrix::IDENT);
    return true;
  }

  void releaseResources() { destroy_it(entity); }

  bool restartSim()
  {
    destroySim();
    AnimV20::AnimcharBaseComponent *animChar = getAnimChar();
    if (!animChar || !animChar->getOriginalNodeTree())
      return false;

    // loadDesc() reads the gravity of the physics engine, so it runs after begin()
    if (!physsimulator::begin(nullptr, physsimulator::PHYS_JOLT, 1, physsimulator::SCENE_TYPE_GROUP, 0))
      return false;
    PhysWorld *pw = (PhysWorld *)physsimulator::getPhysWorld();
    IPhysMotorcycle::Desc desc;
    if (!IPhysMotorcycle::loadDesc(*dasset->props.getBlockByNameEx("motorcycle"), *animChar->getOriginalNodeTree(), desc))
    {
      destroySim();
      return false;
    }

    // the model origin is at the ground, so the bike stands on the base plane with a small gap
    TMatrix modelTm = TMatrix::IDENT;
    modelTm.setcol(3, 0, physsimulator::def_base_plane_ht + 0.05f, 0);

    PhysBoxCollision chassisColl(desc.boxSize.x, desc.boxSize.y, desc.boxSize.z);
    PhysBodyCreationData pbcd;
    pbcd.centerOfMassOffset = desc.comOffset;
    pbcd.momentOfInertia = desc.inertia;
    chassis = new PhysBody(pw, desc.mass, &chassisColl, modelTm * desc.chassisInModel, pbcd);
    bike = IPhysMotorcycle::createJolt(chassis, desc);
    if (!bike)
    {
      destroySim();
      return false;
    }
    frontSpringHz = desc.settings.frontWheel.suspensionFrequency;
    rearSpringHz = desc.settings.rearWheel.suspensionFrequency;
    return true;
  }

  void destroySim()
  {
    PhysWorld *pw = (PhysWorld *)physsimulator::getPhysWorld(); // null until begin() and after end()
    if (pw)
      pw->fetchSimRes(true); // the DNG-based render may still run the step that uses the bodies
    if (bike)
      bike->destroyVehicle();
    bike = nullptr;
    del_it(chassis);
    if (pw)
      physsimulator::end();
  }

  void applyDriverInput(float dt)
  {
    // R+W drives backward; S brakes like Space, and holds the bike once it stops
    float forward = ec_is_key_down(ImGuiKey_W) ? (ec_is_key_down(ImGuiKey_R) ? -1.f : 1.f) : 0.f;
    float brake = ec_is_key_down(ImGuiKey_S) || ec_is_key_down(ImGuiKey_Space) ? 1.f : 0.f;

    // digital keys turn the bars gradually, full lock in about 1 s; the bars return 3 times faster
    const float STEER_RATE = 1.f, STEER_RETURN_RATE = 3.f;
    float target = (ec_is_key_down(ImGuiKey_A) ? 1.f : 0.f) - (ec_is_key_down(ImGuiKey_D) ? 1.f : 0.f);
    float rate = (target != 0 && target * steer >= 0) ? STEER_RATE : STEER_RETURN_RATE;
    steer = target > steer ? min(steer + rate * dt, target) : max(steer - rate * dt, target);

    if (forward != 0 || steer != 0 || brake != 0)
      bike->activate();
    // full brakes in a lean make the bike spin out
    bike->setDriverInput(forward, steer, brake * sqr(1.f - bike->getAbsSinLeanAngle()));
  }

  DagorAsset *dasset = nullptr;
  IObjEntity *entity = nullptr;

  PhysBody *chassis = nullptr;
  IPhysMotorcycle *bike = nullptr;
  float frontSpringHz = 0.f, rearSpringHz = 0.f;
  float steer = 0; // smoothed left input
  bool driverEnabled = false;
  bool drawBodies = false;
};

static InitOnDemand<BikeDescViewPlugin> plugin;

void init_plugin_bike_desc()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
