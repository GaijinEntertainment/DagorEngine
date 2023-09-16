#ifndef __DAGOR_NAV_EXTERNAL_H
#define __DAGOR_NAV_EXTERNAL_H
#pragma once

extern bool (*nav_rt_trace_ray_norm)(const Point3 &p, const Point3 &dir, float &maxt, Point3 &norm);
extern float (*nav_rt_clip_capsule)(Capsule &c, Point3 &capPt, Point3 &worldPt);
extern const BBox3 &(*nav_rt_get_bbox3)();
extern bool (*nav_is_relevant_point)(const Point3 &p);

#endif