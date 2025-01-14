// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"

#include "phys.h"
#include "../av_cm.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_randomSeed.h>
#include <EditorCore/ec_workspace.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <phys/dag_physResource.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <de3_entityFilter.h>
#include <render/dynmodelRenderer.h>

#include <propPanel/control/container.h>
#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_cpuFreq.h>

#include <gui/dag_stdGuiRenderEx.h>

#include <winGuiWrapper/wgw_input.h>
#include <render/dag_cur_view.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

static const int MAX_PHYSOBJ_BODY = 50;
static const int MAX_PHYSOBJ_MAT = 50;

static unsigned collisionMask = 0;
static unsigned rendGeomMask = 0;
static const int auto_inst_seed0 = mem_hash_fnv1(ZERO_PTR<const char>(), 12); // same as entity.cpp

struct PhysSimulationStats
{
  static constexpr const int MEAN_WND_SZ = 20;
  unsigned peakMax = 0, meanMax = 0;
  unsigned meanCur = 0, meanMin = 0;
  unsigned simCount = 0, simSum = 0;
  float simDuration = 0.f;

  void reset()
  {
    peakMax = meanMax = meanCur = meanMin = simCount = simSum = 0;
    simDuration = 0.f;
  }
  bool update(unsigned sim_t, float real_dt)
  {
    bool upd = false;
    simSum += sim_t;
    if (peakMax < sim_t)
      peakMax = sim_t, upd = true;
    if (++simCount == MEAN_WND_SZ)
    {
      meanCur = simSum / MEAN_WND_SZ;
      if (!meanMax)
        meanMax = meanMin = meanCur;
      else if (meanMax < meanCur)
        meanMax = meanCur;
      else if (meanMin > meanCur)
        meanMin = meanCur;
      simSum = simCount = 0;
      upd = true;
    }
    if (!meanCur || meanCur > 100)
      simDuration += real_dt;
    return upd;
  }
};

static inline unsigned getRendSubTypeMask() { return IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER); }
static inline int testBits(int in, int bits) { return (in & bits) == bits; }

class PhysObjViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  enum
  {
    PID_PHYSOBJ_DRAW_BODIES,
    PID_PHYSOBJ_DRAW_C_MASS,
    PID_PHYSOBJ_DRAW_CONSTR,
    PID_PHYSOBJ_DRAW_CONSTR_RS,
    PID_PHYSOBJ_PER_INSTANCE_SEED,

    PID_PHYSOBJ_BODY_GLOBAL_GRP,

    PID_PHYSOBJ_BODY_GRP,
    PID_PHYSOBJ_BODY_TM_0 = PID_PHYSOBJ_BODY_GRP + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_TM_1 = PID_PHYSOBJ_BODY_TM_0 + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_TM_2 = PID_PHYSOBJ_BODY_TM_1 + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_TM_3 = PID_PHYSOBJ_BODY_TM_2 + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_MASS = PID_PHYSOBJ_BODY_TM_3 + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_MOMENT = PID_PHYSOBJ_BODY_MASS + MAX_PHYSOBJ_BODY,

    PID_PHYSOBJ_MATERIAL_GRP = PID_PHYSOBJ_BODY_MOMENT + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_MATERIAL = PID_PHYSOBJ_MATERIAL_GRP + MAX_PHYSOBJ_MAT,

    PID_PHYSOBJ_BODY_COL_GRP = PID_PHYSOBJ_MATERIAL + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_COL_SPH = PID_PHYSOBJ_BODY_COL_GRP + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_COL_BOX = PID_PHYSOBJ_BODY_COL_SPH + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_COL_CAP = PID_PHYSOBJ_BODY_COL_BOX + MAX_PHYSOBJ_BODY,

    PID_PHYSOBJ_BODY_COL_GRP_G = PID_PHYSOBJ_BODY_COL_CAP + MAX_PHYSOBJ_BODY,
    PID_PHYSOBJ_BODY_COL_SPH_G,
    PID_PHYSOBJ_BODY_COL_BOX_G,
    PID_PHYSOBJ_BODY_COL_CAP_G,

    PID_PHYSOBJ_PHYS_TYPE_GRP,
    PID_PHYSOBJ_SIMULATE = PID_PHYSOBJ_PHYS_TYPE_GRP + physsimulator::PHYS_TYPE_LAST,
    PID_PHYSOBJ_SIM_RESTART,
    PID_PHYSOBJ_SIM_PAUSE,
    PID_PHYSOBJ_SIM_SLOWMO,

    PID_PHYSOBJ_SIM_SPRING_FACTOR,
    PID_PHYSOBJ_SIM_DAMPER_FACTOR,
    PID_PHYSOBJ_SIM_TOGGLE_DRIVER,
    PID_PHYSOBJ_SCENE_TYPE_GRP,

    PID_PHYSOBJ_INSTANCE_COUNT,
    PID_PHYSOBJ_INSTANCE_INTERVAL,
  };

  PhysObjViewPlugin() :
    entity(NULL), dynPhysObjData(NULL), nowSimulated(false), dasset(NULL), simulationPaused(false), slowSimulate(false)
  {
    physType = physsimulator::getDefaultPhys();
  }

  ~PhysObjViewPlugin() { end(); }


  virtual const char *getInternalName() const { return "physObjViewer"; }

  virtual void registered()
  {
    rendGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");

    physsimulator::init();
  }
  virtual void unregistered() { physsimulator::close(); }

  virtual bool begin(DagorAsset *asset)
  {
    simulationStat.reset();
    dasset = asset;
    entity = asset ? DAEDITOR3.createEntity(*asset) : NULL;
    const char *name = asset->getName();

    if (!entity || !name)
    {
      DAEDITOR3.conError("can't load asset with name '%s'", asset->getName());
      DAEDITOR3.conShow(true);
      return true;
    }

    entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
    entity->setTm(TMatrix::IDENT);

    if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
      irsh->setPerInstanceSeed(auto_inst_seed0);

    dynPhysObjData = (DynamicPhysObjectData *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), PhysObjGameResClassId);

    slowSimulate = false;
    ::dagor_game_time_scale = 1.0;

    fillPluginPanel();

    return true;
  }
  virtual bool end()
  {
    physsimulator::end();

    nowSimulated = false;
    simulationPaused = false;
    simulationStat.reset();
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

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    if (!entity)
      return false;

    box = entity->getBbox();
    box += Point3(0, physsimulator::def_base_plane_ht, 0);
    return true;
  }

  virtual void actObjects(float dt)
  {
    if (nowSimulated)
    {
      __int64 ref = ref_time_ticks();
      if (!simulationPaused)
        physsimulator::simulate(::dagor_game_time_scale * dt);
      if (simulationStat.update(get_time_usec(ref), ::dagor_game_time_scale * dt))
        repaintView();
    }
  }
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects()
  {
    if (nowSimulated)
    {
      physsimulator::renderTrans(testBits(getRendSubTypeMask(), collisionMask), false, drawBodies, drawCmass, drawConstraints,
        drawConstraintRefsys);
      return;
    }

    if (!dynPhysObjData || !drawCmass)
      return;

    begin_draw_cached_debug_lines();
    set_cached_debug_lines_wtm(TMatrix::IDENT);

    int cnt = dynPhysObjData->physRes->getBodies().size();
    for (int i = 0; i < cnt; i++)
    {
      const PhysicsResource::Body &body = dynPhysObjData->physRes->getBodies()[i];
      const TMatrix &tm = body.tm;
      Point3 c = tm.getcol(3);

      draw_debug_line(c, c + tm.getcol(0), E3DCOLOR(255, 000, 000));
      draw_debug_line(c, c + tm.getcol(1), E3DCOLOR(000, 255, 000));
      draw_debug_line(c, c + tm.getcol(2), E3DCOLOR(000, 000, 255));
    }

    end_draw_cached_debug_lines();
  }

  virtual void renderGeometry(Stage stage)
  {
    if (!getVisible())
      return;

    switch (stage)
    {
      case STG_BEFORE_RENDER: physsimulator::beforeRender(); break;

      case STG_RENDER_DYNAMIC_OPAQUE:
        if (testBits(getRendSubTypeMask(), rendGeomMask))
          physsimulator::render();
        break;

      case STG_RENDER_DYNAMIC_TRANS:
        if (nowSimulated && testBits(getRendSubTypeMask(), rendGeomMask))
          physsimulator::renderTrans(false, true, false, false, false, false);
        break;
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "physObj") == 0; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel)
  {
    if (!entity || !dynPhysObjData)
      return;

    panel.setEventHandler(this);

    PropPanel::ContainerPropertyControl &physTypeGrp = *panel.createRadioGroup(PID_PHYSOBJ_PHYS_TYPE_GRP, "Phys Engine:");
    {
      physTypeGrp.createRadio(physsimulator::PHYS_BULLET, "bullet");
      physTypeGrp.createRadio(physsimulator::PHYS_JOLT, "jolt");

      panel.setInt(PID_PHYSOBJ_PHYS_TYPE_GRP, physType);
    }

    panel.createButton(PID_PHYSOBJ_SIMULATE, "run simulation");
    panel.createButton(PID_PHYSOBJ_SIM_RESTART, "restart simulation", false);

    panel.createButton(PID_PHYSOBJ_SIM_PAUSE, "pause", false);
    panel.createButton(PID_PHYSOBJ_SIM_SLOWMO, slowSimulate ? "normal" : "slow-mo", true, false);

    panel.createEditFloat(PID_PHYSOBJ_SIM_SPRING_FACTOR, "spring factor", physsimulator::springFactor);
    panel.createEditFloat(PID_PHYSOBJ_SIM_DAMPER_FACTOR, "damper factor", physsimulator::damperFactor);
    panel.createCheckBox(PID_PHYSOBJ_DRAW_BODIES, "draw bodies (collision shapes)", drawBodies);
    panel.createCheckBox(PID_PHYSOBJ_DRAW_C_MASS, "draw center mass", drawCmass);
    panel.createCheckBox(PID_PHYSOBJ_DRAW_CONSTR, "draw constraints", drawConstraints);
    panel.createCheckBox(PID_PHYSOBJ_DRAW_CONSTR_RS, "draw constraints refSys", drawConstraintRefsys);
    if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
    {
      panel.createTrackInt(PID_PHYSOBJ_PER_INSTANCE_SEED, "Per-instance seed", irsh->getPerInstanceSeed(), 0, 32767, 1);
      panel.createIndent();
    }

    panel.createSeparator();
    PropPanel::ContainerPropertyControl &sceneTypeGrp = *panel.createRadioGroup(PID_PHYSOBJ_SCENE_TYPE_GRP, "Place physobjs as:");
    {
      sceneTypeGrp.createRadio(physsimulator::SCENE_TYPE_GROUP, "group of objects");
      sceneTypeGrp.createRadio(physsimulator::SCENE_TYPE_STACK, "stack of objects");

      panel.setInt(PID_PHYSOBJ_SCENE_TYPE_GRP, sceneType);
    }
    panel.createEditInt(PID_PHYSOBJ_INSTANCE_COUNT, "physobj instances count", physObjInstanceCount);
    panel.createEditFloat(PID_PHYSOBJ_INSTANCE_INTERVAL, "instance interval size", physObjInstanceInterval);
    panel.createSeparator();

    OAHashNameMap<true> matNames;

    int cnt = dynPhysObjData->physRes->getBodies().size();
    bool expand = (cnt == 1);
    int colSphCnt, colBoxCnt, colCapCnt;
    colSphCnt = colBoxCnt = colCapCnt = 0;

    for (int i = 0; i < cnt; i++)
    {
      const PhysicsResource::Body &body = dynPhysObjData->physRes->getBodies()[i];
      matNames.addNameId(body.materialName.str());

      int sph = body.sphColl.size();
      int box = body.boxColl.size();
      int cap = body.capColl.size();

      for (int j = 0; j < sph; j++)
        matNames.addNameId(body.sphColl[j].materialName.str());

      for (int j = 0; j < box; j++)
        matNames.addNameId(body.boxColl[j].materialName.str());

      for (int j = 0; j < cap; j++)
        matNames.addNameId(body.capColl[j].materialName.str());

      colSphCnt += sph;
      colBoxCnt += box;
      colCapCnt += cap;

      fillPanel(panel, body, expand, i, sph, box, cap);
    }

    PropPanel::ContainerPropertyControl &gStatGrp = *panel.createGroup(PID_PHYSOBJ_BODY_GLOBAL_GRP, "Statistic");

    PropPanel::ContainerPropertyControl &matGrp = *gStatGrp.createGroup(PID_PHYSOBJ_MATERIAL_GRP, "Used materials");

    int matCnt = matNames.nameCount();
    if (matCnt)
    {
      for (int i = 0; i < matCnt; i++)
      {
        const char *mn = matNames.getName(i);
        matGrp.createStatic(PID_PHYSOBJ_MATERIAL + i, mn[0] ? mn : "no material set");
      }
    }
    else
      matGrp.createStatic(PID_PHYSOBJ_MATERIAL, "no material set");

    static String buffer;

    PropPanel::ContainerPropertyControl &colGrp = *gStatGrp.createGroup(PID_PHYSOBJ_BODY_COL_GRP_G, "Collision Statistic");
    {
      bool althoughOne = false;

      if (colCapCnt)
      {
        buffer.printf(128, "capsule: %d", colCapCnt);
        colGrp.createStatic(PID_PHYSOBJ_BODY_COL_CAP_G, buffer);
        althoughOne = true;
      }

      if (colSphCnt)
      {
        buffer.printf(128, "sphere: %d", colSphCnt);
        colGrp.createStatic(PID_PHYSOBJ_BODY_COL_SPH_G, buffer);
        althoughOne = true;
      }

      if (colBoxCnt)
      {
        buffer.printf(128, "box: %d", colBoxCnt);
        colGrp.createStatic(PID_PHYSOBJ_BODY_COL_BOX_G, buffer);
        althoughOne = true;
      }

      if (!althoughOne)
        colGrp.createStatic(PID_PHYSOBJ_BODY_COL_CAP_G, "no collision found");
    }
  }


  void fillPanel(PropPanel::ContainerPropertyControl &panel, const PhysicsResource::Body &body, bool simple, int i, int sph_cnt,
    int box_cnt, int cap_cnt)
  {
    static String caption, uid;
    caption.printf(256, "Body <%s>", body.name.str());
    uid.printf(64, "PhysObjBodyGrp_%d", i);


    PropPanel::ContainerPropertyControl *grp = panel.createGroup(PID_PHYSOBJ_BODY_GRP + i, caption.str());
    {

      static String buffer;

      buffer.printf(128, "mass: %.2f kg", body.mass);
      grp->createStatic(PID_PHYSOBJ_BODY_MASS + i, buffer);

      grp->createStatic(PID_PHYSOBJ_BODY_MOMENT + i, "inertia tensor:");
      buffer.printf(128, "  %.2f, %.2f, %.2f", body.momj.x, body.momj.y, body.momj.z);
      grp->createStatic(PID_PHYSOBJ_BODY_MOMENT + i, buffer);

      if (simple || !(cap_cnt || sph_cnt || box_cnt))
        return;

      buffer.printf(128, "PhysObjCollisionGrp_%d", i);
      PropPanel::ContainerPropertyControl *colGrp = grp->createGroup(PID_PHYSOBJ_BODY_COL_GRP + i, "Collision Statistic");
      {
        if (cap_cnt)
        {
          buffer.printf(128, "capsule: %d", cap_cnt);
          colGrp->createStatic(PID_PHYSOBJ_BODY_COL_CAP + i, buffer);
        }

        if (sph_cnt)
        {
          buffer.printf(128, "sphere: %d", sph_cnt);
          colGrp->createStatic(PID_PHYSOBJ_BODY_COL_SPH + i, buffer);
        }

        if (box_cnt)
        {
          buffer.printf(128, "box: %d", box_cnt);
          colGrp->createStatic(PID_PHYSOBJ_BODY_COL_BOX + i, buffer);
        }
      }
    }
  }


  virtual void postFillPropPanel() {}


  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_PHYSOBJ_DRAW_C_MASS)
      drawCmass = panel->getBool(pcb_id);
    if (pcb_id == PID_PHYSOBJ_DRAW_BODIES)
      drawBodies = panel->getBool(pcb_id);
    if (pcb_id == PID_PHYSOBJ_DRAW_CONSTR)
      drawConstraints = panel->getBool(pcb_id);
    if (pcb_id == PID_PHYSOBJ_DRAW_CONSTR_RS)
      drawConstraintRefsys = panel->getBool(pcb_id);
    else if (pcb_id == PID_PHYSOBJ_PER_INSTANCE_SEED)
    {
      if (entity)
        if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
          irsh->setPerInstanceSeed(panel->getInt(pcb_id));
    }
    else if (pcb_id == PID_PHYSOBJ_PHYS_TYPE_GRP)
    {
      physType = panel->getInt(pcb_id);
      if (physType == PropPanel::RADIO_SELECT_NONE)
        physType = physsimulator::PHYS_DEFAULT;
      restartSimulation(*panel);
    }
    else if (pcb_id == PID_PHYSOBJ_SIM_SPRING_FACTOR)
      physsimulator::springFactor = panel->getFloat(pcb_id);
    else if (pcb_id == PID_PHYSOBJ_SIM_DAMPER_FACTOR)
      physsimulator::damperFactor = panel->getFloat(pcb_id);
    else if (pcb_id == PID_PHYSOBJ_INSTANCE_COUNT)
      physObjInstanceCount = panel->getInt(pcb_id);
    else if (pcb_id == PID_PHYSOBJ_INSTANCE_INTERVAL)
      physObjInstanceInterval = panel->getFloat(pcb_id);
    else if (pcb_id == PID_PHYSOBJ_SCENE_TYPE_GRP)
      sceneType = panel->getInt(pcb_id);
  }


  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {

    if (pcb_id == PID_PHYSOBJ_SIMULATE)
    {
      if (nowSimulated)
      {
        panel->setCaption(pcb_id, "run simulation");
        physsimulator::end();
      }
      else
      {
        if (!physsimulator::begin(dynPhysObjData, physType, physObjInstanceCount, sceneType, physObjInstanceInterval))
          return;
        panel->setCaption(pcb_id, "stop simulation");
      }

      panel->setCaption(PID_PHYSOBJ_SIM_PAUSE, "pause");

      panel->setEnabledById(PID_PHYSOBJ_SIM_RESTART, !nowSimulated);
      panel->setEnabledById(PID_PHYSOBJ_SIM_PAUSE, !nowSimulated);

      nowSimulated = !nowSimulated;
      prepareSimulate(nowSimulated);
    }
    else if (pcb_id == PID_PHYSOBJ_SIM_RESTART)
      restartSimulation(*panel);
    else if (pcb_id == PID_PHYSOBJ_SIM_PAUSE)
    {
      simulationPaused = !simulationPaused;
      if (simulationPaused)
        panel->setCaption(pcb_id, "resume");
      else
        panel->setCaption(pcb_id, "pause");
    }
    else if (pcb_id == PID_PHYSOBJ_SIM_SLOWMO)
    {
      slowSimulate = !slowSimulate;
      ::dagor_game_time_scale = slowSimulate ? 0.1 : 1.0;
      panel->setCaption(pcb_id, slowSimulate ? "normal" : "slow-mo");
    }
  }

  void drawText(IGenViewportWnd *wnd, hdpi::Px x, hdpi::Px y, const String &text)
  {
    using hdpi::_px;
    wnd->drawText(_px(x), _px(y), text);
  }

  void drawText(hdpi::Px x, hdpi::Px y, const char *text, int min_w_pix, E3DCOLOR tc = COLOR_BLACK, E3DCOLOR bc = COLOR_WHITE)
  {
    using hdpi::_px;
    using hdpi::_pxS;

    StdGuiRender::set_color(bc);
    BBox2 box = StdGuiRender::get_str_bbox(text, strlen(text));
    StdGuiRender::render_box(_px(x) + box[0].x - _pxS(2), _px(y) + box[0].y - _pxS(2), //
      _px(x) + _pxS(2) + max<int>(box[1].x, min_w_pix), _px(y) + box[1].y + _pxS(4));

    StdGuiRender::set_color(tc);
    StdGuiRender::draw_strf_to(_px(x), _px(y), text);
  }
  void drawSimulationInfo(IGenViewportWnd *wnd)
  {
    using hdpi::_pxS;

    int x, y;
    wnd->getViewportSize(x, y);

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_WHITE);

    static const int text_max_w = StdGuiRender::get_str_bbox("peak max XXXXX us").size().x;
    hdpi::Px left = _pxActual(x - text_max_w), bottom = _pxActual(y) - _pxScaled(15), ystep = _pxScaled(20);
    drawText(wnd, left, bottom - ystep * 4, String(0, "mean %d us/act", simulationStat.meanCur));
    drawText(wnd, left, bottom - ystep * 3, String(0, "active: %.1f s", simulationStat.simDuration));
    drawText(wnd, left, bottom - ystep * 2, String(0, "peak max %d us", simulationStat.peakMax));
    drawText(wnd, left, bottom - ystep * 1, String(0, "mean max %d us", simulationStat.meanMax));
    drawText(wnd, left, bottom - ystep * 0, String(0, "mean min %d us", simulationStat.meanMin));
  }

  virtual void handleViewportPaint(IGenViewportWnd *wnd)
  {
    drawInfo(wnd);
    if (nowSimulated)
      drawSimulationInfo(wnd);
  }

  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!inside || !nowSimulated)
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);

    springConnected = physsimulator::connectSpring(world, dir);
    return springConnected;
  }

  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!nowSimulated || !springConnected)
      return false;

    springConnected = false;
    return physsimulator::disconnectSpring();
  }

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!nowSimulated || !springConnected)
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);

    physsimulator::setSpringCoord(world, dir);

    return true;
  }

  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!nowSimulated)
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);

    return physsimulator::shootAtObject(world, dir);
  }

protected:
  void prepareSimulate(bool simulate)
  {
    simulationStat.reset();

    if (simulate)
    {
      destroy_it(entity);
      simulationPaused = false;
    }
    else
    {
      entity = dasset ? DAEDITOR3.createEntity(*dasset) : NULL;
      if (entity)
      {
        entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
        entity->setTm(TMatrix::IDENT);
      }
      repaintView();
    }
  }


  void restartSimulation(PropPanel::ContainerPropertyControl &panel)
  {
    if (!nowSimulated)
      return;

    simulationPaused = false;
    panel.setCaption(PID_PHYSOBJ_SIM_PAUSE, "pause");

    simulationStat.reset();
    if (!restartSim())
    {
      nowSimulated = false;
      panel.setEnabledById(PID_PHYSOBJ_SIM_RESTART, true);
      panel.setEnabledById(PID_PHYSOBJ_SIM_PAUSE, true);
      panel.setCaption(PID_PHYSOBJ_SIMULATE, "run simulation");
      prepareSimulate(false);
    }
  }


  virtual bool restartSim()
  {
    if (nowSimulated)
      physsimulator::end();
    return nowSimulated = physsimulator::begin(dynPhysObjData, physType, physObjInstanceCount, sceneType, physObjInstanceInterval);
  }

protected:
  DagorAsset *dasset;
  IObjEntity *entity;
  bool nowSimulated, simulationPaused;
  DynamicPhysObjectData *dynPhysObjData;
  bool drawBodies = false, drawCmass = false, drawConstraints = false, drawConstraintRefsys = false;
  bool springConnected;
  int physType;
  bool slowSimulate;
  PhysSimulationStats simulationStat;
  float curTime = 0.f;
  int sceneType = physsimulator::SCENE_TYPE_GROUP;
  int physObjInstanceCount = 1;
  float physObjInstanceInterval = 0;
};


#define USE_BULLET_PHYSICS 1
#include <phys/dag_vehicle.h>
#include <vehiclePhys/physCar.h>
#include "carDriver.h"
#undef USE_BULLET_PHYSICS
using hdpi::_pxS;

class VehicleViewPlugin : public PhysObjViewPlugin
{
public:
  VehicleViewPlugin() : car(NULL) {}

  virtual const char *getInternalName() const { return "vehicleViewer"; }

  virtual bool begin(DagorAsset *asset)
  {
    physType = physsimulator::getDefaultPhys();
    dasset = asset;
    entity = NULL;
    driverEnabled = false;
    steady = false;
    rwd = fwd = rsa = fsa = false;

    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    const char *cpBlkPath = appBlk.getBlockByNameEx("game")->getStr("car_params", "/game/config/car_params.blk");
    load_car_params_block_from(String(260, "%s%s", ::get_app().getWorkspace().getAppDir(), cpBlkPath));

    nowSimulated = false;
    slowSimulate = false;
    ::dagor_game_time_scale = 1.0;
    car = NULL;
    restartSim();

    if (!car)
    {
      DAEDITOR3.conError("can't create vehicle from '%s' asset", asset->getName());
      DAEDITOR3.conShow(true);
      physsimulator::end();
      nowSimulated = false;
      return true;
    }

    const DataBlock &susp = *asset->props.getBlockByNameEx("suspension");
    fwd = susp.getBlockByNameEx("front")->getBool("powered", true);
    rwd = susp.getBlockByNameEx("rear")->getBool("powered", false);
    fsa = susp.getBlockByNameEx("front")->getBool("rigid_axle", false);
    rsa = susp.getBlockByNameEx("rear")->getBool("rigid_axle", false);

    actObjects(0.01);
    fillPluginPanel();
    return true;
  }
  virtual bool end()
  {
    destroy_it(car);
    physsimulator::end();

    nowSimulated = false;
    simulationPaused = false;
    simulationStat.reset();
    curTime = 0.f;
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

  virtual bool getSelectionBox(BBox3 &box) const
  {
    if (!car)
      return false;

    box = car->getBoundBox();
    return true;
  }

  virtual void actObjects(float dt)
  {
    if (!car)
      return;

    if (nowSimulated && !simulationPaused)
    {
      int64_t ref = ref_time_ticks();
      dt *= ::dagor_game_time_scale;
      curTime += dt;

      if (driverEnabled)
        driver.update(car, dt);
      car->updateBeforeSimulate(dt);
      physsimulator::simulate(dt);
      car->updateAfterSimulate(dt, curTime);

      if (physType == physsimulator::PHYS_BULLET)
        IPhysCar::applyCarPhysModeChangesBullet();

      if (simulationStat.update(get_time_usec(ref), ::dagor_game_time_scale * dt))
        repaintView();

      bool moveless = (car->wheelsOnGround() == car->wheelsCount()) && car->getVel().length() + car->getAngVel().length() * 2 < 0.03;
      if (moveless && !steady)
      {
        steady = true;
        fillPluginPanel();
      }
      else
        steady = moveless;
    }
  }
  virtual void beforeRenderObjects() { PhysObjViewPlugin::beforeRenderObjects(); }
  void renderObjects() override {}
  void renderTransObjects() override
  {
    physsimulator::renderTrans(testBits(getRendSubTypeMask(), collisionMask), false, false, false, false, false);
    if (car)
      car->renderDebug();
  }

  void renderGeometry(Stage stage) override
  {
    IPhysCarLegacyRender *carRender = car ? car->getLegacyRender() : nullptr;
    if (!getVisible() || !carRender)
      return;

    switch (stage)
    {
      case STG_BEFORE_RENDER:
        carRender->beforeRender(::grs_cur_view.pos,
          EDITORCORE->queryEditorInterface<IVisibilityFinderProvider>()->getVisibilityFinder());
        break;

      case STG_RENDER_SHADOWS:
      case STG_RENDER_DYNAMIC_OPAQUE:
        if (testBits(getRendSubTypeMask(), rendGeomMask))
        {
          if (auto *c_inst = carRender->getModel())
            if (!dynrend::render_in_tools(c_inst, dynrend::RenderMode::Opaque))
              c_inst->render();
          for (int w = 0; w < car->wheelsCount(); w++)
            if (auto *w_inst = carRender->getWheelModel(w))
              if (!dynrend::render_in_tools(w_inst, dynrend::RenderMode::Opaque))
                w_inst->render();
        }
        break;

      case STG_RENDER_DYNAMIC_TRANS:
        if (testBits(getRendSubTypeMask(), rendGeomMask))
        {
          if (auto *c_inst = carRender->getModel())
            if (!dynrend::render_in_tools(c_inst, dynrend::RenderMode::Translucent))
              c_inst->render();
          for (int w = 0; w < car->wheelsCount(); w++)
            if (auto *w_inst = carRender->getWheelModel(w))
              if (!dynrend::render_in_tools(w_inst, dynrend::RenderMode::Translucent))
                w_inst->render();
        }
        break;
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "vehicle") == 0; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel)
  {
    panel.setEventHandler(this);

    PropPanel::ContainerPropertyControl &physTypeGrp = *panel.createRadioGroup(PID_PHYSOBJ_PHYS_TYPE_GRP, "Phys Engine:");
    {
      physTypeGrp.createRadio(physsimulator::PHYS_BULLET, "bullet");
      physTypeGrp.createRadio(physsimulator::PHYS_JOLT, "jolt");

      panel.setInt(PID_PHYSOBJ_PHYS_TYPE_GRP, physType);
    }

    panel.createButton(PID_PHYSOBJ_SIM_RESTART, "restart simulation", nowSimulated);
    panel.createButton(PID_PHYSOBJ_SIM_PAUSE, "pause", nowSimulated);
    panel.createButton(PID_PHYSOBJ_SIM_SLOWMO, slowSimulate ? "normal" : "slow-mo", true, false);
    panel.createEditFloat(PID_PHYSOBJ_SIM_SPRING_FACTOR, "spring factor", physsimulator::springFactor);
    panel.createEditFloat(PID_PHYSOBJ_SIM_DAMPER_FACTOR, "damper factor", physsimulator::damperFactor);
    panel.createButton(PID_PHYSOBJ_SIM_TOGGLE_DRIVER, driverEnabled ? "disable driver" : "enable driver (WASD)", nowSimulated);

    TMatrix wtm;
    car->getTm(wtm);
    BBox3 b = car->getLocalBBox();
    Point3 wp0 = wtm * car->getWheelLocalBottomPos(0);
    Point3 wp1 = wtm * car->getWheelLocalBottomPos(1);
    Point3 wp2 = wtm * car->getWheelLocalBottomPos(2);
    Point3 wp3 = wtm * car->getWheelLocalBottomPos(3);
    float trk_f = length(wp1 - wp0);
    float trk_r = length(wp3 - wp2);
    float base = length(wp2 - wp0);
    Point3 cg = car->getMassCenter();
    float wd = (cg - wp2) * (wp0 - wp2) / base / base;
    float cgh = cg.y - (wp0.y + wp1.y + wp2.y + wp3.y) / 4;
    float cgz = (cg - wp0) * (wp1 - wp0) / trk_f / trk_f - 0.5;
    float clearance = b[0].y - car->getWheelLocalBottomPos(0).y;
    for (int i = 1; i < 4; i++)
    {
      float c = b[0].y - car->getWheelLocalBottomPos(i).y;
      if (c < clearance)
        clearance = c;
    }

    panel.createIndent();
    panel.createStatic(-1,
      String(128, "%s  %s  %s", (rwd && fwd) ? "4WD" : (rwd ? "RWD" : "FWD"), fsa ? "Front-SA" : "", rsa ? "Rear-SA" : ""));
    panel.createIndent();
    panel.createStatic(-1, String(128, "Car mass: %.1f kg", car->getMass()));
    panel.createStatic(-1, String(128, "Weight distrbution: %.1f/%.1f (F/R)", wd * 100, (1 - wd) * 100));
    panel.createStatic(-1, String(128, "Wheel base: %.0f mm", base * 1000));
    panel.createStatic(-1, String(128, "Wheel track: %.0f/%.0f mm (F/R)", trk_f * 1000, trk_r * 1000));
    panel.createStatic(-1, String(128, "Cg height: %.0f mm (F/R)", cgh * 1000));
    panel.createStatic(-1, String(128, "Cg side ofs: %.1f mm / %.3f%%", cgz * trk_f * 1000, cgz * 100));
    panel.createStatic(-1, String(128, "SSF: %.3f", (trk_f + trk_r) / 4 / cgh));
    panel.createStatic(-1, String(128, "Clearance=%.0f mm", clearance * 1000));
  }


  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_PHYSOBJ_SIM_TOGGLE_DRIVER)
    {
      driverEnabled = !driverEnabled;
      if (!driverEnabled)
        driver.disable();
      repaintView();
      panel->setCaption(PID_PHYSOBJ_SIM_TOGGLE_DRIVER, driverEnabled ? "disable driver" : "enable driver (WASD)");
    }
    else
      PhysObjViewPlugin::onClick(pcb_id, panel);
  }

  virtual bool restartSim()
  {
    destroy_it(car);
    if (nowSimulated)
      physsimulator::end();

    if (!physsimulator::begin(NULL, physType, physObjInstanceCount, sceneType, physObjInstanceInterval))
      return nowSimulated = false;

    if (physType == physsimulator::PHYS_BULLET)
      car = create_bullet_raywheel_car(dasset->getName(), TMatrix::IDENT, physsimulator::getPhysWorld(), false, false);
    else
      car = NULL;

    if (!car)
    {
      physsimulator::end();
      return nowSimulated = false;
    }

    physsimulator::setTargetObj(car->getCarPhysBody(), dasset->props.getStr("physObj", NULL));

    car->setCarPhysMode(IPhysCar::CPM_ACTIVE_PHYS);
    car->getMachinery()->setAutomaticGearBox(true);
    car->setDriver(&driver);
    car->postReset();
    driver.resetDriver();

    return nowSimulated = true;
  }

  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
  {
    if (vk == wingw::V_RETURN && !modif)
    {
      driverEnabled = !driverEnabled;
      if (!driverEnabled)
        driver.disable();

      if (getPluginPanel())
        getPluginPanel()->setCaption(PID_PHYSOBJ_SIM_TOGGLE_DRIVER, driverEnabled ? "disable driver" : "enable driver (WASD)");

      repaintView();
    }
  }

  virtual void handleViewportPaint(IGenViewportWnd *wnd)
  {
    PhysObjViewPlugin::handleViewportPaint(wnd);

    if (nowSimulated && driverEnabled)
    {
      int x, y;
      wnd->getViewportSize(x, y);

      StdGuiRender::set_font(0);
      drawText(_pxActual(x) - _pxScaled(140), _pxActual(y) - _pxScaled(7), "Driven: WASD", _pxS(140), COLOR_WHITE, COLOR_BLUE);
    }
  }

protected:
  IPhysCar *car;
  TestDriver driver;
  bool driverEnabled;
  bool steady;
  bool rwd, fwd, rsa, fsa;
};


static InitOnDemand<PhysObjViewPlugin> plugin;
static InitOnDemand<VehicleViewPlugin> pluginVehicle;


void init_plugin_physobj()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}

void init_plugin_vehicle()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::pluginVehicle.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::pluginVehicle);
}
