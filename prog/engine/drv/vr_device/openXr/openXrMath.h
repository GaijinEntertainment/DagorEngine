// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <openxr/openxr.h>

#include <drv/dag_vr.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>

inline DPoint3 as_dpoint3(XrVector3f v) { return DPoint3{v.x, v.y, -v.z}; }
inline Quat as_quat(XrQuaternionf q) { return Quat{-q.x, -q.y, q.z, q.w}; }

inline bool is_orientation_valid(XrQuaternionf q) { return q.x != 0 || q.y != 0 || q.z != 0 || q.w != 0; }
inline bool is_orientation_valid(Quat q) { return q.x != 0 || q.y != 0 || q.z != 0 || q.w != 0; }
