// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mesh.h>
#include <3d/dag_materialData.h>
#include <util/dag_string.h>

class IGenSave;
struct Capsule;
class StaticGeometryMesh;
class Point3;
class CollisionResource;


class ICollisionDumpBuilder
{
public:
  // constraint: one instance cooks once; the add entries accumulate with no reset
  virtual void destroy() = 0;

  // optional primitives: a caller reaches one only with its supportMask bit set, so a builder that sets the bit overrides the entry
  virtual void addCapsule(Capsule & /*c*/, int /*obj_id*/, int /*obj_flags*/, const char * /*physmat*/) {}
  virtual void addSphere(const Point3 & /*c*/, float /*rad*/, int /*obj_id*/, int /*obj_flags*/, const char * /*pm*/) {}
  virtual void addBox(const TMatrix & /*box_tm*/, int /*obj_id*/, int /*obj_flags*/, const char * /*physmat*/) {}
  virtual void addConvexHull(Mesh & /*m*/, MaterialDataList * /*mat*/, TMatrix & /*wtm*/, int /*obj_id*/, int /*obj_flags*/,
    int /*physmat_id*/, bool /*pmid_force*/, StaticGeometryMesh * /*sgm*/ = NULL)
  {}

  // triangle mesh (must be supported)
  virtual void addMesh(Mesh &m, MaterialDataList *mat, const TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm = NULL) = 0;

  // true with no bytes written means the clip stream is empty
  virtual bool finishAndWrite(IGenSave &cwr) = 0;

  // The same cook, handed back as a resource the caller owns.
  // A cook that built nothing answers null or a resource with no nodes, and the builder has already said why, so a caller tests both;
  // the default is a builder with no in-process route.
  virtual CollisionResource *finishAndBuild() { return nullptr; }

  // The scene's water, as its own stream. Call it after finishAndWrite; false means no water.
  virtual bool finishAndWriteWater(IGenSave & /*cwr*/) { return false; }

public:
  enum
  {
    SUPPORT_BOX = 0x01,
    SUPPORT_SPHERE = 0x02,
    SUPPORT_CAPSULE = 0x04,
    SUPPORT_CONVEX = 0x08,
  };
  unsigned supportMask;

  ICollisionDumpBuilder() : supportMask(0) {}
};


// the editor's own collision, for the clip file and for the DAG collision it keeps in process.
// cook_name labels this cook's console lines
ICollisionDumpBuilder *create_editor_collision_builder(const char *cook_name);
// Writes a CollisionResource stream (the level-bin SCol block).
// physmat_path carries the water flag, so finishAndWrite refuses the cook when that blk cannot be read.
ICollisionDumpBuilder *create_static_collision_builder(const char *physmat_path, bool fail_on_jolt_degenerate);

static bool getPhysMatNameFromMatName(MaterialData *m, String &s)
{
  if (m && !m->matName.empty())
  {
    const char *adog = ::strrchr(m->matName, '@');
    if (adog)
    {
      s = (adog + 1);
      return true;
    }
  }

  return false;
}


static bool getPhysMatNameFromMatName(const char *name, String &s)
{
  const char *adog = ::strrchr(name, '@');
  if (adog)
  {
    s = (adog + 1);
    return true;
  }

  return false;
}
