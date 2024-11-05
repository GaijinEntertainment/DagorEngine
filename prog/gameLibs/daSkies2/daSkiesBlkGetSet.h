// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>

namespace skies_utils
{
inline Color3 color3(const Point3 &c) { return Color3(c.x, c.y, c.z); }
inline Point3 point3(const Color3 &c) { return Point3(c.r, c.g, c.b); }

inline void get_value(Color3 &v, const DataBlock &blk, const char *name, const Color3 &def)
{
  v = color3(blk.getPoint3(name, point3(def)));
}
inline void get_value(float &v, const DataBlock &blk, const char *name, float def) { v = blk.getReal(name, def); }
inline void get_value(int &v, const DataBlock &blk, const char *name, int def) { v = blk.getInt(name, def); }
inline void get_value(bool &v, const DataBlock &blk, const char *name, bool def) { v = blk.getBool(name, def); }

inline void set_value(DataBlock &blk, const char *name, Color3 val) { blk.setPoint3(name, point3(val)); }
inline void set_value(DataBlock &blk, const char *name, float val) { blk.setReal(name, val); }
inline void set_value(DataBlock &blk, const char *name, int val) { blk.setInt(name, val); }
inline void set_value(DataBlock &blk, const char *name, bool val) { blk.setBool(name, val); }
inline void set_value(DataBlock &blk, const char *name, Point2 val) { blk.setPoint2(name, val); }
} // namespace skies_utils