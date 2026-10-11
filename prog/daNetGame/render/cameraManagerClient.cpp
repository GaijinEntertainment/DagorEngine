// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cameraManager/cameraManager_api.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <math/dag_Point3.h>
#include "camera/sceneCam.h"
#include "main/gameLoad.h"
#include "main/level.h"
#include "render/renderer.h"
#include "render/screencap.h"

static bool get_camera(camera_manager::CameraState &state)
{
  const ecs::EntityId cam = get_cur_cam_entity();
  if (!g_entity_mgr->doesEntityExist(cam))
    return false;
  state.itm = get_cam_itm();
  state.fovDeg = g_entity_mgr->getOr(cam, ECS_HASH("fov"), state.fovDeg);
  return true;
}

static void set_camera(const camera_manager::CameraState &state)
{
  enable_free_camera();
  set_cam_itm(state.itm);
  const ecs::EntityId cam = get_cur_cam_entity();
  if (Point3 *upVector = g_entity_mgr->getNullableRW<Point3>(cam, ECS_HASH("free_camera__upVector")))
    *upVector = state.itm.getcol(1);
  if (float *fov = g_entity_mgr->getNullableRW<float>(cam, ECS_HASH("fov")))
    *fov = state.fovDeg;
}

static eastl::string get_mission_name()
{
  if (!is_level_loaded())
    return eastl::string();
  return sceneload::get_current_game().sceneName;
}

static void request_screenshot(const char *name) { screencap::schedule_screenshot(false, -1, name); }

static struct CameraManagerClientInit
{
  CameraManagerClientInit() { camera_manager::init({get_camera, set_camera, get_mission_name, request_screenshot}); }
} camera_manager_client_init;

void term_camera_manager_client() { camera_manager::shutdown(); }

ECS_DEF_PULL_VAR(cameraManagerClient);
