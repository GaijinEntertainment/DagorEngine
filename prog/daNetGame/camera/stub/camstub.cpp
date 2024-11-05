// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "camera/sceneCam.h"
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/componentTypes.h>
#include <ecs/camera/getActiveCameraSetup.h>

ECS_DEF_PULL_VAR(animCharCameraTarget);

extern const size_t sq_autobind_pull_bind_scene_camera = 0;

namespace camtrack
{
void stop() {}
void play(const char *) {}
} // namespace camtrack

class DataBlock;

const TMatrix &get_cam_itm() { return TMatrix::IDENT; }
void set_cam_itm(const TMatrix &) {}
void enable_free_camera() {}
void force_free_camera() {}
ecs::EntityId enable_spectator_camera(const TMatrix &, int, ecs::EntityId) { return ecs::INVALID_ENTITY_ID; }
void reset_all_cameras() {}
ecs::EntityId get_cur_cam_entity() { return ecs::INVALID_ENTITY_ID; }
ecs::EntityId set_scene_camera_entity(ecs::EntityId) { return ecs::INVALID_ENTITY_ID; }
CameraSetup get_active_camera_setup() { return CameraSetup(); }
void calc_camera_values(const CameraSetup &, TMatrix &, Driver3dPerspective &, int &, int &) {}
TMatrix4 calc_active_camera_globtm() { return TMatrix4::IDENT; }
