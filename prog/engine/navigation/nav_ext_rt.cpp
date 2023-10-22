#include <navigation/dag_navInterface.h>

bool (*nav_rt_trace_ray)(const Point3 &p, const Point3 &dir, float &maxt) = NULL;
bool (*nav_rt_get_height)(const Point3 &p, float *ht, Point3 *normal) = NULL;
