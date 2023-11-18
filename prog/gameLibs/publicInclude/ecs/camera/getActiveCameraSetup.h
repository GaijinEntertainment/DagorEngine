//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <daECS/core/entityId.h>

struct Driver3dPerspective;

enum FovMode
{
  EFM_HOR_PLUS,
  EFM_HYBRID
};

struct CameraSetup
{
  TMatrix transform = TMatrix::IDENT;
  DPoint3 accuratePos = {0, 0, 0};

  float fov = 90.f;
  float znear = 0.1f;
  float zfar = 5000.f;

  FovMode fovMode = EFM_HOR_PLUS;

  ecs::EntityId camEid;
};

//! retrieves properties of active camera (checks that only one camera entity is active at one time)
namespace ecs
{
class EntityManager;
}
CameraSetup get_active_camera_setup(ecs::EntityManager &manager);
CameraSetup get_active_camera_setup();

//! Convinience function to calculate glob tm from camera
TMatrix4 calc_active_camera_globtm();

//! Calculates camera transforms and such
void calc_camera_values(const CameraSetup &camera_setup, TMatrix &view_tm, Driver3dPerspective &persp, int &view_w, int &view_h);
//! Detailed calculation functions
TMatrix calc_camera_view_tm(const TMatrix &view_itm);
Driver3dPerspective calc_camera_perspective(const CameraSetup &camera_setup, int view_w, int view_h);

//! sets camera parameters to D3D
void apply_camera_setup(const CameraSetup &camera_setup);
void apply_camera_setup(const TMatrix &view_itm, const TMatrix &view_tm, const Driver3dPerspective &persp, int view_w, int view_h);
