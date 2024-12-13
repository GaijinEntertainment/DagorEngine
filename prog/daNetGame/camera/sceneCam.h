// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/net/msgDecl.h>

class TMatrix;
class SimpleString;

bool create_free_camera(); // Note: not reentrant

ecs::EntityId set_scene_camera_entity(ecs::EntityId cam_eid); // returns handle to old camera
void toggle_free_camera();
void enable_free_camera();
void disable_free_camera();
void force_free_camera();
ecs::EntityId enable_spectator_camera(const TMatrix &itm, int team, ecs::EntityId target);
bool is_free_camera_enabled();
ecs::EntityId get_cur_cam_entity();

enum CreateCameraFlags : uint8_t
{
  CCF_SET_TARGET_TEAM = 1 << 0,
  CCF_SET_ENTITY = 1 << 1,
};

const TMatrix &get_cam_itm();
void set_cam_itm(const TMatrix &itm);
extern Point3 get_free_cam_speeds();

void reset_all_cameras();
bool set_spectated(ecs::EntityId cam_eid, ecs::EntityId target);
