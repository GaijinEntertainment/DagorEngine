// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;
class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
class TMatrix;
struct E3DCOLOR;

namespace lvl_override
{
#define OVERRIDE_BLK_FUNC_LIST   \
  GET_ITEM_F(const char *, Str)  \
  GET_ITEM_F(bool, Bool)         \
  GET_ITEM_F(int, Int)           \
  GET_ITEM_F(float, Real)        \
  GET_ITEM_F(Point2, Point2)     \
  GET_ITEM_F(Point3, Point3)     \
  GET_ITEM_F(Point4, Point4)     \
  GET_ITEM_F(IPoint2, IPoint2)   \
  GET_ITEM_F(IPoint3, IPoint3)   \
  GET_ITEM_F(E3DCOLOR, E3dcolor) \
  GET_ITEM_F(TMatrix, Tm)

#define GET_ITEM_F(type, fname) extern type get##fname(const DataBlock *data_blk, const char *name, type def);
OVERRIDE_BLK_FUNC_LIST
#undef GET_ITEM_F
} // namespace lvl_override