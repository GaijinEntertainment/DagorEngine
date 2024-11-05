// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/componentTypes.h>

#include <ecs/input/message.h>

#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>
#include <math/dag_approachWindowed.h>
#include <gamePhys/common/loc.h>


ECS_DEF_PULL_VAR(menu_cam);

template <typename Callable>
static void menu_cam_target_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
ECS_REQUIRE(eastl::false_type menu_cam__shouldRotateTarget)
ECS_REQUIRE(eastl::false_type menu_cam__dirInited)
static __forceinline void menu_cam_init_es(const ecs::UpdateStageInfoAct &, const Point3 &menu_cam__initialDir,
  Point2 &menu_cam__initialAngles, bool &menu_cam__dirInited, Point2 &menu_cam__angles, Point2 &menu_cam__angles_smooth_internal)
{
  menu_cam__dirInited = true;
  Point2 angles = dir_to_angles(menu_cam__initialDir);
  menu_cam__initialAngles.set(norm_s_ang(-angles.x), norm_s_ang(angles.y));
  menu_cam__angles.zero();
  menu_cam__angles_smooth_internal.zero();
}

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
ECS_REQUIRE(eastl::false_type menu_cam__shouldRotateTarget)
static __forceinline void menu_cam_es(const ecs::UpdateStageInfoAct &act, ecs::EntityId menu_cam__target,
  const Point3 &menu_cam__offset, const Point3 &menu_cam__offsetMult, const Point2 &menu_cam__initialAngles,
  float menu_cam__target_pos_threshold, Point3 &menu_cam__reference_target_pos_internal, Point2 &menu_cam__angles_smooth_internal,
  Point3 &menu_cam__offset_smooth_internal, Point2 &menu_cam__angles, TMatrix &transform)
{
  TMatrix &itm = transform;
  menu_cam_target_ecs_query(menu_cam__target, [&](const TMatrix &transform) {
    Point2 angleFrom = menu_cam__angles_smooth_internal;
    Point2 angleTo = menu_cam__angles;

    if (fabs(angleTo.x - angleFrom.x) >= PI)
      angleTo.x += 2 * PI * sign(angleFrom.x - angleTo.x);

    menu_cam__angles_smooth_internal = approach_windowed(angleFrom, angleTo, act.dt, 0.2f, 0.05f, 0.035f);
    menu_cam__angles_smooth_internal.x = norm_s_ang(menu_cam__angles_smooth_internal.x);

    Point2 angles = menu_cam__initialAngles + menu_cam__angles_smooth_internal;

    float xSine, xCos;
    float ySine, yCos;
    sincos(angles.x, xSine, xCos);
    sincos(angles.y, ySine, yCos);

    Point3 dir = Point3(xCos * yCos * menu_cam__offsetMult.x, ySine * menu_cam__offsetMult.y, xSine * yCos * menu_cam__offsetMult.z);

    if (menu_cam__offset.lengthSq() > 0.0f)
      menu_cam__offset_smooth_internal = approach(menu_cam__offset_smooth_internal, menu_cam__offset, act.dt, 0.035f);

    itm.setcol(2, dir);
    itm.setcol(0, normalize(Point3(0.f, 1.f, 0.f) % dir));
    itm.setcol(1, normalize(dir % itm.getcol(0)));

    Point3 lookAt = transform.getcol(3);

    if (menu_cam__target_pos_threshold > 0.f)
    {
      if (lengthSq(menu_cam__reference_target_pos_internal - lookAt) <= sqr(menu_cam__target_pos_threshold))
        lookAt = menu_cam__reference_target_pos_internal;
      else
        menu_cam__reference_target_pos_internal = lookAt;
    }

    Point3 lookFrom = (itm % menu_cam__offset_smooth_internal) + lookAt;
    itm.setcol(3, lookFrom);
    itm.setcol(2, normalize(dir));
  });
}

template <typename Callable>
static void menu_rotate_target_cam_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
static void menu_rotate_target_cam_with_offset_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_AFTER(menu_cam_es)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
ECS_BEFORE(menu_rotate_target_cam_es)
ECS_REQUIRE(eastl::true_type menu_cam__shouldRotateTarget)
ECS_REQUIRE(eastl::false_type menu_cam__dirInited)
static __forceinline void menu_rotate_target_cam_init_es(const ecs::UpdateStageInfoAct &, ecs::EntityId menu_cam__target,
  ecs::EntityId &menu_cam__lastTarget, TMatrix &menu_cam__initialTransform, Point2 &menu_cam__angles,
  Point2 &menu_cam__angles_smooth_internal, bool &menu_cam__dirInited)
{
  menu_rotate_target_cam_ecs_query(menu_cam__target, [&](TMatrix &transform) {
    menu_cam__dirInited = true;
    menu_cam__angles.zero();
    menu_cam__angles_smooth_internal.zero();
    // This is needed to avoid transform of an object getting out of it's initial position if we selected the same object twice
    if (menu_cam__lastTarget != menu_cam__target || menu_cam__lastTarget == ecs::INVALID_ENTITY_ID)
      menu_cam__initialTransform = transform;
    menu_cam__lastTarget = menu_cam__target;
  });
}

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
ECS_REQUIRE(eastl::true_type menu_cam__shouldRotateTarget)
ECS_REQUIRE(eastl::true_type menu_cam__dirInited)
static __forceinline void menu_rotate_target_cam_es(const ecs::UpdateStageInfoAct &, const ecs::EntityId &menu_cam__target,
  const TMatrix menu_cam__initialTransform, Point2 &menu_cam__angles, const TMatrix &transform)
{
  const TMatrix &cameraTransform = transform;
  menu_rotate_target_cam_with_offset_ecs_query(menu_cam__target,
    [&](TMatrix &transform, const Point3 &menu_item__boundingBoxCenter,
      const Point3 &menu_item__rotationOffset = Point3(0.f, 0.f, 0.f), const float menu_item__cameraCenterOffsetProportion = 1.0f) {
      const Quat qY(Point3(0.f, 1.f, 0.f), menu_cam__angles.x);
      const Quat qP(cameraTransform.getcol(0), menu_cam__angles.y);

      transform = quat_to_matrix(qP * qY) % menu_cam__initialTransform;
      const Point3 rotationOffset = transform % (menu_item__boundingBoxCenter + menu_item__rotationOffset);
      const Point3 itemPos = menu_cam__initialTransform.getcol(3) - rotationOffset;

      Point3 projectionOffset = Point3(0.0, 0.0, 0.0);
      if (menu_item__cameraCenterOffsetProportion != 1.0f)
      {
        const Point3 &cameraForward = cameraTransform.getcol(2);
        const Point3 projectedItemPos = project_onto_plane(menu_cam__initialTransform.getcol(3), cameraForward) -
                                        project_onto_plane(cameraTransform.getcol(3), cameraForward);
        projectionOffset = projectedItemPos * (menu_item__cameraCenterOffsetProportion - 1.0f);
      }

      transform.setcol(3, itemPos + projectionOffset);
    });
}
