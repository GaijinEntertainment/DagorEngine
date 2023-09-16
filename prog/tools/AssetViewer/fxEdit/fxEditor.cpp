#include "../av_plugin.h"
#include "../av_tree.h"
#include "../av_appwnd.h"
#include <assets/assetRefs.h>
#include <assetsGui/av_selObjDlg.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <scriptHelpers/scriptHelpers.h>
#include <scriptHelpers/scriptPanel.h>

#include <fx/dag_paramScriptsPool.h>
#include <fx/dag_baseFxClasses.h>
#include <fx/dag_fxInterface.h>
#include <fx/dag_commonFx.h>
#include <fx/commonFxTools.h>
#include <gameRes/dag_stdGameRes.h>

// #include <EditorCore/ec_interface.h>
#include <EditorCore/ec_cm.h>
#include <de3_interface.h>

#include <fx/effectClassTools.h>
#include <shaders/dag_shaderMesh.h>
#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_control_event_handler.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <propPanel2/comWnd/panel_window.h>
#include <propPanel2/comWnd/tool_window.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel2/comWnd/list_dialog.h>
#include <sepGui/wndPublic.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_string.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3d.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_TMatrix4.h>
#include <gui/dag_stdGuiRenderEx.h>

#include <dafxToolsHelper/dafxToolsHelper.h>

#include "../av_cm.h"
#include "fxSaveLoad.h"

namespace dafx_ex
{
#include <daFx/dafx_def.hlsli>
#include "../../commonFx/commonFxGame/dafx_globals.hlsli"
} // namespace dafx_ex

// Implemented in fxMgrService.cpp
extern dafx::ContextId g_dafx_ctx;
extern dafx::CullingId g_dafx_cull;
extern dafx::Stats g_dafx_stats;
extern float g_dafx_dt_mul;

enum
{
  CM_RELOAD_SCRIPT = CM_PLUGIN_BASE + 1,
  CM_EFFECT_CLASS_NAME,
  CM_REVERT,

  PID_CHANGE_REF = 1,
  PID_RESET_REF = PID_CHANGE_REF + 32,
  PID_SHOW_DEBUG = PID_RESET_REF + 1,
  PID_SHOW_STATIC_VIS_SPHERE = PID_SHOW_DEBUG + 1,
  PID_SHOW_VOLUME_DEBUG = PID_SHOW_STATIC_VIS_SPHERE + 1,
  PID_DEBUG_MOTION = PID_SHOW_VOLUME_DEBUG + 1,
  PID_DEBUG_MOTION_DIR = PID_DEBUG_MOTION + 1,
  PID_DEBUG_MOTION_DISTANCE = PID_DEBUG_MOTION + 2,
  PID_DEBUG_MOTION_SPEED = PID_DEBUG_MOTION + 3,
  PID_DEBUG_TRAIL_WIDTH = PID_DEBUG_MOTION + 4,
  PID_DEBUG_TRAIL_VERTICAL = PID_DEBUG_MOTION + 5,
  PID_DEBUG_TRAIL_INDEX = PID_DEBUG_MOTION + 6,
  PID_DEBUG_EMITTER_ROT_X = PID_DEBUG_TRAIL_INDEX + 1,
  PID_DEBUG_EMITTER_ROT_Y = PID_DEBUG_EMITTER_ROT_X + 1,
  PID_DEBUG_TIME_SPEED = PID_DEBUG_EMITTER_ROT_Y + 1,
  PID_DEBUG_WIND_DIR = PID_DEBUG_TIME_SPEED + 1,
  PID_DEBUG_WIND_SPEED = PID_DEBUG_WIND_DIR + 1,
  PID_DEBUG_QUALITY = PID_DEBUG_WIND_SPEED + 1,
};

extern bool particles_debug_render;

bool particles_debug_static_vis_sphere = false;

bool particles_debug_volume = false;

bool particles_debug_motion = false;
bool particles_debug_motion_dir = false;
float particles_debug_motion_distance = 1;
float particles_debug_motion_speed = 1;
float particles_debug_trail_width = 1;
float particles_debug_trail_vertical = 0;
int particles_debug_trail_index = 0;
float particles_debug_rot_x = 0;
float particles_debug_rot_y = 0;
float particles_debug_time_speed = 1;
float particles_debug_wind_speed = 0;
float particles_debug_wind_dir = 0;
TMatrix particles_debug_tm = TMatrix::IDENT;

int particles_debug_trail_id = -1;
float particles_debug_motion_time = 0;
int particles_quality_preview = 2;

static int dynamic_lights_countVarId = -1;
static int simple_point_light_pos_radiusVarId = -1;
static int simple_point_light_color_specVarId = -1;
static int simple_point_light_box_centerVarId = -1;
static int simple_point_light_box_extentVarId = -1;

class EffectsPluginSaveCB : public ScriptHelpers::SaveValuesCB
{
public:
  struct Slot
  {
    String name, typeName, ref;
  };

  Tab<Slot> slots;

  EffectsPluginSaveCB() : slots(tmpmem) {}

  int findRefSlot(const char *name) const
  {
    for (int i = 0; i < slots.size(); ++i)
      if (stricmp(slots[i].name, name) == 0)
        return i;
    return -1;
  }

  const char *convertTypeName(const char *type_name)
  {
    if (stricmp(type_name, "texture") == 0)
      return "tex";
    if (stricmp(type_name, "normal") == 0)
      return "tex";
    if (stricmp(type_name, "Effects") == 0)
      return "fx";
    return "?";
  }

  virtual void addRefSlot(const char *name, const char *type_name, bool make_unique)
  {
    String slotName(name);

    for (;;)
    {
      int id = findRefSlot(slotName);
      if (id < 0)
      {
        Slot &s = slots.push_back();
        s.name = slotName;
        s.typeName = convertTypeName(type_name);
        break;
      }

      if (!make_unique)
        return;

      ::make_next_numbered_name(slotName, 2);
    }
  }

  void saveRefSlots(DataBlock &main_blk)
  {
    for (int i = 0; i < slots.size(); ++i)
    {
      DataBlock &blk = *main_blk.addNewBlock("ref");
      blk.setStr("type", slots[i].typeName);
      blk.setStr("slot", slots[i].name);
      if (!slots[i].ref.empty())
        blk.setStr("ref", slots[i].ref);
    }
  }

  void addRef(const DataBlock &b)
  {
    int id = findRefSlot(b.getStr("slot", ""));
    if (id < 0)
    {
      debug("skip obsolete ref: %s", b.getStr("slot", ""));
      return;
    }
    if (stricmp(slots[id].typeName, b.getStr("type", "")) != 0)
    {
      debug("skip type-changed ref: %s  %s->%s", b.getStr("name", ""), slots[id].typeName.str(), b.getStr("type", ""));
      return;
    }
    slots[id].ref = b.getStr("ref", "");
  }
};


enum
{
  EFFECTS_TOOL_WINDOW_TYPE = 20, // just not to conflict with other types
  TOOL_PANEL_HEIGHT = 26,
  TREE_PANEL_HEIGHT = 120,
};


class EffectsPlugin : public IGenEditorPlugin,
                      public ScriptHelpers::ParamChangeCB,
                      public ControlEventHandler,
                      public IWndManagerWindowHandler
{
public:
  EffectsPlugin() :
    isModified(false), scriptClasses(midmem), effect(NULL), mToolPanel(NULL), hwndPanel(0), hwndTool(0), hwndTree(0), curAsset(NULL)
  {
    dynamic_lights_countVarId = ::get_shader_variable_id("dynamic_lights_count", true);
    simple_point_light_pos_radiusVarId = ::get_shader_variable_id("simple_point_light_pos_radius", true);
    simple_point_light_color_specVarId = ::get_shader_variable_id("simple_point_light_color_spec", true);
    simple_point_light_box_centerVarId = ::get_shader_variable_id("simple_point_light_box_center", true);
    simple_point_light_box_extentVarId = ::get_shader_variable_id("simple_point_light_box_extent", true);
  }
  ~EffectsPlugin()
  {
    destroyFx();
    ScriptHelpers::set_param_change_cb(NULL);
  }

  virtual const char *getInternalName() const { return "Effects"; }

  virtual void registered() { register_all_common_fx_tools(); }
  virtual void unregistered() {}

  virtual bool havePropPanel() { return true; }

  virtual bool begin(DagorAsset *asset)
  {
    IWndManager &manager = getWndManager();
    manager.registerWindowHandler(this);

    hwndPanel = getAdditinalPropWindow();
    G_ASSERT(hwndPanel);
    hwndTool = manager.splitWindow(hwndPanel, 0, TOOL_PANEL_HEIGHT, WA_TOP);
    G_ASSERT(hwndTool);
    hwndTree = manager.splitWindow(hwndPanel, 0, TREE_PANEL_HEIGHT, WA_TOP);
    G_ASSERT(hwndTree);

    manager.fixWindow(hwndTool, true);
    manager.setWindowType(hwndTool, EFFECTS_TOOL_WINDOW_TYPE);

    curAsset = asset;

    ScriptHelpers::obj_editor = &effOE;
    ScriptHelpers::create_helper_bar(&manager, hwndTree, hwndPanel);

    className = asset->props.getStr("className", NULL);

    fillDeclPanel(/*save_and_restore_params = */ false);

    ScriptHelpers::load_helpers(*asset->props.getBlockByNameEx("params"));
    saveResource();
    ScriptHelpers::set_param_change_cb(this);

    reloadScript();

    setDirty(false);

    // effOE.initUi();
    fillDeclPanel();

    return true;
  }
  virtual bool end()
  {
    IWndManager &manager = getWndManager();

    manager.removeWindow(hwndTree);
    manager.removeWindow(hwndTool);

    manager.setWindowType(hwndPanel, CLIENT_WINDOW_TYPE_NONE);

    manager.unregisterWindowHandler(this);

    if (curAsset)
      saveResource();

    // IEditorCoreEngine::get()->setGizmo(NULL, IEditorCoreEngine::MODE_None);

    ScriptHelpers::obj_editor->removeAllObjects(false);
    ScriptHelpers::obj_editor->setEditMode(CM_OBJED_MODE_SELECT);
    ScriptHelpers::obj_editor = NULL;

    ShaderGlobal::set_int(dynamic_lights_countVarId, 0);

    curAsset = NULL;

    destroyFx();
    scriptClasses.clear();

    return true;
  }

  virtual IWndEmbeddedWindow *onWmCreateWindow(void *handle, int type)
  {
    switch (type)
    {
      case EFFECTS_TOOL_WINDOW_TYPE:
      {
        if (mToolPanel)
          return NULL;

        IWndManager &manager = getWndManager();
        manager.setCaption(handle, "Effects Tools");

        unsigned w, h;
        manager.getWindowClientSize(handle, w, h);
        CToolWindow *toolPanel = new CToolWindow(this, handle, 0, 0, w, h, "Effects Tools");

        mToolPanel = toolPanel;
        fillToolBar(*mToolPanel->createToolbarPanel(21, ""));

        return toolPanel;
      }
    }

    return ScriptHelpers::on_wm_create_window(handle, type);
  }
  virtual bool onWmDestroyWindow(void *handle)
  {
    if (mToolPanel && mToolPanel->getParentWindowHandle() == handle)
    {
      del_it(mToolPanel);
      return true;
    }

    return ScriptHelpers::on_wm_destroy_window(handle);
  }

  virtual void clearObjects() {}

  virtual void onSaveLibrary()
  {
    if (curAsset)
      saveResource();
  }

  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    if (!effect)
      return false;

    BaseParticleFxEmitter *emitter = (BaseParticleFxEmitter *)effect->getParam(HUID_EMITTER, NULL);
    if (!emitter)
    {
      box.makecube(Point3(0, 0, 0), 4);
      return true;
    }

    Point3 center;
    real rad;
    emitter->getEmitterBoundingSphere(center, rad);

    box.makecube(center, (rad + 0.50f) * 4);

    return true;
  }

  virtual void actObjects(real dt)
  {
    dt *= particles_debug_time_speed;

    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->update(dt);

    if (effect)
    {
      particles_debug_tm = rotyTM(-particles_debug_rot_x * 3.14);
      particles_debug_tm *= rotzTM(-particles_debug_rot_y * 3.14);

      ShaderGlobal::set_int(::get_shader_variable_id("modfx_debug_render", true), particles_debug_render);

      if (particles_debug_motion)
      {
        particles_debug_motion_time += particles_debug_motion_speed * dt;
        Point3 dir = Point3(sin(particles_debug_motion_time), 0, cos(particles_debug_motion_time));
        Point3 pos = particles_debug_motion_distance * dir;

        Point3 speed = particles_debug_motion_speed * particles_debug_motion_distance *
                       Point3(cos(particles_debug_motion_time), 0, -sin(particles_debug_motion_time));

        effect->setParam(HUID_VELOCITY, &speed.x);

        if (particles_debug_motion_dir)
        {
          float s = particles_debug_motion_speed >= 0 ? 1 : -1;
          Point3 nextDir = Point3(sin(particles_debug_motion_time + 1), 0, cos(particles_debug_motion_time + 1));
          Point3 nextPos = particles_debug_motion_distance * nextDir;
          Point3 fwdDir = normalize(nextPos - pos);
          Point3 leftDir = normalize(cross(fwdDir, float3(0, 1, 0)));

          TMatrix dirTm;
          dirTm.setcol(0, leftDir);
          dirTm.setcol(1, fwdDir);
          dirTm.setcol(2, float3(0, 1, 0));
          dirTm.setcol(3, float3(0, 0, 0));

          particles_debug_tm *= dirTm;
        }

        if (effect->isSubOf(HUID_BaseTrailEffect))
        {
          Point3 trailDir = Point3(0, particles_debug_trail_vertical, 0) + dir * (1 - particles_debug_trail_vertical);
          trailDir.normalize();
          ((BaseTrailEffect *)effect)
            ->addTrailPoint(particles_debug_trail_id, pos, Point3(0, 0, 0), trailDir * particles_debug_trail_width);
        }
        else
          particles_debug_tm.setcol(3, pos);
      }
      else
      {
        Point3 pz = {0, 0, 0};
        effect->setParam(HUID_VELOCITY, &pz);
      }

      Point2 windDir = Point2(cos(particles_debug_wind_dir), -sin(particles_debug_wind_dir));
      set_dafx_wind_params(g_dafx_ctx, windDir, particles_debug_wind_speed, effectTotalLifeTime);

      Point4 p4 = Point4::xyz0(particles_debug_tm.getcol(3));
      effect->setParam(_MAKE4C('PFXP'), &p4);
      effect->setParam(HUID_EMITTER_TM, &particles_debug_tm);
      effect->update(dt);
      IEditorCoreEngine::get()->invalidateViewportCache();
      IEditorCoreEngine::get()->updateViewports();
    }
    else
    {
      particles_debug_tm = TMatrix::IDENT;
    }

    setupLight();

    effectTotalLifeTime += dt;
  }
  void drawDafxStats(BaseEffectObject &effect)
  {
    if (!g_dafx_ctx)
      return;

    bool started = StdGuiRender::is_render_started();
    if (!started)
      StdGuiRender::continue_render();

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(E3DCOLOR_MAKE(255, 255, 255, 255));

    const dafx::Stats &s = g_dafx_stats;
    String insts(128, "instances: %d", (int)(s.activeInstances / 2)); // instances are 2 part objects
    String cpuSim(128, "cpuSimulation: %d", s.cpuElemProcessed);
    String gpuSim(128, "gpuSimulation: %d", s.gpuElemProcessed);
    String dips(128, "dips: %d", s.renderDrawCalls);
    String tris(128, "tris: %d/%d", s.visibleTriangles, s.renderedTriangles);

    int fxStats[4] = {0};
    effect.getParam(_MAKE4C('PFXX'), fxStats);
    String params(128, "param_ren: %d, param_sim: %d", fxStats[0], fxStats[1]);
    String data(128, "part_ren: %d, part_sim: %d", fxStats[2], fxStats[3]);

    int y = 40;
    int n = 20;
    StdGuiRender::draw_strf_to(8, y + n * 0, insts);
    StdGuiRender::draw_strf_to(8, y + n * 1, cpuSim);
    StdGuiRender::draw_strf_to(8, y + n * 2, gpuSim);
    StdGuiRender::draw_strf_to(8, y + n * 3, dips);
    StdGuiRender::draw_strf_to(8, y + n * 4, tris);
    StdGuiRender::draw_strf_to(8, y + n * 5, data);
    StdGuiRender::draw_strf_to(8, y + n * 6, params);

    StdGuiRender::flush_data();
    if (!started)
      StdGuiRender::end_render();
  }
  virtual void beforeRenderObjects()
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->beforeRender();
  }
  virtual void renderObjects()
  {
    if (ScriptHelpers::obj_editor)
    {
      begin_draw_cached_debug_lines();
      ScriptHelpers::obj_editor->render();
      end_draw_cached_debug_lines();
    }
  }
  virtual void renderTransObjects()
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->renderTrans();

    if (effect)
    {
      if (particles_debug_static_vis_sphere)
      {
        BSphere3 sphere;
        effect->getParam(HUID_STATICVISSPHERE, &sphere);

        if (!sphere.isempty())
          draw_debug_sph(sphere, E3DCOLOR(255, 255, 255));
      }
    }

    if (!g_dafx_ctx)
      return;

    dafx::render_debug_opt(g_dafx_ctx);
    if (effect)
    {
      if (particles_debug_static_vis_sphere)
      {
        Point3 pos = particles_debug_tm.getcol(3);
        Point3 xAsix = particles_debug_tm.getcol(0);
        Point3 yAsix = particles_debug_tm.getcol(1);
        Point3 zAsix = particles_debug_tm.getcol(2);

        begin_draw_cached_debug_lines(false, false);
        set_cached_debug_lines_wtm(TMatrix::IDENT);
        draw_cached_debug_line(pos, pos + xAsix, E3DCOLOR(255, 0, 0, 255));
        draw_cached_debug_line(pos, pos + yAsix, E3DCOLOR(0, 255, 0, 255));
        draw_cached_debug_line(pos, pos + zAsix, E3DCOLOR(0, 0, 255, 255));
        end_draw_cached_debug_lines();
      }

      if (particles_debug_volume)
      {
        Point3 pos = particles_debug_tm.getcol(3);
        effect->drawEmitter(pos);
      }

      drawDafxStats(*effect);
    }
  }
  virtual void renderGeometry(Stage stage)
  {
    if (!effect || !getVisible())
      return;

    switch (stage)
    {
      case STG_BEFORE_RENDER: effect->render(FX_RENDER_BEFORE); break;

      case STG_RENDER_DYNAMIC_OPAQUE: effect->render(FX_RENDER_SOLID); break;

      case STG_RENDER_FX: effect->render(FX_RENDER_TRANS); break;

      case STG_RENDER_FX_DISTORTION:
        effect->render(FX_RENDER_DISTORTION);
        dafx::render(g_dafx_ctx, g_dafx_cull, "distortion");
        break;
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const
  {
    return asset.getFileNameId() >= 0 && strcmp(asset.getTypeStr(), "fx") == 0;
  }

  void updateDebugMotionControllers(PropPanel2 *panel)
  {
    panel->setEnabledById(PID_DEBUG_MOTION_DISTANCE, particles_debug_motion);
    panel->setEnabledById(PID_DEBUG_MOTION_SPEED, particles_debug_motion);
    panel->setEnabledById(PID_DEBUG_MOTION_DIR, particles_debug_motion);

    bool trail = effect && effect->isSubOf(HUID_BaseTrailEffect);
    panel->setEnabledById(PID_DEBUG_TRAIL_WIDTH, particles_debug_motion && trail);
    panel->setEnabledById(PID_DEBUG_TRAIL_VERTICAL, particles_debug_motion && trail);
    panel->setEnabledById(PID_DEBUG_TRAIL_INDEX, particles_debug_motion && trail);
  }

  virtual void fillPropPanel(PropertyContainerControlBase &panel)
  {
    panel.setEventHandler(this);
    panel.clear();
    panel.createCheckBox(PID_SHOW_DEBUG, "Debug particles render", particles_debug_render);
    panel.createCheckBox(PID_SHOW_STATIC_VIS_SPHERE, "Debug static visibility", particles_debug_static_vis_sphere);
    panel.createCheckBox(PID_SHOW_VOLUME_DEBUG, "Debug volume", particles_debug_volume);

    panel.createCheckBox(PID_DEBUG_MOTION, "Debug emitter motion", particles_debug_motion);
    panel.createCheckBox(PID_DEBUG_MOTION_DIR, "Debug emitter direction", particles_debug_motion_dir);
    panel.createEditFloat(PID_DEBUG_MOTION_DISTANCE, "   motion distance", particles_debug_motion_distance);
    panel.createEditFloat(PID_DEBUG_MOTION_SPEED, "   motion speed", particles_debug_motion_speed);
    panel.createEditFloat(PID_DEBUG_TRAIL_WIDTH, "    trail width", particles_debug_trail_width);
    panel.createTrackFloat(PID_DEBUG_TRAIL_VERTICAL, "    trail vertical", particles_debug_trail_vertical, 0, 1, 0.1);
    panel.createEditInt(PID_DEBUG_TRAIL_INDEX, "    trail index", particles_debug_trail_index);
    panel.createTrackFloat(PID_DEBUG_EMITTER_ROT_X, "    debug rotation y", particles_debug_rot_x, 0, 1, 0.01);
    panel.createTrackFloat(PID_DEBUG_EMITTER_ROT_Y, "    debug rotation z", particles_debug_rot_y, 0, 1, 0.01);
    panel.createTrackFloat(PID_DEBUG_TIME_SPEED, "    debug time speed", particles_debug_time_speed, 0, 5, 0.01);
    panel.createTrackFloat(PID_DEBUG_WIND_SPEED, "    debug wind speed", particles_debug_wind_speed, 0, 20, 0.01);
    panel.createTrackFloat(PID_DEBUG_WIND_DIR, "    debug wind angle", particles_debug_wind_dir, 0, 6.28, 0.01);

    Tab<String> qualityNames(tmpmem);
    qualityNames.push_back(String("low"));
    qualityNames.push_back(String("medium"));
    qualityNames.push_back(String("high"));
    panel.createCombo(PID_DEBUG_QUALITY, "Quality preview:", qualityNames, particles_quality_preview);

    updateDebugMotionControllers(&panel);

    const DataBlock &main_blk = curAsset->props;
    int refNameId = main_blk.getNameId("ref"), refIdx = 0;
    for (int bi = 0; bi < main_blk.blockCount(); ++bi)
    {
      const DataBlock &b = *main_blk.getBlock(bi);
      if (b.getBlockNameId() != refNameId)
        continue;

      panel.createIndent();
      panel.createStatic(0, String(64, "Slot #%d: %s", refIdx + 1, b.getStr("slot", "slot")));
      panel.createButton(PID_CHANGE_REF + refIdx, b.getStr("ref", ""));

      refIdx++;
    }
  }

  virtual void postFillPropPanel() {}

  // ControlEventHandler
  virtual void onClick(int pid, PropertyContainerControlBase *panel)
  {
    int refIdx = -1;
    if (pid >= PID_CHANGE_REF && pid < PID_CHANGE_REF + 32)
      refIdx = pid - PID_CHANGE_REF;

    DataBlock *b = NULL;
    if (refIdx >= 0)
    {
      DataBlock &main_blk = curAsset->props;
      int refNameId = main_blk.getNameId("ref"), idx = 0;
      for (int bi = 0; bi < main_blk.blockCount(); ++bi)
      {
        b = main_blk.getBlock(bi);
        if (b->getBlockNameId() == refNameId)
        {
          if (refIdx == idx)
            break;
          idx++;
        }
        b = NULL;
      }

      if (!b)
        return;

      {
        const char *type_name = b->getStr("type", "");
        int type = curAsset->getMgr().getAssetTypeId(type_name);
        if (type < 0)
          return;

        String sel_type(64, "Select %s", type_name), reset_type(64, "Reset %s", type_name);

        int _x, _y;
        unsigned _w, _h;

        G_ASSERT(getPropPanel() && "Plugin panel closed!");
        if (!getWndManager().getWindowPosSize(getPropPanel()->getParentWindowHandle(), _x, _y, _w, _h))
          return;
        getWndManager().clientToScreen(_x, _y);

        SelectAssetDlg dlg(0, &curAsset->getMgr(), sel_type, sel_type, reset_type, make_span_const(&type, 1), _x - _w - 5, _y, _w, _h);

        dlg.selectObj(b->getStr("ref", ""));

        int ret = dlg.showDialog();
        if (ret == DIALOG_ID_CLOSE)
          return;

        b->setStr("ref", (ret == DIALOG_ID_OK) ? dlg.getSelObjName() : "");
        panel->setCaption(pid, b->getStr("ref", ""));
      }

      reloadScript();
    }

    // if (ScriptHelpers::obj_editor)
    //   ScriptHelpers::obj_editor->handleCommand(cmd);

    switch (pid)
    {
      case CM_RELOAD_SCRIPT:
        fillDeclPanel();
        reloadScript();
        break;

      case CM_EFFECT_CLASS_NAME:
      {
        Tab<ParamScriptFactory *> list(tmpmem);

        ParamScriptsPool *pool = ::get_effect_scripts_pool();
        if (pool)
        {
          list.reserve(pool->factoryCount());
          for (int i = 0; i < pool->factoryCount(); ++i)
            if (pool->getFactory(i))
              list.push_back(pool->getFactory(i));
        }

        if (list.empty())
          break;

        Tab<String> strs(tmpmem);

        for (int i = 0; i < list.size(); ++i)
          strs.push_back() = list[i]->getClassName();

        ListDialog dlg(0, "Select effect's class name", strs, 400, 300);

        if (dlg.showDialog() == DIALOG_ID_OK)
        {
          const int sel = dlg.getSelectedIndex();

          if ((sel >= 0) && (sel < list.size()))
          {
            className = list[sel]->getClassName();

            setDirty();
            fillDeclPanel();

            effectClassNameHint = String(512, "Select effect's class name (current is \"%s\")", (const char *)className);
          }
        }

        break;
      }

      case CM_REVERT:
        if (wingw::message_box(wingw::MBS_YESNO, "Revert FX asset", "Revert asset data to original, removing all changes?") ==
            wingw::MB_ID_YES)
          revert();
        break;

      case PID_SHOW_DEBUG: particles_debug_render = panel->getBool(pid); break;

      case PID_SHOW_STATIC_VIS_SPHERE:
        particles_debug_static_vis_sphere = panel->getBool(pid);
        if (g_dafx_ctx)
          dafx::set_debug_flags(g_dafx_ctx,
            dafx::DEBUG_DISABLE_CULLING | (particles_debug_static_vis_sphere ? dafx::DEBUG_SHOW_BBOXES : 0));
        break;

      case PID_SHOW_VOLUME_DEBUG: particles_debug_volume = panel->getBool(pid); break;

      case PID_DEBUG_MOTION:
        particles_debug_motion = panel->getBool(pid);
        updateDebugMotionControllers(panel);
        break;

      case PID_DEBUG_MOTION_DIR: particles_debug_motion_dir = panel->getBool(pid); break;
    }
  }
  virtual void onChange(int pid, PropPanel2 *panel)
  {
    switch (pid)
    {
      case PID_DEBUG_MOTION_DISTANCE: particles_debug_motion_distance = panel->getFloat(pid); break;
      case PID_DEBUG_MOTION_SPEED: particles_debug_motion_speed = panel->getFloat(pid); break;
      case PID_DEBUG_TRAIL_WIDTH: particles_debug_trail_width = panel->getFloat(pid); break;
      case PID_DEBUG_TRAIL_VERTICAL: particles_debug_trail_vertical = panel->getFloat(pid); break;
      case PID_DEBUG_TRAIL_INDEX:
        particles_debug_trail_index = panel->getInt(pid);
        onScriptHelpersParamChange();
        break;
      case PID_DEBUG_EMITTER_ROT_X: particles_debug_rot_x = panel->getFloat(pid); break;
      case PID_DEBUG_EMITTER_ROT_Y: particles_debug_rot_y = panel->getFloat(pid); break;
      case PID_DEBUG_TIME_SPEED:
        particles_debug_time_speed = panel->getFloat(pid);
        g_dafx_dt_mul = particles_debug_time_speed;
        break;
      case PID_DEBUG_WIND_SPEED: particles_debug_wind_speed = panel->getFloat(pid); break;
      case PID_DEBUG_WIND_DIR: particles_debug_wind_dir = panel->getFloat(pid); break;
      case PID_DEBUG_QUALITY: particles_quality_preview = panel->getInt(pid); break;
    }

    if (pid == PID_DEBUG_QUALITY)
    {
      dafx::Config cfg = dafx::get_config(g_dafx_ctx);
      cfg.qualityMask = 1 << particles_quality_preview;
      dafx::set_config(g_dafx_ctx, cfg);
      onScriptHelpersParamChange();
    }
  }

  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->handleKeyPress(wnd, vk, modif);
    if (wnd && vk == 'S' && !modif)
      createFx();
  }

  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif)
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->handleKeyRelease(wnd, vk, modif);
  }

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseMove(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif)
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseDoubleClick(wnd, x, y, key_modif);

    return false;
  }


  void fillDeclPanel(bool save_and_restore_params = true)
  {
    DataBlock paramBlk;
    if (save_and_restore_params)
      ScriptHelpers::save_helpers(paramBlk, NULL);

    if (ScriptHelpers::obj_editor)
    {
      ScriptHelpers::obj_editor->removeAllObjects(false);
      ScriptHelpers::obj_editor->getUndoSystem()->begin();
    }

    IEffectClassTools *toolsIface = ::get_effect_class_tools_interface(className);
    ScriptHelpers::set_tuned_element(toolsIface ? toolsIface->createTunedElement() : NULL);

    if (ScriptHelpers::obj_editor)
    {
      ScriptHelpers::obj_editor->getUndoSystem()->accept("load");
      ScriptHelpers::obj_editor->getUndoSystem()->clear();
    }

    if (save_and_restore_params)
    {
      ScriptHelpers::load_helpers(paramBlk);
      ScriptHelpers::rebuild_tree_list();
    }
  }

  void reloadScript()
  {
    destroyFx();
    scriptClasses.clear();

    if (!scriptClasses.push_back().load(className))
    {
      wingw::message_box(wingw::MBS_EXCL, "Error Loading Script", "No effect class '%s' registered", (char *)className);
      return;
    }

    createFx();
  }

  bool isDirty() { return isModified; }
  void setDirty(bool d = true) { isModified = d; }

  void saveResource()
  {
    DataBlock fxBlk;

    // save data to blk
    fxBlk.setStr("className", className);

    EffectsPluginSaveCB saveCb;
    ScriptHelpers::save_helpers(*fxBlk.addBlock("params"), &saveCb);

    const DataBlock &main_blk = curAsset->props;
    int refNameId = main_blk.getNameId("ref");

    for (int bi = 0; bi < main_blk.blockCount(); ++bi)
    {
      const DataBlock &blk = *main_blk.getBlock(bi);
      if (blk.getBlockNameId() != refNameId)
        continue;
      saveCb.addRef(blk);
    }

    saveCb.saveRefSlots(fxBlk);
    curAsset->props.setFrom(&fxBlk);
    // fxBlk.saveToTextFile("test_fx.blk");
    setDirty(false);
    ::get_app().invalidateAssetIfChanged(*curAsset);
  }

  void revert()
  {
    if (curAsset)
    {
      DataBlock fxBlk(curAsset->getTargetFilePath());
      curAsset->props.setFrom(&fxBlk);

      ScriptHelpers::load_helpers(*curAsset->props.getBlockByNameEx("params"));
      reloadScript();

      setDirty(false);

      fillDeclPanel();
    }
  }

protected:
  struct ScriptClass
  {
    String className;
    ParamScriptFactory *factory;

    ScriptClass() : factory(NULL) {}
    ~ScriptClass() { clear(); }

    void clear() { factory = NULL; }
    bool load(const char *class_name)
    {
      clear();

      className = class_name;

      ParamScriptsPool *pool = ::get_effect_scripts_pool();
      if (!pool)
      {
        debug_ctx("Effect factories pool is NULL");
        clear();
        return false;
      }

      factory = pool->getFactoryByName(className);
      if (!factory)
      {
        debug_ctx("No effect class '%s' registered", (const char *)className);
        clear();
        return false;
      }

      return true;
    }
  };

  ObjectEditor effOE;

  DagorAsset *curAsset;
  String effectClassNameHint;
  String className;

  BaseEffectObject *effect;

  Tab<ScriptClass> scriptClasses;

  bool isModified;

  void *hwndPanel, *hwndTool, *hwndTree;
  PropertyContainerControlBase *mToolPanel;

  TMatrix4 globtmPrev = TMatrix4::IDENT;
  float effectTotalLifeTime = 0;

protected:
  virtual void onScriptHelpersParamChange()
  {
    setDirty();
    loadFxParams();
    if (g_dafx_ctx) // simulation rollback
    {
      effectTotalLifeTime = min(effectTotalLifeTime, 60.f);
      const float step = 1.f / 60.f;
      for (float t = 0; t < effectTotalLifeTime; t += step) //-V1034
      {
        float dt = min(step, effectTotalLifeTime - t) * particles_debug_time_speed;
        dafx::set_global_value(g_dafx_ctx, "dt", &dt, 4);
        dafx::start_update(g_dafx_ctx, dt);
        dafx::finish_update(g_dafx_ctx);
        if (effect)
          effect->update(dt);
      }
      effectTotalLifeTime = 0;
    }
  }

  void fillToolBar(PropertyContainerControlBase &toolbar)
  {
    toolbar.createButton(CM_RELOAD_SCRIPT, "Reload script and its declaration");
    toolbar.setButtonPictures(CM_RELOAD_SCRIPT, "compile");
    toolbar.createSeparator();

    toolbar.createButton(CM_EFFECT_CLASS_NAME, "Select effect's class name");
    toolbar.setButtonPictures(CM_EFFECT_CLASS_NAME, "res_effects");
    toolbar.createSeparator();

    toolbar.createButton(CM_REVERT, "Revert changes");
    toolbar.setButtonPictures(CM_REVERT, "import_blk");
  }

  void destroyFx() { del_it(effect); }

  void createFx()
  {
    destroyFx();

    if (!scriptClasses.size())
      return;
    if (!scriptClasses[0].factory)
      return;

    if (!scriptClasses[0].factory->isSubOf(HUID_BaseEffectObject))
      return;

    BaseParamScript *obj = scriptClasses[0].factory->createObject();
    if (!obj || !obj->isSubOf(HUID_BaseEffectObject))
      return;

    effect = (BaseEffectObject *)obj;

    DAEDITOR3.setFatalHandler();
    DAEDITOR3.resetFatalStatus();

    effectTotalLifeTime = 0;
    loadFxParams();

    if (DAEDITOR3.getFatalStatus())
    {
      destroyFx();
      DAEDITOR3.conShow(true);
    }
    DAEDITOR3.popFatalHandler();
  }

  void loadFxParams()
  {
    if (!effect)
      return;

    Tab<IDagorAssetRefProvider::Ref> refList(tmpmem);
    IDagorAssetRefProvider *refResv = curAsset->getMgr().getAssetRefProvider(curAsset->getType());
    if (refResv)
      refList = refResv->getAssetRefs(*curAsset);

    mkbindump::BinDumpSaveCB cb(64 << 10, _MAKE4C('PC'), false);
    EffectsPluginSaveLoadParamsDataCB saveLoadCb;

    ScriptHelpers::save_params_data(cb, &saveLoadCb);
    saveLoadCb.createRefs(*curAsset, refList);

    void *data = cb.makeDataCopy();
    effect->loadParamsData((const char *)data, cb.getSize(), &saveLoadCb);
    memfree_anywhere(data);

    if (effect->isSubOf(HUID_BaseTrailEffect))
      particles_debug_trail_id = ((BaseTrailEffect *)effect)->getNewTrailId(Color4(1, 1, 1), particles_debug_trail_index);
    else
      particles_debug_trail_id = -1;

    set_up_dafx_effect(g_dafx_ctx, effect, true);
  }

  void setupLight()
  {
    if (!effect || dynamic_lights_countVarId < 0)
      return;

    Color4 *lightParams = static_cast<Color4 *>(effect->getParam(HUID_LIGHT_PARAMS, NULL));
    Point3 *lightPos = static_cast<Point3 *>(effect->getParam(HUID_LIGHT_POS, NULL));

    bool hasLight = lightParams && lightPos;

    ShaderGlobal::set_int(dynamic_lights_countVarId, int(hasLight));
    if (hasLight)
    {
      Point4 posAndRad(lightPos->x, lightPos->y, lightPos->z, lightParams->a);
      Point4 colAndSpec(lightParams->r, lightParams->g, lightParams->b, 1.f);
      Point4 center(lightPos->x, lightPos->y, lightPos->z, 0.0f);
      Point4 extent(lightParams->a, lightParams->a, lightParams->a, 0.0f);

      ShaderGlobal::set_color4(simple_point_light_pos_radiusVarId, posAndRad);
      ShaderGlobal::set_color4(simple_point_light_color_specVarId, colAndSpec);
      ShaderGlobal::set_color4(simple_point_light_box_centerVarId, center);
      ShaderGlobal::set_color4(simple_point_light_box_extentVarId, extent);
    }
  }
};
DAG_DECLARE_RELOCATABLE(EffectsPlugin::ScriptClass);


static InitOnDemand<EffectsPlugin> plugin;

void init_plugin_effects()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();
  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
