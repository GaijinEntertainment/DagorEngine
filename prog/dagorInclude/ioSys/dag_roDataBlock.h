//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_e3dColor.h>
#include <util/dag_roNameMap.h>

#include <supp/dag_define_KRNLIMP.h>

class IGenLoad;
class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
class IPoint4;
class TMatrix;


/// Read-only data block (with interface similar to DataBlock)
struct RoDataBlock
{
public:
  /// Parameter types enum.
  enum ParamType
  {
    TYPE_NONE,
    TYPE_STRING,   ///< Text string.
    TYPE_INT,      ///< Integer.
    TYPE_REAL,     ///< #real (float).
    TYPE_POINT2,   ///< Point2.
    TYPE_POINT3,   ///< Point3.
    TYPE_POINT4,   ///< Point4.
    TYPE_IPOINT2,  ///< IPoint2.
    TYPE_IPOINT3,  ///< IPoint3.
    TYPE_BOOL,     ///< Boolean.
    TYPE_E3DCOLOR, ///< E3DCOLOR.
    TYPE_MATRIX,   ///< TMatrix.
    TYPE_INT64,    ///< int64_t
    TYPE_IPOINT4,  ///< IPoint4.
  };


  /// empty Read-only data block constructor
  RoDataBlock()
  {
    memset(this, 0, sizeof(*this));
    nameId = -1;
  }

  /// create Read-only data block via loading dump from stream
  KRNLIMP static RoDataBlock *load(IGenLoad &crd, int sz = -1);

  /// patch data once after creating from dump
  KRNLIMP void patchData(void *base);

  /// patch data once after creating from dump
  inline void patchNameMap(void *base) { nameMap->patchData(base); }

  /// Returns name id from NameMap, or -1 if there's no such name in the NameMap.
  KRNLIMP int getNameId(const char *name) const;
  KRNLIMP const char *getName(int name_id) const;

  /// Returns name id of this RoDataBlock.
  int getBlockNameId() const { return nameId; }

  /// Returns name of this RoDataBlock.
  const char *getBlockName() const { return getName(nameId); }

  /// Returns number of sub-blocks in this RoDataBlock.
  /// Use for enumeration.
  int blockCount() const { return blocks.size(); }

  /// Returns pointer to i-th sub-block.
  const RoDataBlock *getBlock(uint32_t idx) const { return safe_at(blocks, idx); }
  RoDataBlock *getBlock(uint32_t idx) { return safe_at(blocks, idx); }

  /// Returns pointer to sub-block with specified name id, or NULL if not found.
  KRNLIMP RoDataBlock *getBlockByName(int name_id, int start_after = -1);
  KRNLIMP RoDataBlock *getBlockByName(int name_id, int start_after = -1) const
  {
    return const_cast<RoDataBlock *>(this)->getBlockByName(name_id, start_after);
  }

  /// Returns pointer to sub-block with specified name, or NULL if not found.
  RoDataBlock *getBlockByName(const char *name, int start_after = -1) { return getBlockByName(getNameId(name), start_after); }
  const RoDataBlock *getBlockByName(const char *name, int start_after = -1) const
  {
    return getBlockByName(getNameId(name), start_after);
  }

  /// Get block by name, returns (always valid) @b emptyBlock, if not found.
  const RoDataBlock *getBlockByNameEx(const char *name) const
  {
    const RoDataBlock *blk = getBlockByName(name);
    return blk ? blk : &emptyBlock;
  }
  RoDataBlock *getBlockByNameEx(const char *name)
  {
    RoDataBlock *blk = getBlockByName(name);
    return blk ? blk : &emptyBlock;
  }

  /// @name Parameters - Getting and Enumeration
  /// @{

  /// Returns number of parameters in this RoDataBlock.
  /// Use for enumeration.
  int paramCount() const { return params.size(); }

  /// Returns type of i-th parameter. See ParamType enum.
  int getParamType(int idx) const { return (idx >= 0 && idx < params.size()) ? params[idx].type : TYPE_NONE; }

  /// Returns i-th parameter name id. See getNameId().
  int getParamNameId(int idx) const { return (idx >= 0 && idx < params.size()) ? params[idx].nameId : -1; }

  /// Returns i-th parameter name. Uses getName().
  const char *getParamName(int param_number) const { return getName(getParamNameId(param_number)); }

  /// Find parameter by name id.
  /// Returns parameter index or -1 if not found.
  KRNLIMP int findParam(int name_id, int start_after = -1) const;

  /// Find parameter by name. Uses getNameId().
  /// Returns parameter index or -1 if not found.
  int findParam(const char *name, int start_after = -1) const { return findParam(getNameId(name), start_after); }

  /// Returns true if there is parameter with specified name id in this RoDataBlock.
  bool paramExists(int name_id, int start_after = -1) const { return findParam(name_id, start_after) >= 0; }

  /// Returns true if there is parameter with specified name in this RoDataBlock.
  bool paramExists(const char *name, int start_after = -1) const { return findParam(name, start_after) >= 0; }

  const char *getStr(int idx) const { return (char *)(ptrdiff_t(nameMap.get()) + params[idx].i); }
  bool getBool(int idx) const { return params[idx].b; }
  int getInt(int idx) const { return params[idx].i; }
  float getReal(int idx) const { return params[idx].r; }
  E3DCOLOR getE3dcolor(int idx) const { return (E3DCOLOR)params[idx].i; }
  const Point2 &getPoint2(int idx) const { return castParam<Point2>(idx); }
  const Point3 &getPoint3(int idx) const { return castParam<Point3>(idx); }
  const Point4 &getPoint4(int idx) const { return castParam<Point4>(idx); }
  const IPoint2 &getIPoint2(int idx) const { return castParam<IPoint2>(idx); }
  const IPoint3 &getIPoint3(int idx) const { return castParam<IPoint3>(idx); }
  const IPoint4 &getIPoint4(int idx) const { return castParam<IPoint4>(idx); }
  const TMatrix &getTm(int idx) const { return castParam<TMatrix>(idx); }
  int64_t getInt64(int idx) const { return *(int64_t *)(ptrdiff_t(nameMap.get()) + params[idx].i); }

  const char *getStr(const char *name, const char *def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_STRING)
      return getStr(i);
    return def;
  }

  bool getBool(const char *name, bool def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_BOOL)
      return getBool(i);
    return def;
  }

  int getInt(const char *name, int def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_INT)
      return getInt(i);
    return def;
  }

  float getReal(const char *name, float def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_REAL)
      return getReal(i);
    return def;
  }

  E3DCOLOR getE3dcolor(const char *name, E3DCOLOR def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_E3DCOLOR)
      return getE3dcolor(i);
    return def;
  }

  const Point2 &getPoint2(const char *name, const Point2 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_POINT2)
      return getPoint2(i);
    return def;
  }

  const Point3 &getPoint3(const char *name, const Point3 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_POINT3)
      return getPoint3(i);
    return def;
  }

  const Point4 &getPoint4(const char *name, const Point4 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_POINT4)
      return getPoint4(i);
    return def;
  }

  const IPoint2 &getIPoint2(const char *name, const IPoint2 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_IPOINT3)
      return getIPoint2(i);
    return def;
  }

  const IPoint3 &getIPoint3(const char *name, const IPoint3 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_IPOINT3)
      return getIPoint3(i);
    return def;
  }

  const IPoint4 &getIPoint4(const char *name, const IPoint4 &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_IPOINT4)
      return getIPoint4(i);
    return def;
  }

  const TMatrix &getTm(const char *name, const TMatrix &def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_MATRIX)
      return getTm(i);
    return def;
  }

  int64_t getInt64(const char *name, int64_t def) const
  {
    int i = findParam(name);
    if (i >= 0 && params[i].type == TYPE_INT64)
      return getInt64(i);
    return def;
  }

  inline bool isValid() const { return true; }

protected:
  struct Param
  {
    union
    {
      int i;
      bool b;
      float r;
    };
    uint16_t nameId, type;
  };

  PatchableTab<Param> params;
  PatchableTab<RoDataBlock> blocks;
  PatchablePtr<RoNameMap> nameMap;
  int nameId;
  int _resv;

  KRNLIMP static RoDataBlock emptyBlock;
  template <class T>
  const T &castParam(int idx) const
  {
    return *(T *)(ptrdiff_t(nameMap.get()) + params[idx].i);
  }
};

#include <supp/dag_undef_KRNLIMP.h>
