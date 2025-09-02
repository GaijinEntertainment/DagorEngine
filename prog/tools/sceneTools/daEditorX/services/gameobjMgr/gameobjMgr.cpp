// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/optional.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_writeObjsToPlaceDump.h>
#include <de3_entityUserData.h>
#include <de3_animCtrl.h>
#include <landMesh/lmeshHoles.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <EditorCore/ec_interface.h>
#include <ecs/core/entityManager.h>
#include <anim/dag_animBlend.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <math/dag_capsule.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <array>

static int collisionSubtypeMask = -1;
static int gameobjEntityClassId = -1;
static int rendEntGeomMask = -1;
static int rendEntGeom = -1;
static int next_hier_id = 0, hier_linked_count = 0;
static bool registeredNotifier = false;
static OAHashNameMap<true> gameObjNames;

class GameObjEntity;
typedef SingleEntityPool<GameObjEntity> GameObjEntityPool;

class VirtualGameObjEntity : public IObjEntity, public IObjEntityUserDataHolder
{
public:
  VirtualGameObjEntity(int cls) : IObjEntity(cls), pool(NULL), nameId(-1), userDataBlk(NULL)
  {
    assetNameId = -1;
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    ladderStepsCount = -1;
    isGroundHole = false;
    holeShapeIntersection = false;
    isRiNavmeshBlocker = false;
    isNavmeshHoleCutter = false;
    bsph.setempty();
    bsph = bbox;
    volType = VT_BOX;
    needsSuperEntRef = false;
  }
  ~VirtualGameObjEntity()
  {
    clear();
    assetNameId = -1;
  }

  void clear()
  {
    del_it(userDataBlk);
    for (auto *e : visEntity)
      destroy_it(e);
    visEntity.clear();
  }

  void setTm(const TMatrix &_tm) override {}
  void getTm(TMatrix &_tm) const override { _tm = TMatrix::IDENT; }
  void destroy() override { delete this; }
  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    return NULL;
  }

  BSphere3 getBsph() const override { return bsph; }
  BBox3 getBbox() const override { return bbox; }

  const char *getObjAssetName() const override
  {
    static String buf;
    buf.printf(0, "%s:gameObj", gameObjNames.getName(nameId));
    return buf;
  }

  void setup(const DagorAsset &asset, bool virtual_ent)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();

    nameId = gameObjNames.addNameId(name);
    needsSuperEntRef = asset.props.getBool("needsSuperEntRef", false);
    const char *type = asset.props.getStr("volumeType", "box");
    volType = VT_BOX;
    if (strcmp(type, "box") == 0)
      volType = VT_BOX;
    else if (strcmp(type, "sphere") == 0)
      volType = VT_SPH;
    else if (strcmp(type, "point") == 0)
      volType = VT_PT;
    else if (strcmp(type, "capsule") == 0)
      volType = VT_CAPS;
    else if (strcmp(type, "cylinder") == 0)
      volType = VT_CYLI;
    else if (strcmp(type, "axes") == 0)
      volType = VT_AXES;
    else
      DAEDITOR3.conError("%s: unrecognized volumeType:t=\"%s\"", getObjAssetName(), type);

    float hsz = asset.props.getReal("boxSz", 1);
    bbox[0].set(-hsz, -hsz, -hsz);
    bbox[1].set(hsz, hsz, hsz);

    ladderStepsCount = asset.props.getInt("ladderStepsCount", -1);
    isGroundHole = asset.props.getBool("isGroundHole", false);
    holeShapeIntersection = asset.props.getBool("holeShapeIntersection", false);
    isRiNavmeshBlocker = asset.props.getBool("isRiNavmeshBlocker", false);
    isNavmeshHoleCutter = asset.props.getBool("isNavmeshHoleCutter", false);

    bsph = bbox;
    if (volType == VT_SPH)
      bsph.r = hsz, bsph.r2 = hsz * hsz;

    if (volType == VT_CAPS || volType == VT_CYLI)
    {
      float width = asset.props.getReal("shapeWidth", 0.5);
      float height = asset.props.getReal("shapeHeight", 1);

      float half_width = width * 0.5f;
      float half_height = height * 0.5f;

      bbox[0].set(-half_width, -half_width, -half_height);
      bbox[1].set(half_width, half_width, half_height);
    }

    if (const ecs::Template *templ = g_entity_mgr->getTemplateDB().getTemplateByName(asset.getName()))
    {
      IObjEntity *ent = nullptr;
      if (create_visual_entity(templ, asset, "animchar__res", "animChar", virtual_ent, ent))
        visEntity.push_back(ent);
      if (create_visual_entity(templ, asset, "effect__name", "fx", virtual_ent, ent))
        visEntity.push_back(ent);
      else if (create_visual_entity(templ, asset, "ecs_fx__res", "fx", virtual_ent, ent))
        visEntity.push_back(ent);
      if (create_visual_entity(templ, asset, "ri_extra__name", "rendInst", virtual_ent, ent))
        visEntity.push_back(ent);
      else if (create_visual_entity(templ, asset, "ri_preview__name", "rendInst", virtual_ent, ent))
        visEntity.push_back(ent);
      else if (create_visual_entity(templ, asset, "ri_gpu_object__name", "rendInst", virtual_ent, ent))
        visEntity.push_back(ent);
      isAlive = get_template_bool_comp(templ, "isAlive", false);

      // spot light
      light__max_radius = get_template_real_comp(templ, "light__max_radius", 0.0f);
      spot_light__cone_angle = get_template_real_comp(templ, "spot_light__cone_angle", NAN);
      spot_light__inner_attenuation = get_template_real_comp(templ, "spot_light__inner_attenuation", 1.0f);
      if (!isnan(spot_light__cone_angle))
      {
        volType = VT_SPOT_LIGHT;
        if (spot_light__cone_angle < 0.0f)
        {
          TMatrix tm;
          getTm(tm);
          float col_length = length(tm.getcol(2));
          spot_light__cone_angle = 2.0f * atanf(col_length);
        }
        else
        {
          spot_light__cone_angle *= PI / 180.0;
        }
      }

      // hierarchy support
      canBeP = asset.props.getBool("canBeParentForAttach", get_template_component_ptr(templ, "hierarchy_unresolved_id") != nullptr);
      canBeC = asset.props.getBool("canBeAttached", get_template_component_ptr(templ, "hierarchy_unresolved_parent_id") != nullptr);
    }
    if (visEntity.empty())
      if (const char *ref_res = asset.props.getStr("ref_dynmodel", nullptr))
        if (DagorAsset *a = *ref_res ? DAEDITOR3.getAssetByName(ref_res) : nullptr)
        {
          if (IObjEntity *ent = DAEDITOR3.createEntity(*a, virtual_ent))
            visEntity.push_back(ent);
          else
            DAEDITOR3.conError("%s: failed to create ref model=%s:%s", asset.getName(), ref_res, a->getTypeStr());
        }

    for (auto *e : visEntity)
      e->setSubtype(rendEntGeom);
    updateVisEntityIsAliveParam();
  }
  void updateVisEntityIsAliveParam()
  {
    for (auto *e : visEntity)
      if (auto *animCtrl = e->queryInterface<IAnimCharController>())
        if (auto *animState = animCtrl->getAnimState())
        {
          int id = animCtrl->getAnimGraph()->getParamId("isAlive", animState->PT_ScalarParam);
          if (id >= 0)
            animState->setParam(id, isAlive ? 1.0f : 0.0f);
        }
  }

  static inline const ecs::ChildComponent *get_template_component_ptr(const ecs::Template *templ, const char *comp_name)
  {
    auto name_hash = ECS_HASH_SLOW(comp_name);
    const ecs::ChildComponent *res = templ->getComponentsMap().find(name_hash);
    if (!res)
      g_entity_mgr->getTemplateDB().data().iterate_template_parents(*templ, [&](const ecs::Template &p_tmpl) {
        if (!res)
          res = p_tmpl.getComponentsMap().find(name_hash);
      });
    return res;
  }
  template <typename T>
  static inline T get_template_component(const ecs::Template *templ, const char *comp_name, T def_val)
  {
    auto name_hash = ECS_HASH_SLOW(comp_name);
    const ecs::ChildComponent *res = get_template_component_ptr(templ, comp_name);
    return res ? res->getOr<T>(def_val) : def_val;
  }

  static const char *get_template_string_comp(const ecs::Template *templ, const char *comp_name)
  {
    auto name_hash = ECS_HASH_SLOW(comp_name);
    const ecs::ChildComponent *res = templ->getComponentsMap().find(name_hash);
    if (!res)
      g_entity_mgr->getTemplateDB().data().iterate_template_parents(*templ, [&](const ecs::Template &p_tmpl) {
        if (!res)
          res = p_tmpl.getComponentsMap().find(name_hash);
      });
    if (!res)
      return nullptr;
    const char *val = res->getOr("");
    return *val ? val : nullptr;
  }

  static real get_template_real_comp(const ecs::Template *templ, const char *comp_name, real def_val)
  {
    return get_template_component<real>(templ, comp_name, def_val);
  }

  static bool get_template_bool_comp(const ecs::Template *templ, const char *comp_name, bool def_val)
  {
    return get_template_component<bool>(templ, comp_name, def_val);
  }

  static bool create_visual_entity(const ecs::Template *templ, const DagorAsset &asset, const char *comp_name, const char *asset_type,
    bool virtual_ent, IObjEntity *&out_ent)
  {
    if (const char *res = get_template_string_comp(templ, comp_name))
    {
      if (DagorAsset *a = DAEDITOR3.getAssetByName(res, DAEDITOR3.getAssetTypeId(asset_type)))
      {
        out_ent = DAEDITOR3.createEntity(*a, virtual_ent);
        return true;
      }
      DAEDITOR3.conError("%s: failed to create %s=%s:%s", asset.getName(), comp_name, res, asset_type);
    }
    return false;
  };

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }


  // IObjEntityUserDataHolder
  DataBlock *getUserDataBlock(bool create_if_not_exist) override
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  void resetUserDataBlock() override { del_it(userDataBlk); }
  void setSuperEntityRef(const char *ref) override
  {
    if (needsSuperEntRef)
      getUserDataBlock(true)->setStr("ref", ref);
  }
  bool canBeParentForAttach() override { return canBeP; }
  bool canBeAttached() override { return canBeC; }
  void attachTo(IObjEntity *e, const TMatrix &local_tm) override
  {
    if (e->getAssetTypeId() != getAssetTypeId())
      return;
    VirtualGameObjEntity *pe = static_cast<VirtualGameObjEntity *>(e);
    DataBlock *blkP = pe->getUserDataBlock(true);
    DataBlock *blkC = getUserDataBlock(true);
    int p_id = blkP->getInt("hierarchy_unresolved_id", -1);
    if (p_id < 0)
      blkP->setInt("hierarchy_unresolved_id", p_id = ++next_hier_id);
    blkC->setInt("hierarchy_unresolved_parent_id", p_id);
    blkC->setTm("hierarchy_transform", local_tm);
    hier_linked_count++;
  }

public:
  DataBlock *userDataBlk;
  GameObjEntityPool *pool;
  BBox3 bbox;
  int ladderStepsCount;
  bool isGroundHole;
  bool holeShapeIntersection;
  bool isRiNavmeshBlocker;
  bool isNavmeshHoleCutter;
  bool isAlive = true;
  float light__max_radius = -1.0f;
  float spot_light__cone_angle = -1.0f;
  float spot_light__inner_attenuation = -1.0f;
  BSphere3 bsph;
  enum
  {
    VT_BOX,
    VT_SPH,
    VT_PT,
    VT_CAPS,
    VT_CYLI,
    VT_SPOT_LIGHT,
    VT_AXES
  };
  int16_t volType;
  bool needsSuperEntRef;
  bool canBeP = false, canBeC = false;
  int nameId;
  int assetNameId;
  StaticTab<IObjEntity *, 2> visEntity;
};

class GameObjEntity : public VirtualGameObjEntity
{
public:
  GameObjEntity(int cls) : VirtualGameObjEntity(cls), idx(MAX_ENTITIES) { tm.identity(); }
  ~GameObjEntity() { clear(); }

  void clear() { VirtualGameObjEntity::clear(); }

  void setTm(const TMatrix &_tm) override
  {
    tm = _tm;
    for (auto *e : visEntity)
      e->setTm(_tm);
  }
  void getTm(TMatrix &_tm) const override { _tm = tm; }
  void destroy() override
  {
    pool->delEntity(this);
    clear();
  }

  void copyFrom(const GameObjEntity &e)
  {
    bbox = e.bbox;
    bsph = e.bsph;
    ladderStepsCount = e.ladderStepsCount;
    isGroundHole = e.isGroundHole;
    holeShapeIntersection = e.holeShapeIntersection;
    isRiNavmeshBlocker = e.isRiNavmeshBlocker;
    isNavmeshHoleCutter = e.isNavmeshHoleCutter;
    isAlive = e.isAlive;
    light__max_radius = e.light__max_radius;
    spot_light__cone_angle = e.spot_light__cone_angle;
    spot_light__inner_attenuation = e.spot_light__inner_attenuation;
    volType = e.volType;
    needsSuperEntRef = e.needsSuperEntRef;
    canBeP = e.canBeP;
    canBeC = e.canBeC;

    nameId = e.nameId;
    assetNameId = e.assetNameId;
    for (auto *ve : e.visEntity)
      visEntity.push_back(DAEDITOR3.cloneEntity(ve));
    for (auto *ve : visEntity)
      ve->setSubtype(rendEntGeom);
    updateVisEntityIsAliveParam();
  }

  static bool isFrontFace(const Point3 &a, const Point3 &b, const Point3 &c, const TMatrix &tm)
  {
    auto *vp = EDITORCORE->getCurrentViewport();
    Point3 pts[3];
    vp->worldToNDC(tm * a, pts[0]);
    vp->worldToNDC(tm * b, pts[1]);
    vp->worldToNDC(tm * c, pts[2]);
    float crossProd = (pts[1].x - pts[0].x) * (pts[2].y - pts[0].y) - (pts[1].y - pts[0].y) * (pts[2].x - pts[0].x);
    return crossProd < 0;
  }

  static Point2 worldToScreen(const Point3 &p, const TMatrix4 &modelproj)
  {
    Point4 ndc = Point4(p.x, p.y, p.z, 1.f) * modelproj;
    real invW = 1.f / ndc.w;
    return Point2(ndc.x * invW, ndc.y * invW);
  }

  static eastl::optional<eastl::pair<Point3, Point3>> find_cone_tangent_points(const Point3 &c, const Point3 &h, float R,
    const Point3 &p)
  {
    float h_len = h.length();
    if (h_len < FLT_EPSILON)
      return eastl::nullopt;

    Point3 n = h / h_len;
    Point3 up = (std::abs(n.z) > 0.9999f) ? Point3(0, 1, 0) : Point3(0, 0, 1);
    Point3 u = normalize(cross(up, n));
    Point3 v = cross(n, u);

    Point3 p_local = p - c;
    Point3 p_prime(p_local * u, p_local * v, p_local * n);

    float k = R / h_len;
    float A = p_prime.x;
    float B = p_prime.y;
    float C = k * k * p_prime.z * h_len;

    float L_sq = A * A + B * B;
    if (L_sq < FLT_EPSILON)
      return eastl::nullopt;

    float R_sq = R * R;
    float dist_sq = C * C / L_sq;
    if (dist_sq > R_sq)
      return eastl::nullopt;

    Point2 P_closest(A * C / L_sq, B * C / L_sq);
    float s = std::sqrt(R_sq - dist_sq);
    float L = std::sqrt(L_sq);
    Point2 line_dir(-B / L, A / L);
    Point2 t1_prime_2D = P_closest + line_dir * s;
    Point2 t2_prime_2D = P_closest - line_dir * s;
    Point3 t1 = c + u * t1_prime_2D.x + v * t1_prime_2D.y + n * h_len;
    Point3 t2 = c + u * t2_prime_2D.x + v * t2_prime_2D.y + n * h_len;
    return eastl::make_pair(t1, t2);
  }

  static void draw_spot_light_visualization(const TMatrix &tm, const Point3 &apex, const Point3 &dir, const float angle, float radius,
    const float innerAttenuation)
  {
    const int32_t SEGS = 32;
    TMatrix4 viewTm, projTm;
    d3d::gettm(TM_VIEW, &viewTm);
    d3d::gettm(TM_PROJ, &projTm);
    const TMatrix4 modelproj = (TMatrix4(tm) * viewTm) * projTm;
    auto invTm = orthonormalized_inverse(tmatrix(viewTm) * tm);
    const Point3 camera = invTm.getcol(3);
    const E3DCOLOR colorOuter(255, 255, 0);
    const E3DCOLOR colorInner(255, 140, 0);
    const E3DCOLOR colorDirection(255, 255, 255);

    Point3 forward, tangent, binormal;
    ortho_normalize(dir, Point3(0, 1, 0), forward, tangent, binormal);

    float alphaSin, alphaCos;
    sincos(angle * 0.5f, alphaSin, alphaCos);
    const float circleRadius = radius * alphaSin;
    const float coneHeight = radius * alphaCos;
    sincos(angle * 0.5f * innerAttenuation, alphaSin, alphaCos);
    const float innerRadius = radius * alphaSin;
    const float innerConeHeight = radius * alphaCos;

    Point3 circleCenter = apex + forward * coneHeight;
    Point3 innerCircleCenter = apex + forward * innerConeHeight;
    Point3 pt[SEGS];

    // direction line
    draw_cached_debug_line(apex, apex + forward * radius, colorDirection);

    // outer cone
    for (int32_t i = 0; i < SEGS; ++i)
    {
      float angle = i * TWOPI / SEGS;
      float s, c;
      sincos(angle, s, c);
      pt[i] = tangent * c + binormal * s;
    }
    for (int32_t i = 0; i < SEGS; ++i)
    {
      Point3 a = circleCenter + pt[i] * circleRadius;
      Point3 b = circleCenter + pt[(i + 1) % SEGS] * circleRadius;
      draw_cached_debug_line(a, b, colorOuter);
    }
    if (auto tangentPoints = find_cone_tangent_points(apex, forward * coneHeight, circleRadius, camera))
    {
      draw_cached_debug_line(apex, tangentPoints->first, colorOuter);
      draw_cached_debug_line(apex, tangentPoints->second, colorOuter);
    }

    // inner cone
    if (innerAttenuation > 1e-4f && innerAttenuation < 1.0f - 1e-4f)
    {
      for (int32_t i = 0; i < SEGS; ++i)
      {
        Point3 a = innerCircleCenter + pt[i] * innerRadius;
        Point3 b = innerCircleCenter + pt[(i + 1) % SEGS] * innerRadius;
        draw_cached_debug_line(a, b, colorInner);
      }
      if (auto tangentPoints = find_cone_tangent_points(apex, forward * innerConeHeight, innerRadius, camera))
      {
        draw_cached_debug_line(apex, tangentPoints->first, colorInner);
        draw_cached_debug_line(apex, tangentPoints->second, colorInner);
      }
    }

    // spherical part
    Point4 clipPlane(forward.x, forward.y, forward.z, -dot(forward, circleCenter));
    draw_cached_debug_sphere_outline_clipped(camera, apex, radius, clipPlane, colorOuter, SEGS);
  }

  static void draw_letter(const TMatrix &view, char letter, float thickness, float scale, const E3DCOLOR &color)
  {
    const float EXT = 0.5f;
    const Point2 letterSize = Point2(0.74f, 1.f) * scale;
    switch (letter)
    {
      case 'X':
      {
        const Point2 letterX[] = {mul(Point2(-EXT, -EXT), letterSize), mul(Point2(EXT, EXT), letterSize),
          mul(Point2(-EXT, EXT), letterSize), mul(Point2(EXT, -EXT), letterSize)};
        draw_cached_line_strip(view, make_span_const(letterX, 2), thickness, color);
        draw_cached_line_strip(view, make_span_const(letterX + 2, 2), thickness, color);
        break;
      }

      case 'Y':
      {
        Point2 letterY[] = {mul(Point2(-EXT, EXT), letterSize), mul(Point2(0, 0), letterSize), mul(Point2(EXT, EXT), letterSize)};
        draw_cached_line_strip(view, make_span_const(letterY), thickness, color);
        letterY[0] = mul(Point2(0, -EXT), letterSize);
        draw_cached_line_strip(view, make_span_const(letterY, 2), thickness, color);
        break;
      }

      case 'Z':
      {
        const Point2 letterZ[] = {mul(Point2(-EXT, EXT), letterSize), mul(Point2(EXT, EXT), letterSize),
          mul(Point2(-EXT, -EXT), letterSize), mul(Point2(EXT, -EXT), letterSize)};
        draw_cached_line_strip(view, make_span_const(letterZ), thickness, color);
        break;
      }
    }
  }

  static void draw_axis_arrow(const Point3 &pos, int axis, const float height, const E3DCOLOR &color)
  {
    constexpr int SEGS = 6;
    constexpr std::array<std::pair<float, float>, SEGS> points = []() constexpr {
      std::array<std::pair<float, float>, SEGS> points;
      for (int i = 0; i < SEGS; ++i)
      {
        float angle = (float)i * TWOPI / SEGS - PI;
        points[i] = std::make_pair(taylor_cos(angle), taylor_sin(angle));
      }
      return points;
    }();

    const float RADIUS = height * 0.25f;
    Point3 offsets[SEGS];
    Point3 circleCenter(pos);
    switch (axis)
    {
      case 0:
        circleCenter -= Point3(height, 0, 0);
        for (int i = 0; i < SEGS; ++i)
          offsets[i] = Point3(0, points[i].first * RADIUS, points[i].second * RADIUS);
        break;
      case 1:
        circleCenter -= Point3(0, height, 0);
        for (int i = 0; i < SEGS; ++i)
          offsets[i] = Point3(points[i].first * RADIUS, 0, points[i].second * RADIUS);
        break;
      case 2:
        circleCenter -= Point3(0, 0, height);
        for (int i = 0; i < SEGS; ++i)
          offsets[i] = Point3(points[i].first * RADIUS, points[i].second * RADIUS, 0);
        break;
    }
    for (int i = 0; i < SEGS; ++i)
    {
      Point3 p[3] = {pos, circleCenter + offsets[i], circleCenter + offsets[(i + 1) % SEGS]};
      draw_cached_debug_solid_triangle(p, color);
      p[0] = circleCenter;
      draw_cached_debug_solid_triangle(p, color);
    }
  }

  static void draw_axes(const TMatrix &wtm, const Point3 &pos, const E3DCOLOR &color)
  {
    const float EXT = 0.5f;
    TMatrix viewMat;
    d3d::gettm(TM_VIEW, viewMat);
    viewMat = inverse(viewMat * wtm);
    viewMat.orthonormalize();
    const Point3 camera = viewMat.getcol(3);

    // letters
    const float LETTER_SCALE = 0.05f;
    const float LETTER_WIDTH = 0.0025f;
    const float LETTER_OFFSET = 0.1f;
    const float LETTER_MAX_DIST = LETTER_SCALE * 200.0f;
    if (lengthSq(camera - pos) < LETTER_MAX_DIST * LETTER_MAX_DIST)
    {
      viewMat.setcol(3, pos + Point3(EXT + LETTER_OFFSET, 0, 0));
      draw_letter(viewMat, 'X', LETTER_WIDTH, LETTER_SCALE, color);
      viewMat.setcol(3, pos + Point3(0, EXT + LETTER_OFFSET, 0));
      draw_letter(viewMat, 'Y', LETTER_WIDTH, LETTER_SCALE, color);
      viewMat.setcol(3, pos + Point3(0, 0, EXT + LETTER_OFFSET));
      draw_letter(viewMat, 'Z', LETTER_WIDTH, LETTER_SCALE, color);
    }

    // lines
    draw_cached_debug_line(Point3(-EXT, 0, 0), Point3(EXT, 0, 0), color);
    draw_cached_debug_line(Point3(0, -EXT, 0), Point3(0, EXT, 0), color);
    draw_cached_debug_line(Point3(0, 0, -EXT), Point3(0, 0, EXT), color);

    // arrows
    const float ARROW_HEIGHT = EXT * 0.2f;
    draw_axis_arrow(pos + Point3(EXT, 0, 0), 0, ARROW_HEIGHT, color);
    draw_axis_arrow(pos + Point3(0, EXT, 0), 1, ARROW_HEIGHT, color);
    draw_axis_arrow(pos + Point3(0, 0, EXT), 2, ARROW_HEIGHT, color);
  }

  void renderObj()
  {
    set_cached_debug_lines_wtm(tm);

    const E3DCOLOR color(0, 255, 0);

    if (volType == VT_BOX)
    {
      draw_cached_debug_box(bbox, color);

      if (ladderStepsCount > 0)
      {
        Point3 leftPt((bbox[0].x + bbox[1].x) * 0.5f, 0.f, bbox[0].z);
        Point3 rightPt((bbox[0].x + bbox[1].x) * 0.5f, 0.f, bbox[1].z);
        float step = (bbox[1].y - bbox[0].y) / float(ladderStepsCount - 1);
        for (int i = 0; i < ladderStepsCount; ++i)
        {
          float ht = bbox[0].y + i * step;
          draw_cached_debug_line(Point3::xVz(leftPt, ht), Point3::xVz(rightPt, ht), color);
        }

        // arrow
        Point3 a(bbox[1].x, bbox[0].y, bbox[0].z);
        Point3 b(bbox[1].x, bbox[1].y, (bbox[0].z + bbox[1].z) * 0.5f);
        Point3 c(bbox[1].x, bbox[0].y, bbox[1].z);
        if (isFrontFace(a, b, c, tm))
        {
          const E3DCOLOR blueColor(0, 120, 255);
          draw_cached_debug_line(a, b, blueColor);
          draw_cached_debug_line(b, c, blueColor);
        }

        // cross
        a = Point3(bbox[0].x, bbox[0].y, bbox[0].z);
        b = Point3(bbox[0].x, bbox[1].y, bbox[1].z);
        c = Point3(bbox[0].x, bbox[0].y, bbox[1].z);
        if (isFrontFace(c, b, a, tm))
        {
          Point3 d = Point3(bbox[0].x, bbox[1].y, bbox[0].z);
          const E3DCOLOR redColor(255, 0, 0);
          draw_cached_debug_line(a, b, redColor);
          draw_cached_debug_line(c, d, redColor);
        }
      }
    }
    else if (volType == VT_SPH)
      draw_cached_debug_sphere(bsph.c, bsph.r, color);
    else if (volType == VT_PT)
    {
      draw_cached_debug_line(Point3(bbox[0].x, 0, 0), Point3(bbox[1].x, 0, 0), color);
      draw_cached_debug_line(Point3(0, bbox[0].y, 0), Point3(0, bbox[1].y, 0), color);
      draw_cached_debug_line(Point3(0, 0, bbox[0].z), Point3(0, 0, bbox[1].z), color);
    }
    else if (volType == VT_CAPS)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);
      Point3 boxSize = bbox.width();
      float radius = boxSize.x * 0.5f;
      float height = boxSize.z - 2 * radius;
      Point3 a(0, 0, height);
      Point3 b(0, 0, -height);

      real xWidth = length(tm.col[0]);
      real yWidth = length(tm.col[1]);
      real actualRadius = min(xWidth, yWidth) * 0.5f;

      Capsule caps = {a, b, actualRadius};

      draw_cached_debug_capsule(caps, color, tm);
    }
    else if (volType == VT_CYLI)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);
      Point3 boxSize = bbox.width();
      float half_height = boxSize.z * 0.5f;
      Point3 a = Point3(0, 0, half_height);
      Point3 b = Point3(0, 0, -half_height);

      Point3 tm_a = tm * a;
      Point3 tm_b = tm * b;

      Point3 col_x = tm.col[0];
      Point3 col_y = tm.col[1];
      Point3 col_z = tm.col[2];

      boxSize = Point3(length(col_x), length(col_y), length(col_z));

      real width = min(boxSize.x, min(boxSize.y, boxSize.z));
      real radius = width * 0.5f;

      draw_cached_debug_cylinder_w(tm_a, tm_b, radius, color);
    }
    else if (volType == VT_SPOT_LIGHT)
    {
      TMatrix spotLightTm = tm;
      spotLightTm.orthonormalize();
      set_cached_debug_lines_wtm(spotLightTm);
      draw_spot_light_visualization(spotLightTm, bbox.center(), Point3(0, 0, 1), spot_light__cone_angle, light__max_radius,
        spot_light__inner_attenuation);
    }
    else if (volType == VT_AXES)
    {
      draw_axes(tm, bbox.center(), color);
    }
  }

public:
  enum
  {
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;
};


class GameObjEntityManagementService : public IEditorService,
                                       public IObjEntityMgr,
                                       public IBinaryDataBuilder,
                                       public IOnExportNotify,
                                       public IDagorAssetChangeNotify,
                                       public IGatherGroundHoles,
                                       public IGatherRiNavmeshBlockers,
                                       public IGatherNavmeshHoleCutters,
                                       public IGatherGameLadders
{
public:
  GameObjEntityManagementService()
  {
    gameobjEntityClassId = IDaEditor3Engine::get().getAssetTypeId("gameObj");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    rendEntGeom = IDaEditor3Engine::get().registerEntitySubTypeId("single_ent" /*"rend_ent_geom"*/);
    visible = true;
  }

  // IEditorService interface
  const char *getServiceName() const override { return "_goEntMgr"; }
  const char *getServiceFriendlyName() const override { return "(srv) Game object entities"; }

  void setServiceVisible(bool vis) override { visible = vis; }
  bool getServiceVisible() const override { return visible; }

  void actService(float dt) override {}
  void beforeRenderService() override {}
  void renderService() override
  {
    if (!visible)
      return;
    begin_draw_cached_debug_lines();
    int subtypeMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(subtypeMask, lh_mask))
        ent[i]->renderObj();
    end_draw_cached_debug_lines();
  }
  void renderTransService() override {}

  void onBeforeReset3dDevice() override {}

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IOnExportNotify);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, IGatherGroundHoles);
    RETURN_INTERFACE(huid, IGatherRiNavmeshBlockers);
    RETURN_INTERFACE(huid, IGatherNavmeshHoleCutters);
    RETURN_INTERFACE(huid, IGatherGameLadders);
    return NULL;
  }

  void clearServiceData() override
  {
    for (auto &ent : objPool.getEntities())
      if (ent)
        ent->clear();
    objPool.freeUnusedEntities();
  }

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override
  {
    return gameobjEntityClassId >= 0 && gameobjEntityClassId == entity_class;
  }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override
  {
    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, gameobjEntityClassId);
    }

    if (virtual_ent)
    {
      VirtualGameObjEntity *ent = new VirtualGameObjEntity(gameobjEntityClassId);
      ent->setup(asset, true);
      return ent;
    }

    GameObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new GameObjEntity(gameobjEntityClassId);

    ent->setup(asset, false);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
  {
    GameObjEntity *o = reinterpret_cast<GameObjEntity *>(origin);
    GameObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new GameObjEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IDagorAssetChangeNotify interface
  void onAssetRemoved(int asset_name_id, int asset_type) override
  {
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
        ent[i]->clear();
    EDITORCORE->invalidateViewportCache();
  }
  void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type) override
  {
    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
      {
        ent[i]->clear();
        ent[i]->setup(asset, !ent[i]->isNonVirtual());
      }
    EDITORCORE->invalidateViewportCache();
  }

  // IOnExportNotify interface
  void onBeforeExport(unsigned target_code) override
  {
    for (auto *e : objPool.getEntities())
      if (e && e->isNonVirtual())
      {
        for (auto *ve : e->visEntity)
          ve->setSubtype(31); // hide to prevent exporting these entities to final location
        if (auto *b = e->getUserDataBlock(false))
        {
          b->removeParam("hierarchy_unresolved_id");
          b->removeParam("hierarchy_unresolved_parent_id");
          b->removeParam("hierarchy_transform");
        }
      }
    next_hier_id = 0;
    hier_linked_count = 0;
    if (DAGORED2)
      DAGORED2->spawnEvent(_MAKE4C('EnGO'), nullptr);
    if (next_hier_id > 0)
      DAEDITOR3.conNote("gameObjMgr: assigned %d IDs for hierarchy, linked %d children to parents", next_hier_id, hier_linked_count);
  }
  void onAfterExport(unsigned target_code) override
  {
    for (auto *e : objPool.getEntities())
      if (e && e->isNonVirtual())
        for (auto *ve : e->visEntity)
          ve->setSubtype(rendEntGeom);
  }

  // IBinaryDataBuilder interface
  bool validateBuild(int target, ILogWriter &log, PropPanel::ContainerPropertyControl *params) override
  {
    if (!objPool.calcEntities(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT)))
      log.addMessage(log.WARNING, "No gameObj entities for export");
    return true;
  }

  bool addUsedTextures(ITextureNumerator &tn) override { return true; }

  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *) override
  {
    dag::Vector<SrcObjsToPlace> objs;

    dag::ConstSpan<GameObjEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->nameId == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->nameId;
          objs[k].resName = gameObjNames.getName(ent[i]->nameId);
        }

        objs[k].tm.push_back(ent[i]->tm);
        if (ent[i]->userDataBlk)
          objs[k].addBlk.addNewBlock(ent[i]->userDataBlk, "");
        else
          objs[k].addBlk.addNewBlock("");
      }

    for (int k = 0; k < objs.size(); k++)
      if (DagorAsset *a = DAEDITOR3.getAssetByName(objs[k].resName, gameobjEntityClassId))
        objs[k].addBlk.addNewBlock(&a->props, "__asset");

    writeObjsToPlaceDump(cwr, make_span(objs), _MAKE4C('GmO'));
    return true;
  }

  bool checkMetrics(const DataBlock &metrics_blk) override
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    int cnt = objPool.calcEntities(subtype_mask);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: GameObj count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // IGatherGroundHoles interface
  void gatherGroundHoles(LandMeshHolesCell &obstacles) override
  {
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();
    for (GameObjEntity *entity : entities)
      if (entity && entity->isGroundHole &&
          (entity->volType == VirtualGameObjEntity::VT_BOX || entity->volType == VirtualGameObjEntity::VT_SPH))
      {
        TMatrix tm;
        entity->getTm(tm);
        const bool isSphere = entity->volType == VirtualGameObjEntity::VT_SPH;
        obstacles.addHole({tm, isSphere, entity->holeShapeIntersection},
          LandMeshHolesManager::HoleArgs(tm, isSphere, entity->holeShapeIntersection).getBBox2());
      }
  }

  // IGatherRiNavmeshBlockers interface
  void gatherRiNavmeshBlockers(Tab<BBox3> &blockers) override
  {
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();

    for (GameObjEntity *entity : entities)
      if (entity && entity->isRiNavmeshBlocker && entity->volType == VirtualGameObjEntity::VT_BOX)
      {
        TMatrix curTm;
        entity->getTm(curTm);
        blockers.push_back(curTm * entity->getBbox());
      }
  }

  // IGatherNavmeshHoleCutters interface
  void gatherNavmeshHoleCutters(Tab<TMatrix> &cutters) override
  {
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();

    for (GameObjEntity *entity : entities)
      if (entity && entity->isNavmeshHoleCutter && entity->volType == VirtualGameObjEntity::VT_BOX)
      {
        TMatrix curTm;
        entity->getTm(curTm);
        cutters.push_back(curTm);
      }
  }

  // IGatherGameLadders
  void gatherGameLadders(Tab<GameLadder> &ladders) override
  {
    int numLadders = 0;
    dag::ConstSpan<GameObjEntity *> entities = objPool.getEntities();
    for (GameObjEntity *entity : entities)
    {
      if (entity && entity->ladderStepsCount > 0 && entity->volType == VirtualGameObjEntity::VT_BOX)
        ++numLadders;
    }
    ladders.reserve(ladders.size() + numLadders);
    for (GameObjEntity *entity : entities)
    {
      if (entity && entity->ladderStepsCount > 0 && entity->volType == VirtualGameObjEntity::VT_BOX)
      {
        TMatrix tm;
        entity->getTm(tm);
        ladders.push_back(GameLadder(tm, entity->ladderStepsCount));
      }
    }
  }

protected:
  GameObjEntityPool objPool;
  bool visible;
};


void init_gameobj_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) GameObjEntityManagementService);
}
