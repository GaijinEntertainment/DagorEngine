// required externals:
//   rendinst::riex_handle_t extract_ri_extra_from_eid(ecs::EntityId eid)
//   bool is_free_camera_enabled()
//   void enable_free_camera()
//   void disable_free_camera()
//   void force_free_camera()
//   const TMatrix &get_cam_itm()

#include "inGameEditor.h"
#include "entityEditor.h"
#include "entityObj.h"
#include <daECS/scene/scene.h>
#include <gamePhys/collision/collisionLib.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <daInput/input_api.h>
#include "util/dag_delayedAction.h"
#include "util/dag_base64.h"
#include <ecs/scripts/sqEntity.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <json/json.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstAccess.h>

static IDaEditor4StubEC daEd4Stub;
static eastl::unique_ptr<IDaEditor4EmbeddedComponent> daEd4Embedded;
static IDaEditor4EmbeddedComponent *daEd4 = &daEd4Stub;
static eastl::unique_ptr<EntityObjEditor> objEd;
static String objEd_sceneFilePath;
static EntityObjEditor::SaveOrderRules objEd_saveOrderRules;
enum EGroupDef
{
  GROUPDEF_REQUIRE,
  GROUPDEF_VARIANT,
  GROUPDEF_TSUFFIX
};
static eastl::vector<eastl::pair<eastl::string, eastl::pair<EGroupDef, eastl::string>>> objEd_groupsRules;
static eastl::string objEd_startWorkMode;
struct VromRemover
{
  void operator()(VirtualRomFsData *vr)
  {
    vr ? remove_vromfs(vr) : (char *)NULL;
    delete vr;
  }
};
static eastl::unique_ptr<VirtualRomFsData, VromRemover> daEd4Vrom;
static bool wasFreeCameraActive = false;
static enum class EditorState { DISABLED, ACTIVE, RELOADING } daed4_state = EditorState::DISABLED;
static eastl::string objEd_pointAction_op;
static eastl::string objEd_pointAction_opWas;
static int objEd_pointAction_mod = 0;
static bool objEd_pointAction_has_pos = false;
static Point3 objEd_pointAction_pos(0, 0, 0);
static eastl::string objEd_pointAction_ext_id;
static eastl::string objEd_pointAction_ext_name;
static TMatrix objEd_pointAction_ext_mtx;
static Point4 objEd_pointAction_ext_sph(0, 0, 0, 0);
static ecs::EntityId objEd_pointAction_ext_eid = ecs::INVALID_ENTITY_ID;

static bool trace_ray_static_common(const Point3 &p0, const Point3 &dir, float &dist, Point3 *out_norm,
  rendinst::RendInstDesc *ri_desc, int excludeFlags = 0, int includeFlags = 0)
{
  float dir_l2 = dir.lengthSq();
  if (dir_l2 < VERY_SMALL_NUMBER)
    return false;

  const bool rendInstMode = objEd && objEd->isPickingRendinst();
  const int baseTraceFlags = rendInstMode ? (dacoll::ETF_FRT | dacoll::ETF_LMESH) : dacoll::ETF_DEFAULT;
  const int traceFlags = (baseTraceFlags & ~excludeFlags) | includeFlags;

  if (fabsf(dir_l2 - 1.0f) < VERY_SMALL_NUMBER)
    return dacoll::traceray_normalized(p0, dir, dist, NULL, out_norm, traceFlags, ri_desc);

  dir_l2 = sqrtf(dir_l2);
  dist *= dir_l2;
  bool ret = dacoll::traceray_normalized(p0, dir / dir_l2, dist, NULL, out_norm, traceFlags, ri_desc);
  dist /= dir_l2;
  return ret;
}

static bool trace_ray_static(const Point3 &p0, const Point3 &dir, float &dist, Point3 *out_norm)
{
  return trace_ray_static_common(p0, dir, dist, out_norm, nullptr);
}

static bool trace_ray_static_ex(const Point3 &p0, const Point3 &dir, float &dist, EditableObject *exclude_obj, Point3 *out_norm)
{
  if (!exclude_obj)
    return trace_ray_static_common(p0, dir, dist, out_norm, nullptr);

  EntityObj *o = RTTI_cast<EntityObj>(exclude_obj);
  if (!o)
    return trace_ray_static_common(p0, dir, dist, out_norm, nullptr);

  const rendinst::riex_handle_t riex_handle = extract_ri_extra_from_eid(o->getEid());
  if (riex_handle == rendinst::RIEX_HANDLE_NULL)
    return trace_ray_static_common(p0, dir, dist, out_norm, nullptr);

  Point3 p = p0;
  float dist_sum = 0.0f;
  float dist_left = dist;
  const float was_dist = dist;
  const float SKIP_DIST = 0.01f;

  // Here we trace until first non-RIExtra, or not same RIExtra as exclude_obj's, or something non-RI skipped
  // during same call of trace_ray_static_common() in while(), because exclude_obj's RIExtra was first.
  //
  // When touched RIExtra of exclude_obj we step forward by SKIP_DIST past intersection and try again.
  // We may hit it again and should keep skipping its collision shapes SKIP_DIST at a time.
  //
  // To limit number of tries we introduce MAX_SKIPS constant and skipsLeft variable:
  //
  const int MAX_SKIPS = 32;
  int skipsLeft = MAX_SKIPS;

  rendinst::RendInstDesc ri_desc;
  while (trace_ray_static_common(p, dir, dist, out_norm, &ri_desc))
  {
    if (skipsLeft <= 0 || !ri_desc.isRiExtra() || riex_handle != ri_desc.getRiExtraHandle() ||
        trace_ray_static_common(p, dir, dist, out_norm, nullptr, dacoll::ETF_RI))
    {
      dist += dist_sum;
      return true;
    }

    dist += SKIP_DIST;

    p += dir * dist;
    dist_sum += dist;
    dist_left -= dist;

    dist = dist_left;
    --skipsLeft;
  }

  dist = was_dist;
  return trace_ray_static_common(p0, dir, dist, out_norm, nullptr, dacoll::ETF_RI);
}

static bool ray_sphere_intersect(const Point3 &p0, const Point3 &dir, const Point3 &center, float radius, float &out_dist_near,
  float &out_dist_far)
{
  Point3 dir_norm = dir;
  dir_norm.normalize();
  const Point3 dcenter = p0 - center;
  const float b = 2.0f * dot(dcenter, dir);
  const float c = dot(dcenter, dcenter) - radius * radius;
  const float d = b * b - 4 * c;

  if (d < 0.0f)
    return false;

  const float sq = sqrt(d);
  const float v0 = (-b - sq) * 0.5f;
  const float v1 = (-b + sq) * 0.5f;

  if (v0 >= 0.0f)
  {
    if (v1 >= 0.0f)
    {
      out_dist_near = min(v0, v1);
      out_dist_far = max(v0, v1);
      return true;
    }
    out_dist_near = v1;
    out_dist_far = v0;
    return true;
  }
  else if (v1 >= 0.0f)
  {
    out_dist_near = v0;
    out_dist_far = v1;
    return true;
  }

  return false;
}

static float distance_to_line_sq(const Point3 &pt, const Point3 &p0, const Point3 &dir)
{
  Point3 dir_norm = dir;
  dir_norm.normalize();
  return (p0 + dir_norm * dot(pt - p0, dir_norm) - pt).lengthSq();
}

static void perform_point_action_static(bool trace, const Point3 &p0, const Point3 &dir, const Point3 &traced_pos, const char *op,
  int mod)
{
  G_UNUSED(p0);
  G_UNUSED(dir);

  objEd_pointAction_op = op;
  objEd_pointAction_mod = mod;
  objEd_pointAction_has_pos = trace;
  objEd_pointAction_pos = trace ? traced_pos : Point3(0, 0, 0);

  objEd_pointAction_ext_id.clear();
  objEd_pointAction_ext_name.clear();
  objEd_pointAction_ext_mtx.identity();
  objEd_pointAction_ext_sph = Point4(0, 0, 0, 0);
  objEd_pointAction_ext_eid = ecs::INVALID_ENTITY_ID;

  if (trace)
  {
    Point3 intr_pos = traced_pos;

    float dist = 1000.0f;
    float max_dist = dist;

    rendinst::RendInstDesc ri_desc;
    if (trace_ray_static_common(p0, dir, dist, nullptr, &ri_desc, 0, dacoll::ETF_RI_TREES))
    {
      if (ri_desc.isRiExtra())
      {
        const rendinst::riex_handle_t riex_handle = ri_desc.getRiExtraHandle();
        const uint32_t pool_idx = rendinst::handle_to_ri_type(riex_handle);
        const vec4f bsph = rendinst::getRIGenExtraBSphere(riex_handle);
        Point3_vec4 center;
        v_st(&center.x, bsph);
        const float radius = abs(v_extract_w(bsph));

        float intr_dist_near, intr_dist_far;
        if (ray_sphere_intersect(p0, dir, center, radius, intr_dist_near, intr_dist_far) && intr_dist_near > 0.0f)
        {
          Base64 b64;
          b64.encode((const uint8_t *)&riex_handle, sizeof(riex_handle));
          objEd_pointAction_ext_id = b64.c_str();
          objEd_pointAction_ext_name = rendinst::getRIGenExtraName(pool_idx);
          objEd_pointAction_ext_sph = Point4(center[0], center[1], center[2], radius);
          objEd_pointAction_ext_mtx = rendinst::getRIGenMatrix(rendinst::RendInstDesc(riex_handle));
          objEd_pointAction_ext_eid = find_ri_extra_eid(riex_handle);
        }
      }

      intr_pos = p0 + dir * dist;
      max_dist = dist;
    }

    for (int pass = 0; pass < 2; ++pass)
    {
      if (!objEd_pointAction_ext_id.empty())
        break;

      float dist_near = 0.0f;
      float dist_far = 1000.0f;

      float best_dist_sq = 0.0f;

      if (pass == 1)
        intr_pos = p0;

      bbox3f box;
      const float off_dist = (pass == 0) ? 10.0f : 30.0f;
      const Point3 off(off_dist, off_dist, off_dist);
      const Point3 pt1 = intr_pos - off;
      const Point3 pt2 = intr_pos + off;
      box.bmin = v_ldu(&pt1.x);
      box.bmax = v_ldu(&pt2.x);

      uint32_t poolsCount = rendinst::getRiGenExtraResCount();
      for (uint32_t pool_idx = 0; pool_idx < poolsCount; ++pool_idx)
      {
        Tab<rendinst::riex_handle_t> handles(framemem_ptr());
        rendinst::getRiGenExtraInstances(handles, pool_idx, box);
        const int handlesCount = handles.size();
        for (int i = 0; i < handlesCount; ++i)
        {
          const rendinst::riex_handle_t riex_handle = handles[i];
          const vec4f bsph = rendinst::getRIGenExtraBSphere(riex_handle);
          Point3_vec4 center;
          v_st(&center.x, bsph);
          const float radius = abs(v_extract_w(bsph));

          float intr_dist_near, intr_dist_far;
          if (ray_sphere_intersect(p0, dir, center, radius, intr_dist_near, intr_dist_far) && intr_dist_near > 0.0f &&
              intr_dist_near < max_dist)
          {
            const float dist_sq = (pass == 0) ? (center - intr_pos).lengthSq() : distance_to_line_sq(center, p0, dir);
            if ((pass == 0 && dist_sq < radius * radius && (objEd_pointAction_ext_id.empty() || dist_sq < best_dist_sq)) ||
                (pass == 1 && (objEd_pointAction_ext_id.empty() || dist_sq < best_dist_sq)))
            {
              dist_near = intr_dist_near;
              dist_far = intr_dist_far;

              best_dist_sq = dist_sq;

              Base64 b64;
              b64.encode((const uint8_t *)&riex_handle, sizeof(riex_handle));
              objEd_pointAction_ext_id = b64.c_str();
              objEd_pointAction_ext_name = rendinst::getRIGenExtraName(pool_idx);
              objEd_pointAction_ext_sph = Point4(center[0], center[1], center[2], radius);
              objEd_pointAction_ext_mtx = rendinst::getRIGenMatrix(rendinst::RendInstDesc(riex_handle));
              objEd_pointAction_ext_eid = find_ri_extra_eid(riex_handle);
            }
          }
        }
      }
    }
  }

  sqeventbus::send_event("entity_editor.onEditorChanged");
}

void init_entity_object_editor()
{
  if (objEd)
    return;

  debug("create objEd on first DaEditor4 activate");
  objEd.reset(new EntityObjEditor);
  daEd4->setObjectEditor(objEd.get());
  objEd->setupNewScene(objEd_sceneFilePath);

  objEd->setSaveOrderRules(objEd_saveOrderRules);

  objEd->clearTemplatesGroups();
  for (const auto &rule : objEd_groupsRules)
  {
    switch (rule.second.first)
    {
      case GROUPDEF_REQUIRE: objEd->addTemplatesGroupRequire(rule.first.c_str(), rule.second.second.c_str()); break;
      case GROUPDEF_VARIANT: objEd->addTemplatesGroupVariant(rule.first.c_str(), rule.second.second.c_str()); break;
      case GROUPDEF_TSUFFIX: objEd->addTemplatesGroupTSuffix(rule.first.c_str(), rule.second.second.c_str()); break;
    }
  }

  objEd->setWorkMode(objEd_startWorkMode.c_str());
}

static bool on_daed4_activate(bool activate)
{
  if (activate)
    init_entity_object_editor();

  sqeventbus::send_event("entity_editor.onEditorActivated", Json::Value(activate));

  if (activate || wasFreeCameraActive)
  {
    if (activate)
      wasFreeCameraActive = is_free_camera_enabled();
    else // De4ActivationHandler::gkehButtonUp reset action set, restore camera action later
      delayed_call([]() { dainput::activate_action_set(dainput::get_action_set_handle("Camera")); });
    enable_free_camera();
  }
  else
    disable_free_camera();

  if (activate)
  {
    if (objEd)
      objEd->refreshEids();
  }
  else
  {
    if (objEd->getEditMode() == CM_OBJED_MODE_CREATE_ENTITY || objEd->getEditMode() == CM_OBJED_MODE_POINT_ACTION)
      objEd->setEditMode(CM_OBJED_MODE_SELECT);
  }

  daed4_state = activate ? EditorState::ACTIVE : EditorState::DISABLED;
  return true;
}

static void on_daed4_update(float dt)
{
  G_UNUSED(dt);
  force_free_camera();
}

static void on_daed4_changed() { sqeventbus::send_event("entity_editor.onEditorChanged"); }

void init_da_editor4()
{
  daEd4Embedded.reset(create_da_editor4(NULL));
  daEd4 = daEd4Embedded ? daEd4Embedded.get() : &daEd4Stub;
  daEd4->setActivationHandler(on_daed4_activate);
  daEd4->setUpdateHandler(on_daed4_update);
  daEd4->setChangedHandler(on_daed4_changed);
  daEd4->setStaticTraceRay(&trace_ray_static);
  daEd4->setStaticTraceRayEx(&trace_ray_static_ex);
  daEd4->setStaticPerformPointAction(&perform_point_action_static);

  ecs::g_scenes.demandInit();
}


void term_da_editor4()
{
  daEd4->setObjectEditor(NULL);
  objEd.reset();
  daEd4Embedded.reset();
  daEd4 = &daEd4Stub;
  daEd4Vrom.reset();
}


EntityObjEditor *get_entity_obj_editor() { return objEd.get(); }

bool has_in_game_editor() { return daEd4 != &daEd4Stub; }

static SQInteger sq_get_saved_components(HSQUIRRELVM vm)
{
  SQInteger eid_int;
  sq_getinteger(vm, 2, &eid_int);
  ecs::EntityId eid((ecs::entity_id_t)eid_int);

  eastl::vector<eastl::string> comps;
  const bool isSceneEntity = EntityObjEditor::getSavedComponents(eid, comps);
  if (!isSceneEntity)
  {
    sq_pushnull(vm);
    return 1;
  }

  int idx = 0;
  Sqrat::Array res(vm, comps.size());
  for (const auto &comp : comps)
    res.SetValue(SQInteger(idx++), comp.c_str());

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}
static eastl::string sq_get_template_name_for_ui(SQInteger eid_int)
{
  ecs::EntityId eid((ecs::entity_id_t)eid_int);
  return EntityObjEditor::getTemplateNameForUI(eid);
}
static void sq_reset_component(SQInteger eid_int, const char *comp_name)
{
  ecs::EntityId eid((ecs::entity_id_t)eid_int);
  EntityObjEditor::resetComponent(eid, comp_name);
}
static void sq_save_component(SQInteger eid_int, const char *comp_name)
{
  ecs::EntityId eid((ecs::entity_id_t)eid_int);
  EntityObjEditor::saveComponent(eid, comp_name);
}
static void sq_save_add_template(SQInteger eid_int, const char *templ_name)
{
  ecs::EntityId eid((ecs::entity_id_t)eid_int);
  EntityObjEditor::saveAddTemplate(eid, templ_name);
}
static void sq_save_del_template(SQInteger eid_int, const char *templ_name)
{
  ecs::EntityId eid((ecs::entity_id_t)eid_int);
  EntityObjEditor::saveDelTemplate(eid, templ_name);
}

bool is_editor_activated() { return daed4_state == EditorState::ACTIVE; }
bool is_editor_in_reload() { return daed4_state == EditorState::RELOADING; }
void start_editor_reload() { daed4_state = daed4_state == EditorState::DISABLED ? EditorState::DISABLED : EditorState::RELOADING; }
void finish_editor_reload() { daed4_state = daed4_state == EditorState::DISABLED ? EditorState::DISABLED : EditorState::ACTIVE; }
bool is_editor_free_camera_enabled() { return daed4_state == EditorState::ACTIVE && daEd4->isFreeCameraActive(); }
static bool is_any_entity_selected() { return objEd && objEd->selectedCount() > 0; }
static bool is_asset_wnd_shown() { return objEd && objEd->getEditMode() == CM_OBJED_MODE_CREATE_ENTITY; }
static const char *get_scene_filepath() { return objEd ? objEd->getSceneFilePath() : objEd_sceneFilePath.c_str(); }

static void clear_entity_save_order() { objEd_saveOrderRules.clear(); }
static void add_entity_save_order_comp(const char *comp_name_start) { objEd_saveOrderRules.emplace_back(comp_name_start); }

static void clear_groups() { objEd_groupsRules.clear(); }
static void add_group_require(const char *group_name, const char *require)
{
  objEd_groupsRules.push_back({group_name, {GROUPDEF_REQUIRE, require}});
}
static void add_group_variant(const char *group_name, const char *variant)
{
  objEd_groupsRules.push_back({group_name, {GROUPDEF_VARIANT, variant}});
}
static void add_group_variext(const char *group_name, const char *variant, const char *tsuffix)
{
  objEd_groupsRules.push_back({group_name, {GROUPDEF_VARIANT, variant}});
  objEd_groupsRules.push_back({group_name, {GROUPDEF_TSUFFIX, tsuffix}});
}

void daEd4_get_all_ecs_tags(eastl::vector<eastl::string> &out_tags);

static SQInteger get_ecs_tags(HSQUIRRELVM vm)
{
  eastl::vector<eastl::string> tags;
  daEd4_get_all_ecs_tags(tags);

  int idx = 0;
  Sqrat::Array res(vm, tags.size());
  for (const auto &tag : tags)
    res.SetValue(SQInteger(idx++), tag.c_str());

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

static void set_start_work_mode(const char *mode) { objEd_startWorkMode = mode; }

static const char *get_point_action_op()
{
  objEd_pointAction_opWas = objEd_pointAction_op;
  objEd_pointAction_op.clear();
  return objEd_pointAction_opWas.c_str();
}
static int get_point_action_mod() { return objEd_pointAction_mod; }
static bool get_point_action_has_pos() { return objEd_pointAction_has_pos; }
static Point3 get_point_action_pos() { return objEd_pointAction_pos; }

static const char *get_point_action_ext_id() { return objEd_pointAction_ext_id.c_str(); }
static const char *get_point_action_ext_name() { return objEd_pointAction_ext_name.c_str(); }
static TMatrix get_point_action_ext_mtx() { return objEd_pointAction_ext_mtx; }
static Point4 get_point_action_ext_sph() { return objEd_pointAction_ext_sph; }
static int get_point_action_ext_eid() { return (int)(ecs::entity_id_t)objEd_pointAction_ext_eid; }

static SQInteger gather_ri_by_sphere(HSQUIRRELVM vm)
{
  SQFloat x, y, z, r;
  sq_getfloat(vm, 2, &x);
  sq_getfloat(vm, 3, &y);
  sq_getfloat(vm, 4, &z);
  sq_getfloat(vm, 5, &r);

  bbox3f box;
  const Point3 pos(x, y, z);
  const Point3 off(r, r, r);
  const Point3 pt1 = pos - off;
  const Point3 pt2 = pos + off;
  box.bmin = v_ldu(&pt1.x);
  box.bmax = v_ldu(&pt2.x);

  uint32_t poolsCount = rendinst::getRiGenExtraResCount();
  const float testRadiusSq = r * r;
  int fillCount = 0;

  for (uint32_t pool_idx = 0; pool_idx < poolsCount; ++pool_idx)
  {
    Tab<rendinst::riex_handle_t> handles(framemem_ptr());
    rendinst::getRiGenExtraInstances(handles, pool_idx, box);
    const int handlesCount = handles.size();
    for (int i = 0; i < handlesCount; ++i)
    {
      const rendinst::riex_handle_t riex_handle = handles[i];
      const vec4f bsph = rendinst::getRIGenExtraBSphere(riex_handle);
      Point3_vec4 center;
      v_st(&center.x, bsph);

      if ((center - pos).lengthSq() <= testRadiusSq)
      {
        ++fillCount;
      }
    }
  }

  Sqrat::Array res(vm, fillCount);

  int filledCount = 0;
  for (uint32_t pool_idx = 0; pool_idx < poolsCount; ++pool_idx)
  {
    Tab<rendinst::riex_handle_t> handles(framemem_ptr());
    rendinst::getRiGenExtraInstances(handles, pool_idx, box);
    const int handlesCount = handles.size();
    for (int i = 0; i < handlesCount; ++i)
    {
      const rendinst::riex_handle_t riex_handle = handles[i];
      const vec4f bsph = rendinst::getRIGenExtraBSphere(riex_handle);
      Point3_vec4 center;
      v_st(&center.x, bsph);
      const float radius = abs(v_extract_w(bsph));

      if ((center - pos).lengthSq() <= testRadiusSq && filledCount < fillCount)
      {
        Sqrat::Table riData(vm);
        Base64 b64;
        b64.encode((const uint8_t *)&riex_handle, sizeof(riex_handle));
        riData.SetValue("id", b64.c_str());
        riData.SetValue("name", rendinst::getRIGenExtraName(pool_idx));
        riData.SetValue("sph", Point4(center[0], center[1], center[2], radius));
        riData.SetValue("mtx", rendinst::getRIGenMatrix(rendinst::RendInstDesc(riex_handle)));
        riData.SetValue("eid", (int)(ecs::entity_id_t)find_ri_extra_eid(riex_handle));
        res.SetValue(filledCount, riData);

        ++filledCount;
      }
    }
  }

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

static SQInteger get_ri_from_entity(HSQUIRRELVM vm)
{
  SQInteger eid_int;
  sq_getinteger(vm, 2, &eid_int);
  ecs::EntityId eid((ecs::entity_id_t)eid_int);

  const rendinst::riex_handle_t riex_handle = extract_ri_extra_from_eid(eid);
  if (riex_handle == rendinst::RIEX_HANDLE_NULL)
  {
    sq_pushnull(vm);
    return 1;
  }

  const vec4f bsph = rendinst::getRIGenExtraBSphere(riex_handle);
  Point3_vec4 center;
  v_st(&center.x, bsph);
  const float radius = abs(v_extract_w(bsph));

  const uint32_t pool_idx = rendinst::handle_to_ri_type(riex_handle);

  Sqrat::Table res(vm);
  Base64 b64;
  b64.encode((const uint8_t *)&riex_handle, sizeof(riex_handle));
  res.SetValue("id", b64.c_str());
  res.SetValue("name", rendinst::getRIGenExtraName(pool_idx));
  res.SetValue("sph", Point4(center[0], center[1], center[2], radius));
  res.SetValue("mtx", rendinst::getRIGenMatrix(rendinst::RendInstDesc(riex_handle)));
  res.SetValue("eid", (int)(ecs::entity_id_t)eid);

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

static TMatrix make_cam_spawn_tm()
{
  TMatrix tm = get_cam_itm();
  const Point3 p0 = Point3(tm[3][0], tm[3][1], tm[3][2]);
  const Point3 dir = Point3(tm[2][0], tm[2][1], tm[2][2]);

  float dist = 50.0f;
  if (!trace_ray_static_common(p0, dir, dist, nullptr, nullptr))
    return TMatrix::IDENT;

  Point3 orient = dir;
  if (orient.x == 0.0f && orient.z == 0.0f)
  {
    orient.x = 1.0f;
    orient.z = 0.0f;
  }
  orient.y = 0.0f;
  orient.normalize();

  tm = TMatrix::IDENT;
  tm[0][0] = orient.x;
  tm[0][1] = 0.0f;
  tm[0][2] = orient.z;
  tm[2][0] = -orient.z;
  tm[2][1] = 0.0f;
  tm[2][2] = orient.x;
  tm[3][0] = p0.x + dir.x * dist;
  tm[3][1] = p0.y + dir.y * dist;
  tm[3][2] = p0.z + dir.z * dist;
  return tm;
}

/// @module entity_editor

void register_editor_script(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  ::register_da_editor4_script(module_mgr);
  EntityObjEditor::register_script_class(vm);

  Sqrat::Table exports(vm);
  exports //
    .SetValue("DE4_MODE_CREATE_ENTITY", CM_OBJED_MODE_CREATE_ENTITY)
    .Func("get_instance", get_entity_obj_editor)
    .SquirrelFunc("get_saved_components", sq_get_saved_components, 2, "ti")
    .Func("get_template_name_for_ui", sq_get_template_name_for_ui)
    .Func("reset_component", sq_reset_component)
    .Func("save_component", sq_save_component)
    .Func("save_add_template", sq_save_add_template)
    .Func("save_del_template", sq_save_del_template)
    .Func("is_editor_activated", is_editor_activated)
    .Func("is_any_entity_selected", is_any_entity_selected)
    .Func("is_asset_wnd_shown", is_asset_wnd_shown)
    .Func("get_scene_filepath", get_scene_filepath)
    .Func("clear_entity_save_order", clear_entity_save_order)
    .Func("add_entity_save_order_comp", add_entity_save_order_comp)
    .Func("clear_groups", clear_groups)
    .Func("add_group_require", add_group_require)
    .Func("add_group_variant", add_group_variant)
    .Func("add_group_variext", add_group_variext)
    .SquirrelFunc("get_ecs_tags", get_ecs_tags, 1, ".")
    .Func("set_start_work_mode", set_start_work_mode)

    .Func("get_point_action_op", get_point_action_op)
    .Func("get_point_action_mod", get_point_action_mod)
    .Func("get_point_action_has_pos", get_point_action_has_pos)
    .Func("get_point_action_pos", get_point_action_pos)
    .Func("get_point_action_ext_id", get_point_action_ext_id)
    .Func("get_point_action_ext_name", get_point_action_ext_name)
    .Func("get_point_action_ext_mtx", get_point_action_ext_mtx)
    .Func("get_point_action_ext_sph", get_point_action_ext_sph)
    .Func("get_point_action_ext_eid", get_point_action_ext_eid)

    .SquirrelFunc("gather_ri_by_sphere", gather_ri_by_sphere, 5, "tffff")
    .SquirrelFunc("get_ri_from_entity", get_ri_from_entity, 2, "ti")

    .Func("make_cam_spawn_tm", make_cam_spawn_tm)
    /**/;
  module_mgr->addNativeModule("entity_editor", exports);
}


IDaEditor4EmbeddedComponent &get_da_editor4() { return *daEd4; }
void da_editor4_setup_scene(const char *fpath)
{
  objEd_sceneFilePath = fpath;
  if (objEd)
    objEd->setupNewScene(fpath);
}
