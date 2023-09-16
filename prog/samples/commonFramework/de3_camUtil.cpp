#include "de3_ICamera.h"
#include "de3_freeCam.h"
#include <math/dag_TMatrix.h>

float IFreeCameraDriver::zNear = 0.1, IFreeCameraDriver::zFar = 1000.0;
float IFreeCameraDriver::turboScale = 20;
bool IFreeCameraDriver::moveWithWASD = true;
bool IFreeCameraDriver::moveWithArrows = true;
bool IFreeCameraDriver::isActive = true;

void set_camera_pos(IGameCamera &cam, float x, float y, float z) { set_camera_pos(cam, Point3(x, y, z)); }

void set_camera_pos(IGameCamera &cam, const Point3 &pos)
{
  TMatrix tm;
  cam.getInvViewMatrix(tm);
  tm.setcol(3, pos);
  cam.setInvViewMatrix(tm);
}

static bool cam_reversed_depth = false;
void dagor_use_reversed_depth(bool rev) { cam_reversed_depth = rev; }
bool dagor_is_reversed_depth() { return cam_reversed_depth; }
