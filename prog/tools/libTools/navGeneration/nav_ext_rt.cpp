#include <navigation/dag_navInterface.h>
#include <libTools/navGeneration/nav_external.h>

const BBox3 &(*nav_rt_get_bbox3)() = NULL;
bool (*nav_is_relevant_point)(const Point3 &p) = NULL;
bool (*nav_rt_trace_ray_norm)(const Point3 &p, const Point3 &dir, float &maxt, Point3 &norm) = NULL;
float (*nav_rt_clip_capsule)(Capsule &c, Point3 &capPt, Point3 &worldPt) = NULL;
