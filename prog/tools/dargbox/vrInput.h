#pragma once

#include <math/dag_TMatrix.h>

namespace vr
{

void handle_controller_input();
void render_controller_poses();
TMatrix get_aim_pose(int index);
TMatrix get_grip_pose(int index);

} // namespace vr
