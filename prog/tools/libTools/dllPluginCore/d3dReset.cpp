#include <3d/dag_drv3d.h>

bool dagor_d3d_force_driver_reset = false;
void d3derr_in_device_reset(const char *msg) { fatal("%s:\n%s (device reset)", msg, d3d::get_last_error()); }
