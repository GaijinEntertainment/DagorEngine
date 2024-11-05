//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

class DagorAsset;
class TMatrix;
class BSphere3;
class BBox3;


class IObjEntity
{
public:
  static const int ST_NOT_COLLIDABLE = 31; //< hardcoded subtype that disables collision test for entity

  inline IObjEntity(int cls) : entityClass(cls), subType(0), editLayerIdx(0), flags(0) {}

  virtual void setTm(const TMatrix &tm) = 0;
  virtual void getTm(TMatrix &tm) const = 0;
  virtual void destroy() = 0;

  virtual void *queryInterfacePtr(unsigned huid) = 0;
  template <class T>
  inline T *queryInterface()
  {
    return (T *)queryInterfacePtr(T::HUID);
  }

  //! returns bounding sphere in local(!) space
  virtual BSphere3 getBsph() const = 0;
  //! returns axis-aligned bounding box in local(!) space
  virtual BBox3 getBbox() const = 0;

  inline int getAssetTypeId() const { return entityClass; }
  inline int getSubtype() const { return subType; }
  virtual void setSubtype(int t) { subType = t; }
  inline int getEditLayerIdx() const { return editLayerIdx; }
  virtual void setEditLayerIdx(int t) { editLayerIdx = t; }

  inline int checkSubtypeMask(unsigned mask) const { return (1 << subType) & mask; }
  inline int checkSubtypeAndLayerHiddenMasks(unsigned stmask, uint64_t lhmask) const
  {
    return ((stmask >> subType) & 1) && !((lhmask >> editLayerIdx) & 1);
  }

  inline unsigned getFlags() const { return flags; }
  inline void setFlags(unsigned t) { flags = t; }

  //! returns asset name for entity; may be very temporary!
  virtual const char *getObjAssetName() const = 0;

  //! called on enter/leave transformation mode with gizmo (implementation may apply perf optimizations)
  virtual void setGizmoTranformMode(bool enabled) {}

public:
  static IObjEntity *create(const DagorAsset &asset, bool virtual_ent = false);
  static IObjEntity *clone(IObjEntity *origin);

  static int registerSubTypeId(const char *subtype_str);

protected:
  int entityClass;
  unsigned subType : 6;
  unsigned editLayerIdx : 6;
  unsigned flags : 20;
};


class IObjEntityMgr
{
public:
  static constexpr unsigned HUID = 0xB689E91Fu; // IObjEntityMgr

  virtual bool canSupportEntityClass(int entity_class) const = 0;
  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) = 0;
  virtual IObjEntity *cloneEntity(IObjEntity *origin) = 0;
};
