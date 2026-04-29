// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_tree.h"
#include "../av_appwnd.h"
#include <assets/assetRefs.h>
#include <assetsGui/av_selObjDlg.h>
#include <assetsGui/av_assetTreeDragHandler.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <scriptHelpers/scriptHelpers.h>
#include <scriptHelpers/scriptPanel.h>

#include <fx/dag_paramScriptsPool.h>
#include <fx/dag_baseFxClasses.h>
#include <fx/dag_commonFx.h>
#include <fx/commonFxTools.h>
#include <gameRes/dag_stdGameRes.h>

// #include <EditorCore/ec_interface.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_wndPublic.h>
#include <de3_interface.h>

#include <fx/effectClassTools.h>
#include <shaders/dag_shaderMesh.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <propPanel/commonWindow/listDialog.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/dragAndDropHandler.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_string.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_direct.h> // dd_file_exist

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <math/dag_bezier.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_TMatrix4.h>
#include <gui/dag_stdGuiRenderEx.h>

#include <dafxToolsHelper/dafxToolsHelper.h>
#include <render/dag_cur_view.h>
#include <render/wind/ambientWind.h>
#include <render/wind/fxWindHelper.h>
#include <render/fx/fx.h>

#include "../av_cm.h"
#include "fxSaveLoad.h"
#include "../../commonFx/commonFxGame/dafxSystemDesc.h"
#include <daFx/dafx_gravity_zone.hlsli>
namespace dafx_ex
{
#include <daFx/dafx_def.hlsli>
#include "../../commonFx/commonFxGame/dafx_globals.hlsli"
} // namespace dafx_ex

using hdpi::_pxActual;
using hdpi::_pxScaled;
using PropPanel::DragAndDropResult;

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
  PID_DEBUG_DEFAULT_TRANSFORM = PID_DEBUG_WIND_SPEED + 1,
  PID_DEBUG_QUALITY = PID_DEBUG_DEFAULT_TRANSFORM + 1,
  PID_DEBUG_RESOLUTION = PID_DEBUG_QUALITY + 1,
  PID_DEBUG_CAMERA_VELOCITY = PID_DEBUG_RESOLUTION + 1,
  PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW1 = PID_DEBUG_CAMERA_VELOCITY + 1,
  PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW2 = PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW1 + 1,
  PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW3 = PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW2 + 1,
  PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW4 = PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW3 + 1,
  PID_DEBUG_HDR_EMISSION = PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW4 + 1,
  PID_DEBUG_HDR_EMISSION_THRESHOLD = PID_DEBUG_HDR_EMISSION + 1,
  PID_DEBUG_MOTION_DIR_AXIS = PID_DEBUG_HDR_EMISSION_THRESHOLD + 1,
  PID_DEBUG_PARENT_VELOCITY = PID_DEBUG_MOTION_DIR_AXIS + 1,
  PID_DEBUG_GRAV_ROTATION = PID_DEBUG_PARENT_VELOCITY + 1,
};

static bool particles_debug_render;

bool particles_debug_static_vis_sphere = false;

bool particles_debug_volume = false;

bool particles_debug_motion = false;
bool particles_debug_motion_dir = false;
int particles_debug_motion_dir_axis = 0;
float particles_debug_motion_distance = 1;
float particles_debug_motion_speed = 1;
float particles_debug_trail_width = 1;
float particles_debug_trail_vertical = 0;
int particles_debug_trail_index = 0;
float particles_debug_rot_x = 0;
float particles_debug_rot_y = 0;
float particles_debug_grav_rot = 0;
float particles_debug_time_speed = 1;
float particles_debug_wind_speed = 0;
float particles_debug_wind_dir = 0;
Point2 particles_debug_camera_velocity2D = Point2(0.0f, 4.5f);
TMatrix4 particles_debug_spline_placement = TMatrix4::ZERO;
float particles_debug_parent_velocity = 0;
bool particles_debug_hdr_emission = false;
float particles_debug_hdr_emission_threshold = 1;
TMatrix particles_debug_tm = TMatrix::IDENT;

int particles_debug_trail_id = -1;
float particles_debug_motion_time = 0;
int partices_default_transform = 0;
int particles_quality_preview = 2;

static int dynamic_lights_countVarId = -1;
static int simple_point_light_pos_radiusVarId = -1;
static int simple_point_light_color_specVarId = -1;
static int simple_point_light_box_centerVarId = -1;
static int simple_point_light_box_extentVarId = -1;

static int dafx_gravity_zone_bufferVarId = -1;
static UniqueBuf dafx_gravity_zone_buffer_gpu;
static GravityZoneDescriptor dafx_gravity_zone_container = {
  TMatrix::IDENT, float3(1, 1, 1), GRAVITY_ZONE_SHAPE_BOX, GRAVITY_ZONE_TYPE_LINEAR, 1.0f};
extern GravityZoneDescriptor_buffer dafx_gravity_zone_buffer;

template <typename ValueType>
static void apply_value(int pid, PropPanel::ContainerPropertyControl &panel, ValueType value);

template <>
void apply_value(int pid, PropPanel::ContainerPropertyControl &panel, float value)
{
  panel.setFloat(pid, value);
}

template <>
void apply_value(int pid, PropPanel::ContainerPropertyControl &panel, int value)
{
  panel.setInt(pid, value);
}

template <>
void apply_value(int pid, PropPanel::ContainerPropertyControl &panel, bool value)
{
  panel.setBool(pid, value);
}

template <>
void apply_value(int pid, PropPanel::ContainerPropertyControl &panel, Point4 value)
{
  panel.setPoint4(pid, value);
}

template <typename ValueType>
static ValueType get_param(int pid, PropPanel::ContainerPropertyControl *panel);

template <>
float get_param(int pid, PropPanel::ContainerPropertyControl *panel)
{
  return panel->getFloat(pid);
}

template <>
int get_param(int pid, PropPanel::ContainerPropertyControl *panel)
{
  return panel->getInt(pid);
}

template <>
bool get_param(int pid, PropPanel::ContainerPropertyControl *panel)
{
  return panel->getBool(pid);
}

template <>
Point4 get_param(int pid, PropPanel::ContainerPropertyControl *panel)
{
  return panel->getPoint4(pid);
}

extern const DataBlock *get_project_settings_blk();

uint32_t get_gpu_buffer_size_based_on_quality(uint32_t quality_mask)
{
  constexpr uint32_t DEFAULT_BUFFER_SIZE = 524288;
  if (quality_mask & (1 << 0)) // low
    return get_project_settings_blk()->getBlockByNameEx("graphics")->getInt("fxLowQualityGpuDataBufferSize", DEFAULT_BUFFER_SIZE);
  else if (quality_mask & (1 << 1)) // medium
    return get_project_settings_blk()->getBlockByNameEx("graphics")->getInt("fxMediumQualityGpuDataBufferSize", DEFAULT_BUFFER_SIZE);
  else if (quality_mask & (1 << 2)) // high
    return get_project_settings_blk()->getBlockByNameEx("graphics")->getInt("fxHighQualityGpuDataBufferSize", DEFAULT_BUFFER_SIZE);

  return DEFAULT_BUFFER_SIZE;
}

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

  void addRefSlot(const char *name, const char *type_name, bool make_unique) override
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
                      public PropPanel::ControlEventHandler,
                      public IWndManagerWindowHandler
{
public:
  EffectsPlugin() : isModified(false), scriptClasses(midmem), effect(NULL), mToolPanel(NULL), curAsset(NULL)
  {
    dynamic_lights_countVarId = ::get_shader_variable_id("dynamic_lights_count", true);
    simple_point_light_pos_radiusVarId = ::get_shader_variable_id("simple_point_light_pos_radius", true);
    simple_point_light_color_specVarId = ::get_shader_variable_id("simple_point_light_color_spec", true);
    simple_point_light_box_centerVarId = ::get_shader_variable_id("simple_point_light_box_center", true);
    simple_point_light_box_extentVarId = ::get_shader_variable_id("simple_point_light_box_extent", true);
    fxwindhelper::load_fx_wind_curve_params(::dgs_get_game_params());
    if (!get_app().dngBasedSceneRenderUsed())
    {
      dafx_gravity_zone_bufferVarId = ::get_shader_variable_id("dafx_gravity_zone_buffer", true);
      if (dafx_gravity_zone_bufferVarId != -1)
        dafx_gravity_zone_buffer_gpu =
          dag::buffers::create_persistent_sr_structured(sizeof(GravityZoneDescriptor), 1, "dafx_gravity_zone_buffer");
    }
    dropHandler = new EffectsPropPanelDropHandler(*this);
  }
  ~EffectsPlugin() override
  {
    destroyFx();
    ScriptHelpers::set_param_change_cb(NULL);
    dafx_gravity_zone_buffer_gpu.close();
    del_it(dropHandler);
  }

  const char *getInternalName() const override { return "Effects"; }

  void registered() override {}
  void unregistered() override {}

  bool havePropPanel() override { return true; }

  bool begin(DagorAsset *asset) override
  {
    IWndManager &manager = getWndManager();
    manager.registerWindowHandler(this);

    manager.setWindowType(nullptr, EFFECTS_TOOL_WINDOW_TYPE);

    curAsset = asset;

    ScriptHelpers::obj_editor = &effOE;
    ScriptHelpers::create_helper_bar(&manager);

    className = asset->props.getStr("className", NULL);

    fillDeclPanel(/*save_and_restore_params = */ false);

    ScriptHelpers::load_helpers(*asset->props.getBlockByNameEx("params"));
    saveResource();
    ScriptHelpers::set_param_change_cb(this);

    reloadScript();

    setDirty(false);

    // effOE.initUi();
    fillDeclPanel();
    loadPanelState();

    return true;
  }
  bool end() override
  {
    savePanelState();
    IWndManager &manager = getWndManager();

    ScriptHelpers::removeWindows();
    manager.removeWindow(mToolPanel);

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

  void *onWmCreateWindow(int type) override
  {
    switch (type)
    {
      case EFFECTS_TOOL_WINDOW_TYPE:
      {
        if (mToolPanel)
          return nullptr;

        mToolPanel = IEditorCoreEngine::get()->createPropPanel(this, "Effects Tools");
        fillToolBar(*mToolPanel->createToolbarPanel(21, ""));

        return mToolPanel;
      }
    }

    return ScriptHelpers::on_wm_create_window(type);
  }

  bool onWmDestroyWindow(void *window) override
  {
    if (mToolPanel && mToolPanel == window)
    {
      del_it(mToolPanel);
      return true;
    }

    return false;
  }

  void clearObjects() override {}

  void onSaveLibrary() override
  {
    if (curAsset)
      saveResource();
  }

  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
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

  void actObjects(real dt) override
  {
    dt *= particles_debug_time_speed;

    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->update(dt);

    if (effect)
    {
      particles_debug_tm = rotyTM(-particles_debug_rot_x * 3.14);
      particles_debug_tm *= rotzTM(-particles_debug_rot_y * 3.14);

      if (!effectDafxSetupDone && dafx_helper_globals::ctx)
      {
        set_up_dafx_effect(dafx_helper_globals::ctx, effect, true, true, &particles_debug_tm, partices_default_transform == 0);
        effectDafxSetupDone = true;
      }

      ShaderGlobal::set_int(::get_shader_variable_id("modfx_debug_render", true), particles_debug_render);
      ShaderGlobal::set_int(::get_shader_variable_id("fx_debug_editor_mode", true), (int)particles_debug_hdr_emission);
      ShaderGlobal::set_float(::get_shader_variable_id("fx_debug_exposure_threshold", true), particles_debug_hdr_emission_threshold);

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
          Point3 nextDir = Point3(sin(particles_debug_motion_time + 1), 0, cos(particles_debug_motion_time + 1));
          Point3 nextPos = particles_debug_motion_distance * nextDir;
          Point3 fwdDir = normalize(nextPos - pos);
          Point3 leftDir = normalize(cross(fwdDir, float3(0, 1, 0)));

          int fwdCol = 0;
          int leftCol = 0;
          int upCol = 0;
          if (particles_debug_motion_dir_axis == 0) // x
          {
            fwdCol = 0;
            leftCol = 2;
            upCol = 1;
          }
          else if (particles_debug_motion_dir_axis == 1) // y
          {
            fwdCol = 1;
            leftCol = 0;
            upCol = 2;
          }
          else // z
          {
            fwdCol = 2;
            leftCol = 0;
            upCol = 1;
          }

          TMatrix dirTm;
          dirTm.setcol(fwdCol, fwdDir);
          dirTm.setcol(leftCol, leftDir);
          dirTm.setcol(upCol, normalize(cross(leftDir, fwdDir)));
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
        if (particles_debug_parent_velocity > FLT_EPSILON)
          pz = particles_debug_tm.getcol(0) * particles_debug_parent_velocity;

        effect->setParam(HUID_VELOCITY, &pz);
      }

      if (dafx_helper_globals::ctx)
      {
        Point2 windDir = Point2(cos(particles_debug_wind_dir), -sin(particles_debug_wind_dir));
        set_dafx_wind_params(dafx_helper_globals::ctx, windDir,
          AmbientWind::beaufort_to_meter_per_second(fxwindhelper::apply_wind_strength_curve(particles_debug_wind_speed)),
          effectTotalLifeTime);

        set_dafx_camera_velocity(dafx_helper_globals::ctx,
          Point3(particles_debug_camera_velocity2D.x, 0, particles_debug_camera_velocity2D.y));
        setGravZone();
      }

      Point4 p4 = Point4::xyz0(particles_debug_tm.getcol(3));
      effect->setParam(_MAKE4C('PFXP'), &p4);

      effect->setParam(partices_default_transform == 0 ? HUID_EMITTER_TM : HUID_TM, &particles_debug_tm);
      BezierSplineInt<Point4> spline;
      TMatrix4 splineCoefTm = TMatrix4::ZERO;
      spline.calculate(particles_debug_spline_placement.row);
      for (int i = 0; i < 4; ++i)
        splineCoefTm.row[i] = spline.sk[i];
      effect->setParam(_MAKE4C('SPLN'), &splineCoefTm);

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
    if (!dafx_helper_globals::ctx)
      return;

    bool started = StdGuiRender::is_render_started();
    if (!started)
      StdGuiRender::continue_render();

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(E3DCOLOR_MAKE(255, 255, 255, 255));

    const dafx::Stats &s = dafx_helper_globals::stats;
    String insts(128, "instances: %d", (int)(s.activeInstances / 2)); // instances are 2 part objects
    String cpuSim(128, "cpuSimulation: %d", s.cpuElemProcessed);
    String gpuSim(128, "gpuSimulation: %d", s.gpuElemProcessed);
    String dips(128, "dips: %d", s.renderDrawCalls);
    String tris(128, "tris: %d/%d", s.visibleTriangles, s.renderedTriangles);

    int fxStats[4] = {0};
    effect.getParam(_MAKE4C('PFXX'), fxStats);
    String params(128, "param_ren: %d, param_sim: %d", fxStats[0], fxStats[1]);
    String data(128, "part_ren: %d, part_sim: %d", fxStats[2], fxStats[3]);
    String distance(128, "distance to camera: %.2f", length(::grs_cur_view.pos));

    int x = hdpi::_pxS(8);
    int y = hdpi::_pxS(40);
    int n = hdpi::_pxS(20);
    StdGuiRender::draw_strf_to(x, y + n * 0, insts);
    StdGuiRender::draw_strf_to(x, y + n * 1, cpuSim);
    StdGuiRender::draw_strf_to(x, y + n * 2, gpuSim);
    StdGuiRender::draw_strf_to(x, y + n * 3, dips);
    StdGuiRender::draw_strf_to(x, y + n * 4, tris);
    StdGuiRender::draw_strf_to(x, y + n * 5, data);
    StdGuiRender::draw_strf_to(x, y + n * 6, params);
    StdGuiRender::draw_strf_to(x, y + n * 7, distance);

    StdGuiRender::flush_data();
    if (!started)
      StdGuiRender::end_render();
  }
  void beforeRenderObjects() override
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->beforeRender();
  }
  void renderObjects() override
  {
    if (ScriptHelpers::obj_editor)
    {
      begin_draw_cached_debug_lines();
      ScriptHelpers::obj_editor->render();
      end_draw_cached_debug_lines();
    }
  }
  void renderTransObjects() override
  {
    if (ScriptHelpers::obj_editor)
      ScriptHelpers::obj_editor->renderTrans();

    if (!dafx_helper_globals::ctx)
      return;

    dafx::render_debug_opt(dafx_helper_globals::ctx);
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
  void renderGeometry(Stage stage) override
  {
    if (!effect || !getVisible())
      return;

    switch (stage)
    {
      case STG_BEFORE_RENDER: effect->render(FX_RENDER_BEFORE, ::grs_cur_view.itm); break;

      case STG_RENDER_DYNAMIC_OPAQUE: effect->render(FX_RENDER_SOLID, ::grs_cur_view.itm); break;

      case STG_RENDER_FX: effect->render(FX_RENDER_TRANS, ::grs_cur_view.itm); break;

      case STG_RENDER_FX_DISTORTION:
        effect->render(FX_RENDER_DISTORTION, ::grs_cur_view.itm);
        if (dafx_helper_globals::ctx)
          dafx::render(dafx_helper_globals::ctx, dafx_helper_globals::cull_id, "distortion", 0.f);
        break;

      default: break;
    }
  }

  bool supportAssetType(const DagorAsset &asset) const override
  {
    return asset.getFileNameId() >= 0 && strcmp(asset.getTypeStr(), "fx") == 0;
  }

  void updateDebugMotionControllers(PropPanel::ContainerPropertyControl *panel)
  {
    panel->setEnabledById(PID_DEBUG_MOTION_DISTANCE, particles_debug_motion);
    panel->setEnabledById(PID_DEBUG_MOTION_SPEED, particles_debug_motion);
    panel->setEnabledById(PID_DEBUG_MOTION_DIR, particles_debug_motion);

    bool trail = effect && effect->isSubOf(HUID_BaseTrailEffect);
    panel->setEnabledById(PID_DEBUG_TRAIL_WIDTH, particles_debug_motion && trail);
    panel->setEnabledById(PID_DEBUG_TRAIL_VERTICAL, particles_debug_motion && trail);
    panel->setEnabledById(PID_DEBUG_TRAIL_INDEX, particles_debug_motion && trail);
  }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override
  {
    panel.setEventHandler(this);
    panel.clear();
    panel.createCheckBox(PID_SHOW_DEBUG, "Debug particles render", particles_debug_render);
    panel.createCheckBox(PID_SHOW_STATIC_VIS_SPHERE, "Debug static visibility", particles_debug_static_vis_sphere);
    panel.createCheckBox(PID_SHOW_VOLUME_DEBUG, "Debug volume", particles_debug_volume);
    panel.createCheckBox(PID_DEBUG_HDR_EMISSION, "Debug hdr emission", particles_debug_hdr_emission);
    panel.createTrackFloat(PID_DEBUG_HDR_EMISSION_THRESHOLD, "    debug hdr emission threshold",
      particles_debug_hdr_emission_threshold, 0.0, 10.0, 0.01);

    panel.createCheckBox(PID_DEBUG_MOTION, "Debug emitter motion", particles_debug_motion);
    panel.createCheckBox(PID_DEBUG_MOTION_DIR, "Debug emitter direction", particles_debug_motion_dir);

    Tab<String> axisNames(tmpmem);
    axisNames.push_back(String("x"));
    axisNames.push_back(String("y"));
    axisNames.push_back(String("z"));
    panel.createCombo(PID_DEBUG_MOTION_DIR_AXIS, "Debug emitter direction axis", axisNames, particles_debug_motion_dir_axis);
    panel.createEditFloat(PID_DEBUG_MOTION_DISTANCE, "   motion distance", particles_debug_motion_distance);
    panel.createEditFloat(PID_DEBUG_MOTION_SPEED, "   motion speed", particles_debug_motion_speed);
    panel.createEditFloat(PID_DEBUG_TRAIL_WIDTH, "    trail width", particles_debug_trail_width);
    panel.createTrackFloat(PID_DEBUG_TRAIL_VERTICAL, "    trail vertical", particles_debug_trail_vertical, 0, 1, 0.1);
    panel.createEditInt(PID_DEBUG_TRAIL_INDEX, "    trail index", particles_debug_trail_index);
    panel.createTrackFloat(PID_DEBUG_EMITTER_ROT_X, "    debug rotation y", particles_debug_rot_x, 0, 1, 0.01);
    panel.createTrackFloat(PID_DEBUG_EMITTER_ROT_Y, "    debug rotation z", particles_debug_rot_y, 0, 1, 0.01);
    panel.createTrackFloat(PID_DEBUG_TIME_SPEED, "    debug time speed", particles_debug_time_speed, 0, 5, 0.01);
    panel.createTrackFloat(PID_DEBUG_WIND_SPEED, "    debug wind speed(Beaufort)", particles_debug_wind_speed, 0, 8, 0.01);
    panel.createTrackFloat(PID_DEBUG_WIND_DIR, "    debug wind angle", particles_debug_wind_dir, 0, 6.28, 0.01);
    panel.createTrackFloat(PID_DEBUG_CAMERA_VELOCITY, "  debug camera velocity", particles_debug_camera_velocity2D.y, 0, 10, 0.01);
    panel.createTrackFloat(PID_DEBUG_PARENT_VELOCITY, "  debug parent velocity", particles_debug_parent_velocity, 0, 10, 0.01);
    panel.createTrackFloat(PID_DEBUG_GRAV_ROTATION, "    debug grav_zone rotation z", particles_debug_grav_rot, 0, 1, 0.01);
    PropPanel::ContainerPropertyControl *splineDebugGrp = panel.createGroup(ID_SPEC_GRP, "Debug spline placement");
    if (splineDebugGrp)
    {
      splineDebugGrp->createPoint4(PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW1, " first point",
        particles_debug_spline_placement.getrow(0), 3, true);
      splineDebugGrp->createPoint4(PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW2, " second point",
        particles_debug_spline_placement.getrow(1), 3, true);
      splineDebugGrp->createPoint4(PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW3, " third point",
        particles_debug_spline_placement.getrow(2), 3, true);
      splineDebugGrp->createPoint4(PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW4, " fourth point",
        particles_debug_spline_placement.getrow(3), 3, true);
    }

    Tab<String> transformNames(tmpmem);
    transformNames.push_back(String("world"));
    transformNames.push_back(String("local"));
    panel.createCombo(PID_DEBUG_DEFAULT_TRANSFORM, "Default transform type:", transformNames, partices_default_transform);

    Tab<String> qualityNames(tmpmem);
    qualityNames.push_back(String("low"));
    qualityNames.push_back(String("medium"));
    qualityNames.push_back(String("high"));
    panel.createCombo(PID_DEBUG_QUALITY, "Quality preview:", qualityNames, particles_quality_preview);

    Tab<String> resolutionNames(tmpmem);
    resolutionNames.push_back(String("lowres (force)"));
    resolutionNames.push_back(String("default"));
    resolutionNames.push_back(String("highres (force)"));
    panel.createCombo(PID_DEBUG_RESOLUTION, "Resolution preview:", resolutionNames, dafx_helper_globals::particles_resolution_preview);

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
    panel.setDropTargetHandler(dropHandler);
  }

  void postFillPropPanel() override {}

  template <typename ValueType>
  class ParamChangeUndoRedo : public UndoRedoObject
  {
  public:
    explicit ParamChangeUndoRedo(PropPanel::ContainerPropertyControl &panel, int pid, EffectsPlugin *plugin, ValueType undo,
      ValueType redo) :
      panel(panel), pid(pid), plugin(plugin), redoValue(redo), undoValue(undo)
    {}

    void restore(bool save_redo_data) override { applyChange(undoValue); }
    void redo() override { applyChange(redoValue); }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "ParamChangeUndoRedo"; }

  protected:
    virtual void applyChange(ValueType value)
    {
      apply_value<ValueType>(pid, panel, value);
      plugin->applyUndoOperation(true);
      plugin->onChange(pid, &panel);
      plugin->applyUndoOperation(false);
    }
    PropPanel::ContainerPropertyControl &panel;
    EffectsPlugin *plugin;
    ValueType undoValue;
    ValueType redoValue;
    int pid;
  };

  template <typename ValueType>
  class ParamClickUndoRedo : public ParamChangeUndoRedo<ValueType>
  {
    using ParamChangeUndoRedo<ValueType>::panel;
    using ParamChangeUndoRedo<ValueType>::plugin;
    using ParamChangeUndoRedo<ValueType>::pid;

  public:
    explicit ParamClickUndoRedo(PropPanel::ContainerPropertyControl &panel, int pid, EffectsPlugin *plugin, ValueType undo,
      ValueType redo) :
      ParamChangeUndoRedo<ValueType>(panel, pid, plugin, undo, redo)
    {}

  protected:
    void applyChange(ValueType value) override
    {
      apply_value<ValueType>(pid, panel, value);
      plugin->applyUndoOperation(true);
      plugin->onClick(pid, &panel);
      plugin->applyUndoOperation(false);
    }
  };

  template <typename ValueType>
  ValueType getParamUndoableChange(int pid, PropPanel::ContainerPropertyControl *panel, ValueType old_value)
  {
    ValueType newValue = get_param<ValueType>(pid, panel);
    if (!applyingUndoOperation)
    {
      get_app().getUndoSystem()->begin();
      get_app().getUndoSystem()->put(new ParamChangeUndoRedo<ValueType>(*panel, pid, this, old_value, newValue));
      get_app().getUndoSystem()->accept("changeParam");
    }
    return newValue;
  }

  template <typename ValueType>
  ValueType getParamUndoableClick(int pid, PropPanel::ContainerPropertyControl *panel, ValueType old_value)
  {
    ValueType newValue = get_param<ValueType>(pid, panel);
    if (!applyingUndoOperation)
    {
      get_app().getUndoSystem()->begin();
      get_app().getUndoSystem()->put(new ParamClickUndoRedo<ValueType>(*panel, pid, this, old_value, newValue));
      get_app().getUndoSystem()->accept("clickParam");
    }
    return newValue;
  }

  void applyUndoOperation(bool apply) { applyingUndoOperation = apply; }

  // ControlEventHandler
  void onClick(int pid, PropPanel::ContainerPropertyControl *panel) override
  {
    int refIdx = -1;
    if (pid >= PID_CHANGE_REF && pid < PID_CHANGE_REF + 32)
      refIdx = pid - PID_CHANGE_REF;

    DataBlock *b = NULL;
    if (refIdx >= 0)
    {
      const char *type_name = nullptr;
      int type = -1;
      if (!getCurEffectAssetRef(refIdx, b, type_name, type))
        return;

      {
        SelectAssetDlgOptions dlgOptions;
        dlgOptions.selectableAssetTypes.push_back(type);
        dlgOptions.initiallySelectedAsset = b->getStr("ref", "");
        dlgOptions.dialogCaption.printf(64, "Select %s", type_name);
        dlgOptions.selectButtonCaption.printf(64, "Select %s", type_name);
        dlgOptions.resetButtonCaption.printf(64, "Reset %s", type_name);
        dlgOptions.positionBesideWindow = "Properties";

        const eastl::optional<String> result = DAEDITOR3.selectAsset(dlgOptions);
        if (!result.has_value())
          return;

        b->setStr("ref", result.value());
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

        PropPanel::ListDialog dlg("Select effect's class name", strs, _pxScaled(400), _pxScaled(300));

        if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
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

      case PID_SHOW_DEBUG: particles_debug_render = getParamUndoableClick(pid, panel, particles_debug_render); break;

      case PID_SHOW_STATIC_VIS_SPHERE:
        particles_debug_static_vis_sphere = getParamUndoableClick(pid, panel, particles_debug_static_vis_sphere);
        if (dafx_helper_globals::ctx)
          dafx::set_debug_flags(dafx_helper_globals::ctx,
            dafx::DEBUG_DISABLE_CULLING | (particles_debug_static_vis_sphere ? dafx::DEBUG_SHOW_BBOXES : 0));
        break;

      case PID_SHOW_VOLUME_DEBUG: particles_debug_volume = getParamUndoableClick(pid, panel, particles_debug_volume); break;

      case PID_DEBUG_MOTION:
        particles_debug_motion = getParamUndoableClick(pid, panel, particles_debug_motion);
        updateDebugMotionControllers(panel);
        break;

      case PID_DEBUG_MOTION_DIR: particles_debug_motion_dir = getParamUndoableClick(pid, panel, particles_debug_motion_dir); break;
    }
  }

  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override
  {
    switch (pid)
    {
      case PID_DEBUG_MOTION_DIR_AXIS:
        particles_debug_motion_dir_axis = getParamUndoableChange(pid, panel, particles_debug_motion_dir_axis);
        break;
      case PID_DEBUG_MOTION_DISTANCE:
        particles_debug_motion_distance = getParamUndoableChange(pid, panel, particles_debug_motion_distance);
        break;
      case PID_DEBUG_MOTION_SPEED:
        particles_debug_motion_speed = getParamUndoableChange(pid, panel, particles_debug_motion_speed);
        break;
      case PID_DEBUG_TRAIL_WIDTH: particles_debug_trail_width = getParamUndoableChange(pid, panel, particles_debug_trail_width); break;
      case PID_DEBUG_TRAIL_VERTICAL:
        particles_debug_trail_vertical = getParamUndoableChange(pid, panel, particles_debug_trail_vertical);
        break;
      case PID_DEBUG_TRAIL_INDEX:
        particles_debug_trail_index = getParamUndoableChange(pid, panel, particles_debug_trail_index);
        onScriptHelpersParamChange();
        break;
      case PID_DEBUG_EMITTER_ROT_X: particles_debug_rot_x = getParamUndoableChange(pid, panel, particles_debug_rot_x); break;
      case PID_DEBUG_EMITTER_ROT_Y: particles_debug_rot_y = getParamUndoableChange(pid, panel, particles_debug_rot_y); break;
      case PID_DEBUG_TIME_SPEED:
        particles_debug_time_speed = getParamUndoableChange(pid, panel, particles_debug_time_speed);
        dafx_helper_globals::dt_mul = particles_debug_time_speed;
        break;
      case PID_DEBUG_WIND_SPEED: particles_debug_wind_speed = getParamUndoableChange(pid, panel, particles_debug_wind_speed); break;
      case PID_DEBUG_WIND_DIR: particles_debug_wind_dir = getParamUndoableChange(pid, panel, particles_debug_wind_dir); break;
      case PID_DEBUG_DEFAULT_TRANSFORM:
        partices_default_transform = getParamUndoableChange(pid, panel, partices_default_transform);
        break;
      case PID_DEBUG_QUALITY: particles_quality_preview = getParamUndoableChange(pid, panel, particles_quality_preview); break;
      case PID_DEBUG_RESOLUTION:
        dafx_helper_globals::particles_resolution_preview =
          getParamUndoableChange(pid, panel, dafx_helper_globals::particles_resolution_preview);
        break;
      case PID_DEBUG_CAMERA_VELOCITY:
        particles_debug_camera_velocity2D.y = getParamUndoableChange(pid, panel, particles_debug_camera_velocity2D.y);
        break;
      case PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW1:
        particles_debug_spline_placement.setrow(0, getParamUndoableChange(pid, panel, particles_debug_spline_placement.getrow(0)));
        break;
      case PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW2:
        particles_debug_spline_placement.setrow(1, getParamUndoableChange(pid, panel, particles_debug_spline_placement.getrow(1)));
        break;
      case PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW3:
        particles_debug_spline_placement.setrow(2, getParamUndoableChange(pid, panel, particles_debug_spline_placement.getrow(2)));
        break;
      case PID_DEBUG_SPLINE_PLACEMENT_SPLINE_DATA_ROW4:
        particles_debug_spline_placement.setrow(3, getParamUndoableChange(pid, panel, particles_debug_spline_placement.getrow(3)));
        break;
      case PID_DEBUG_PARENT_VELOCITY:
        particles_debug_parent_velocity = getParamUndoableChange(pid, panel, particles_debug_parent_velocity);
        break;
      case PID_DEBUG_GRAV_ROTATION: particles_debug_grav_rot = getParamUndoableChange(pid, panel, particles_debug_grav_rot); break;
      case PID_DEBUG_HDR_EMISSION:
        particles_debug_hdr_emission = getParamUndoableChange(pid, panel, particles_debug_hdr_emission);
        break;
      case PID_DEBUG_HDR_EMISSION_THRESHOLD:
        particles_debug_hdr_emission_threshold = getParamUndoableChange(pid, panel, particles_debug_hdr_emission_threshold);
        break;
    }

    if (pid == PID_DEBUG_QUALITY && dafx_helper_globals::ctx)
    {
      dafx::Config cfg = dafx::get_config(dafx_helper_globals::ctx);
      cfg.qualityMask = 1 << particles_quality_preview;
      cfg.gpu_data_buffer_size = get_gpu_buffer_size_based_on_quality(cfg.qualityMask);
      dafx::set_config(dafx_helper_globals::ctx, cfg);
      onScriptHelpersParamChange();
    }
  }

  void registerEditorCommands(IEditorCommandSystem &command_system) override
  {
    effOE.registerEditorCommands(command_system);

    command_system.addCommand(EditorCommandIds::FX_EDITOR_RESET_EFFECTS, ImGuiKey_S);
  }

  void registerMenuAccelerators() override
  {
    IWndManager &wndManager = *EDITORCORE->getWndManager();
    wndManager.addViewportAccelerator(CM_FX_EDITOR_RESET_EFFECTS, EditorCommandIds::FX_EDITOR_RESET_EFFECTS);
  }

  void handleViewportAcceleratorCommand(IGenViewportWnd &wnd, unsigned id) override
  {
    if (id == CM_FX_EDITOR_RESET_EFFECTS)
      createFx();
  }

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseMove(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ScriptHelpers::obj_editor)
      return ScriptHelpers::obj_editor->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);

    return false;
  }

  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override
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

  void savePanelState()
  {
    String pluginStatesPath;
    DataBlock pluginStatesBlk;
    pluginStatesPath = assetlocalprops::makePath("_plugin_states.blk");
    pluginStatesBlk.load(pluginStatesPath);
    DataBlock *paramsState = pluginStatesBlk.addBlock("fxEditorParamsState");
    DataBlock *fxTypeState = paramsState->addBlock(className);
    fxTypeState->clearData();
    ScriptHelpers::save_params_state(*fxTypeState);
    DataBlock *windowState = pluginStatesBlk.addBlock("windowState");
    DataBlock *fxTypeWindowState = windowState->addBlock(className);
    fxTypeWindowState->setInt("vscroll", mToolPanel->getScrollPos());
    pluginStatesBlk.saveToTextFile(pluginStatesPath);
  }

  void loadPanelState()
  {
    String pluginStatesPath;
    DataBlock pluginStatesBlk;
    pluginStatesPath = assetlocalprops::makePath("_plugin_states.blk");
    if (!dd_file_exist(pluginStatesPath))
      pluginStatesBlk.saveToTextFile(pluginStatesPath);
    pluginStatesBlk.load(pluginStatesPath);
    DataBlock *paramsState = pluginStatesBlk.getBlockByName("fxEditorParamsState");
    if (paramsState && paramsState->blockExists(className))
    {
      DataBlock *fxTypeState = paramsState->getBlockByName(className);
      ScriptHelpers::load_params_state(*fxTypeState);
    }
    DataBlock *windowState = pluginStatesBlk.getBlockByName("windowState");
    if (windowState && windowState->blockExists(className))
    {
      DataBlock *fxTypeWindowState = windowState->getBlockByName(className);
      mToolPanel->setScrollPos(fxTypeWindowState->getInt("vscroll", 0));
    }
  }

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

  void updateImgui() override
  {
    if (!mToolPanel)
      return;

    DAEDITOR3.imguiBegin(*mToolPanel);
    mToolPanel->updateImgui();
    ScriptHelpers::updateImgui();
    DAEDITOR3.imguiEnd();
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
        DEBUG_CTX("Effect factories pool is NULL");
        clear();
        return false;
      }

      factory = pool->getFactoryByName(className);
      if (!factory)
      {
        DEBUG_CTX("No effect class '%s' registered", (const char *)className);
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
  bool effectDafxSetupDone = false;
  bool applyingUndoOperation = false;

  Tab<ScriptClass> scriptClasses;

  bool isModified;

  PropPanel::PanelWindowPropertyControl *mToolPanel;

  TMatrix4 globtmPrev = TMatrix4::IDENT;
  float effectTotalLifeTime = 0;

  class EffectsPropPanelDropHandler : public PropPanel::IDropTargetHandler
  {
  public:
    explicit EffectsPropPanelDropHandler(EffectsPlugin &_e) : effects(_e) {}

    DragAndDropResult handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
    {
      if (pcb_id < PID_CHANGE_REF || PID_RESET_REF <= pcb_id)
        return DragAndDropResult::NONE;

      const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery(ASSET_DRAG_AND_DROP_TYPE);
      if (!dragAndDropPayload)
        return DragAndDropResult::NONE;

      DataBlock *b = nullptr;
      const char *type_name = nullptr;
      int type = -1;
      if (!effects.getCurEffectAssetRef(pcb_id - PID_CHANGE_REF, b, type_name, type))
        return DragAndDropResult::NONE;

      AssetDragData dragData;
      PropPanel::getDragAndDropPayloadData<AssetDragData>(dragAndDropPayload, &dragData);

      // Only allow dropping into the same asset type.
      DagorAsset *asset = dragData.asset;
      if (asset != nullptr && asset->getType() == type)
      {
        if (dragAndDropPayload->IsDelivery())
        {
          b->setStr("ref", asset->getName());
          panel->setCaption(pcb_id, b->getStr("ref", ""));

          effects.reloadScript();

          return DragAndDropResult::ACCEPTED;
        }
        return DragAndDropResult::NONE;
      }

      return DragAndDropResult::NOT_ALLOWED;
    }

  private:
    EffectsPlugin &effects;
  };

  EffectsPropPanelDropHandler *dropHandler;

protected:
  void onScriptHelpersParamChange() override
  {
    setDirty();
    loadFxParams();
    if (dafx_helper_globals::ctx) // simulation rollback
    {
      effectTotalLifeTime = min(effectTotalLifeTime, 60.f);
      const float step = 1.f / 60.f;
      for (float t = 0; t < effectTotalLifeTime; t += step) //-V1034
      {
        float dt = min(step, effectTotalLifeTime - t) * particles_debug_time_speed;
        dafx::set_global_value(dafx_helper_globals::ctx, "dt", &dt, 4);
        dafx::start_update(dafx_helper_globals::ctx, dt);
        dafx::finish_update(dafx_helper_globals::ctx);
        if (effect)
          effect->update(dt);
      }
      effectTotalLifeTime = 0;
    }
  }

  void fillToolBar(PropPanel::ContainerPropertyControl &toolbar)
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

  bool getCurEffectAssetRef(int ref_idx, DataBlock *&b, const char *&type_name, int &type)
  {
    DataBlock &main_blk = curAsset->props;
    int refNameId = main_blk.getNameId("ref"), idx = 0;
    for (int bi = 0; bi < main_blk.blockCount(); ++bi)
    {
      b = main_blk.getBlock(bi);
      if (b->getBlockNameId() == refNameId)
      {
        if (ref_idx == idx)
          break;
        idx++;
      }
      b = nullptr;
    }

    if (b)
    {
      type_name = b->getStr("type", "");
      type = curAsset->getMgr().getAssetTypeId(type_name);
    }

    return b != nullptr && type >= 0;
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
    effectDafxSetupDone = false;

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
    if (IDagorAssetRefProvider *refResv = curAsset->getMgr().getAssetRefProvider(curAsset->getType()))
      refResv->getAssetRefs(*curAsset, refList);

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

    if (dafx_helper_globals::ctx)
    {
      set_up_dafx_effect(dafx_helper_globals::ctx, effect, true, true, &particles_debug_tm, partices_default_transform == 0);
      effectDafxSetupDone = true;
    }
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

      ShaderGlobal::set_float4(simple_point_light_pos_radiusVarId, posAndRad);
      ShaderGlobal::set_float4(simple_point_light_color_specVarId, colAndSpec);
      ShaderGlobal::set_float4(simple_point_light_box_centerVarId, center);
      ShaderGlobal::set_float4(simple_point_light_box_extentVarId, extent);
    }
  }

  void setGravZone()
  {
    if (effect)
    {
      Matrix3 gravityTm;
      gravityTm.rotzTM(-particles_debug_grav_rot * 3.14);
      effect->setParam(_MAKE4C('GZTM'), (void *)&gravityTm);
    }

    if (!dafx_helper_globals::ctx)
      return;

    TMatrix gravityTm;
    gravityTm.rotzTM(-particles_debug_grav_rot * 3.14);

    if (get_app().dngBasedSceneRenderUsed())
    {
      acesfx::GravityZoneBuffer buffer;
      acesfx::push_gravity_zone(buffer, gravityTm, Point3(1000, 1000, 1000), GRAVITY_ZONE_SHAPE_BOX, GRAVITY_ZONE_TYPE_LINEAR, 1.0f,
        0.0f, true);
      acesfx::set_gravity_zones(buffer);
    }
    else if (dafx_gravity_zone_bufferVarId != -1)
    {
      unsigned gravity_zone_count = 1;
      dafx::set_global_value(dafx_helper_globals::ctx, "gravity_zone_count", &gravity_zone_count, sizeof(gravity_zone_count));

      dafx_gravity_zone_container =
        GravityZoneDescriptor(gravityTm, float3(1000, 1000, 1000), GRAVITY_ZONE_SHAPE_BOX, GRAVITY_ZONE_TYPE_LINEAR, 1.0f);

      dafx_gravity_zone_buffer = &dafx_gravity_zone_container;

      dafx_gravity_zone_buffer_gpu->updateData(0, sizeof(dafx_gravity_zone_container), &dafx_gravity_zone_container, 0);
      ShaderGlobal::set_buffer(dafx_gravity_zone_bufferVarId, dafx_gravity_zone_buffer_gpu);
    }
  }
};
DAG_DECLARE_RELOCATABLE(EffectsPlugin::ScriptClass);


static InitOnDemand<EffectsPlugin> plugin;

void init_plugin_effects(const DataBlock &appblk)
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  register_all_common_fx_tools(appblk);
  ::plugin.demandInit();
  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
