// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>
#include <render/collimatorMoaCore/moaShapesPacking.h>

#include <generic/dag_enumerate.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_info.h>
#include <util/dag_convar.h>


CONSOLE_BOOL_VAL("collimator_moa", always_redraw_shapes, false);

constexpr int CBUF_REGS_LIMIT = 4096 / (4 * 4);
constexpr int CBUF_REGS_SOFT_LIMIT = CBUF_REGS_LIMIT / 2;
constexpr int MAX_CBUF_REG_PER_SHAPE = 3;
constexpr int MAX_SHAPES_COUNT_PER_IMAGE = CBUF_REGS_SOFT_LIMIT / MAX_CBUF_REG_PER_SHAPE;

using collimator_moa::SDFShape;

#define SDF_SHAPE(name, ecs_name, _val) #ecs_name,
static const char *shape_names[(int)SDFShape::COUNT] = {
#include <render/collimatorMoaCore/shape_tags.hlsli>
};
#undef SDF_SHAPE

template <typename Callable>
inline void gather_gun_mode_collimator_moa_shapes_ecs_query(ecs::EntityId, Callable c);

using ShapeTypesVector = dag::Vector<SDFShape, framemem_allocator>;

static SDFShape to_shape_type_id(const char *shape_name)
{
  for (int i = 0; i < (int)SDFShape::COUNT; ++i)
    if (!strcmp(shape_names[i], shape_name))
      return (SDFShape)i;

  logerr("collimator_moa: encountered unexpected shape_name %s", shape_name);
  return SDFShape::CIRCLE;
}

static bool is_valid_shape_name(const char *name)
{
  for (int i = 0; i < (int)SDFShape::COUNT; ++i)
    if (!strcmp(shape_names[i], name))
      return true;

  return false;
}

static void pack_shape(dag::Vector<Point4, framemem_allocator> &dst, const ecs::Object &shape, const SDFShape shape_type)
{
  switch (shape_type)
  {
    case SDFShape::CIRCLE:
    {
      const Point2 center = shape.getMemberOr(ECS_HASH("center"), Point2(0, 0));
      const float radius = shape.getMemberOr(ECS_HASH("radius"), 1.0f);

      collimator_moa::pack_circle_to(dst, center, radius);
      break;
    }

    case SDFShape::RING:
    {
      const Point2 center = shape.getMemberOr(ECS_HASH("center"), Point2(0, 0));
      const float radius = shape.getMemberOr(ECS_HASH("radius"), 0.0f);
      const float width = shape.getMemberOr(ECS_HASH("width"), 0.0f);

      collimator_moa::pack_ring_to(dst, center, radius, width);
      break;
    }

    case SDFShape::LINE:
    {
      const Point2 begin = shape.getMemberOr(ECS_HASH("begin"), Point2(0, 0));
      const Point2 end = shape.getMemberOr(ECS_HASH("end"), Point2(1, 0));
      const float halfWidth = 0.5 * shape.getMemberOr(ECS_HASH("width"), 1.0f);

      collimator_moa::pack_line_to(dst, begin, end, halfWidth);
      break;
    }

    case SDFShape::TRIANGLE:
    {
      const Point2 a = shape.getMemberOr(ECS_HASH("point_a"), Point2(-1, 0));
      const Point2 b = shape.getMemberOr(ECS_HASH("point_b"), Point2(0, 1));
      const Point2 c = shape.getMemberOr(ECS_HASH("point_c"), Point2(1, 0));

      collimator_moa::pack_triangle_to(dst, a, b, c);
      break;
    }

    case SDFShape::ARC:
    {
      const Point2 center = shape.getMemberOr(ECS_HASH("center"), Point2(0, 0));
      const float radius = shape.getMemberOr(ECS_HASH("radius"), 1.0f);
      const float halfWidth = shape.getMemberOr(ECS_HASH("width"), 1.0f) * 0.5f;
      const float beginAngle = shape.getMemberOr(ECS_HASH("begin"), 0.0f) * DEG_TO_RAD;
      const float endAngle = shape.getMemberOr(ECS_HASH("end"), 90.0f) * DEG_TO_RAD;

      collimator_moa::pack_arc_to(dst, center, radius, halfWidth, beginAngle, endAngle);
      break;
    }

    default: logerr("collimator_moa: encountered unknown shape type %d during cbuffer packing", (int)shape_type);
  }
}

static SDFShape get_shape_type(const ecs::Object &shape)
{
  const char *shapeName = shape.getMemberOr(ECS_HASH("type"), "");
  return to_shape_type_id(shapeName);
}

struct ShapesBufSize
{
  int shapesCount = 0;
  int cbufRegCount = 0;
};

struct PackedShapes
{
  dag::Vector<Point4, framemem_allocator> buf;
  int shapesCount = 0;
};

static PackedShapes pack_shapes(const ecs::Array &shapes)
{
  PackedShapes packedData;

  for (const ecs::ChildComponent &child : shapes)
  {
    const ecs::Object *shape = child.getNullable<ecs::Object>();
    G_ASSERT_CONTINUE(shape);

    const SDFShape shapeType = get_shape_type(*shape);
    pack_shape(packedData.buf, *shape, shapeType);

    ++packedData.shapesCount;
  }

  return packedData;
}

static ShapesBufSize update_shapes_buf(UniqueBufHolder &shapes_buf, const ecs::Array &shapes)
{
  FRAMEMEM_REGION;
  PackedShapes packedShapes = pack_shapes(shapes);

  // metal validates that cbuffers size match what is written in shader so it has to match
  const int cbufferRegsCount = d3d::get_driver_code().is(d3d::metal) ? 1024 : packedShapes.buf.size();
  shapes_buf = dag::buffers::create_persistent_cb(cbufferRegsCount, "dcm_shapes_buf");

  if (!shapes_buf)
  {
    logerr("collimator_moa: failed to allocated shapes buf with regs count '%d'", cbufferRegsCount);
    return {};
  }

  if (!shapes_buf->updateDataWithLock(0, cbufferRegsCount * sizeof(packedShapes.buf[0]), packedShapes.buf.data(), VBLOCK_WRITEONLY))
  {
    logerr("collimator_moa: failed to update shapes buffer with regs count '%d'", cbufferRegsCount);
    shapes_buf.close();
    return {};
  }

  return {packedShapes.shapesCount, cbufferRegsCount};
}

ecs::EntityId get_gun_mod_collimator_image_eid(const ecs::EntityId gun_mod_eid)
{
  ecs::EntityId imgEid;
  if (gun_mod_eid)
    imgEid = ECS_GET_OR(gun_mod_eid, gunmod__collimator_moa_img_eid, ecs::EntityId{});

  return imgEid;
}

const ecs::Array *get_collimator_moa_image_shapes(const ecs::EntityId img_eid)
{
  const ecs::Array *res = nullptr;
  if (img_eid)
  {
    bool shapesValid = false;
    const ecs::Array *shapes;
    gather_gun_mode_collimator_moa_shapes_ecs_query(img_eid,
      [&shapes, &shapesValid](const ecs::Array &collimator_moa_image__shapes, const bool collimator_moa_image__valid) {
        shapes = &collimator_moa_image__shapes;
        shapesValid = collimator_moa_image__valid;
      });

    res = shapesValid ? shapes : nullptr;
  }

  return res;
}

ECS_TAG(render)
ECS_NO_ORDER
static void collimator_moa_track_selected_image_es(const UpdateStageInfoBeforeRender &,
  const ecs::EntityId collimator_moa_render__gun_mod_eid, ecs::EntityId &collimator_moa_render__active_image_eid,
  int &collimator_moa_render__active_shapes_count, int &collimator_moa_render__shapes_buf_reg_count,
  UniqueBufHolder &collimator_moa_render__current_shapes_buf)
{
  const ecs::EntityId imgEid = get_gun_mod_collimator_image_eid(collimator_moa_render__gun_mod_eid);
  if (collimator_moa_render__active_image_eid == imgEid && !always_redraw_shapes)
    return;

  const ecs::Array *imgShapes = get_collimator_moa_image_shapes(imgEid);
  if (!imgShapes)
  {
    collimator_moa_render__current_shapes_buf.close();
    collimator_moa_render__active_image_eid = ecs::EntityId{};
    return;
  }

  G_ASSERT_RETURN(!imgShapes->empty(), );
  const auto [totalShapes, cbufRegCount] = update_shapes_buf(collimator_moa_render__current_shapes_buf, *imgShapes);

  collimator_moa_render__active_image_eid = imgEid;
  collimator_moa_render__active_shapes_count = totalShapes;
  collimator_moa_render__shapes_buf_reg_count = cbufRegCount;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(collimator_moa_image__shapes)
static void collimator_moa_image_validation_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  const ecs::Array &collimator_moa_image__shapes, bool &collimator_moa_image__valid)
{
  collimator_moa_image__valid = false;

  const char *tmpl = g_entity_mgr->getEntityTemplateName(eid);
  tmpl = tmpl ? tmpl : "[unknown]";

  if (collimator_moa_image__shapes.empty())
  {
    logerr("'%s' shapes array is empty", tmpl);
    return;
  }

  if (collimator_moa_image__shapes.size() > MAX_SHAPES_COUNT_PER_IMAGE)
  {
    logerr("'%s' shapes array exceeds max amount of shapes: %d > %d", collimator_moa_image__shapes.size(), MAX_SHAPES_COUNT_PER_IMAGE);
    return;
  }

  bool validationPassed = true;
  for (auto [iShape, shapeChildComponent] : enumerate(collimator_moa_image__shapes))
  {
    const ecs::Object *shapeObj = shapeChildComponent.getNullable<ecs::Object>();
    if (!shapeObj)
    {
      logerr("%s.shapes[%d] must be type of ecs::Object", tmpl, iShape);
      validationPassed = false;
      continue;
    }

    const char *shapeName = shapeObj->getMemberOr(ECS_HASH("type"), "");
    if (!shapeName)
    {
      logerr("%s.shapes[%d] misses type:t", tmpl, iShape);
      validationPassed = false;
      continue;
    }

    if (!is_valid_shape_name(shapeName))
    {
      logerr("%s.shapes[%d].type has unknown type '%s'", tmpl, iShape, shapeName);
      validationPassed = false;
      continue;
    }
  }

  collimator_moa_image__valid = validationPassed;
  return;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(gunmod__collimator_moa_img_template)
static void gunmod_track_collimator_moa_img_eid_es_event_handler(const ecs::Event &,
  const ecs::string &gunmod__collimator_moa_img_template, ecs::EntityId &gunmod__collimator_moa_img_eid)
{
  gunmod__collimator_moa_img_eid = ecs::EntityId{};
  if (!gunmod__collimator_moa_img_template.empty())
    gunmod__collimator_moa_img_eid = g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(gunmod__collimator_moa_img_template.c_str()));
}