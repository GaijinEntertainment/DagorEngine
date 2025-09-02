// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/rendInst/riExtra.h>
#include <math/dag_TMatrix.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/debugCollisionVisualization.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/gpuObjects.h>


ecs::EntityId find_ri_extra_eid(rendinst::riex_handle_t) { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
bool replace_ri_extra_res(ecs::EntityId, const char *, bool, bool, bool) { G_ASSERT_RETURN(false, false); }
struct CollisionObject;
namespace gamephys
{
struct CollisionContactData;
}
namespace rendinst
{
uint32_t getRiGenExtraResCount() { G_ASSERT_RETURN(false, 0); }
bbox3f riex_get_lbb(int) { G_ASSERT_RETURN(false, bbox3f()); }
int getRIGenExtraResIdx(const char *) { G_ASSERT_RETURN(false, 0); }
TMatrix getRIGenMatrix(const RendInstDesc &) { G_ASSERT_RETURN(false, TMatrix::IDENT); }
int getRiGenExtraInstances(Tab<riex_handle_t> &, uint32_t) { G_ASSERT_RETURN(false, 0); }
int getRiGenExtraInstances(Tab<riex_handle_t> &, uint32_t, const bbox3f &) { G_ASSERT_RETURN(false, 0); }
int getRiGenExtraInstances(Tab<riex_handle_t> &, uint32_t, const BSphere3 &) { G_ASSERT_RETURN(false, 0); }
void gatherRIGenExtraCollidable(riex_collidable_t &, const BBox3 &, bool) { G_ASSERT(0); }
void gatherRIGenExtraCollidable(riex_collidable_t &, const TMatrix &, const BBox3 &, bool) { G_ASSERT(0); }
void gatherRIGenExtraCollidableMax(riex_collidable_t &, const BSphere3 &, float) { G_ASSERT(0); }
void drawDebugCollisions(DrawCollisionsFlags, mat44f_cref, const Point3 &, bool, float, float) { G_ASSERT(0); }
void doRIGenDamage(const BSphere3 &, unsigned, const Point3 &, bool) { G_ASSERT(0); }
bool isRIGenDestr(const RendInstDesc &) { G_ASSERT_RETURN(false, false); }
bool isRIGenPosInst(const RendInstDesc &) { G_ASSERT_RETURN(false, false); }
bool isRIExtraGenPosInst(uint32_t) { G_ASSERT_RETURN(false, false); }
bool updateRiExtraReqLod(uint32_t, unsigned) { G_ASSERT_RETURN(false, false); }
bool isDestroyedRIExtraFromNextRes(const RendInstDesc &) { G_ASSERT_RETURN(false, false); }
int getRIExtraNextResIdx(const int) { G_ASSERT_RETURN(false, -1); }
void setRiGenResMatId(uint32_t, int) { G_ASSERT(0); }
void setRiGenResHp(uint32_t, float) { G_ASSERT(0); }
void setRiGenResDestructionImpulse(uint32_t, float) { G_ASSERT(0); }
int addRIGenExtraResIdx(const char *, int, int, AddRIFlags) { G_ASSERT_RETURN(false, 0); }
void addRiGenExtraDebris(uint32_t, int) { G_ASSERT(0); }
void reloadRIExtraResources(const char *) { G_ASSERT(0); }
int cloneRIGenExtraResIdx(const char *, const char *) { G_ASSERT_RETURN(false, 0); }
bool delRIGenExtra(riex_handle_t) { G_ASSERT_RETURN(false, false); }
void foreachRIGenInBox(const BBox3 &, GatherRiTypeFlags, ForeachCB &) { G_ASSERT(0); }
uint32_t setMaxNumRiCollisionCb(uint32_t) { G_ASSERT_RETURN(false, 0); };
void testObjToRendInstIntersection(const BSphere3 &, RendInstCollisionCB &, GatherRiTypeFlags, const TraceMeshFaces *, PhysMat::MatID,
  bool)
{
  G_ASSERT(0);
}
void testObjToRendInstIntersection(const BBox3 &, RendInstCollisionCB &, GatherRiTypeFlags, const TraceMeshFaces *, PhysMat::MatID,
  bool)
{
  G_ASSERT(0);
}
void testObjToRendInstIntersection(const BBox3 &, const TMatrix &, RendInstCollisionCB &, GatherRiTypeFlags, const TraceMeshFaces *,
  PhysMat::MatID, bool)
{
  G_ASSERT(0);
}
void testObjToRendInstIntersection(const Capsule &, RendInstCollisionCB &, GatherRiTypeFlags, const TraceMeshFaces *, PhysMat::MatID,
  bool)
{
  G_ASSERT(0);
}

const CollisionResource *getRiGenCollisionResource(const RendInstDesc &) { G_ASSERT_RETURN(false, nullptr); }
void getRIGenExtra44(riex_handle_t, mat44f &) { G_ASSERT(0); }
const char *getRIGenResName(const RendInstDesc &) { G_ASSERT_RETURN(false, nullptr); }
const char *getRIGenDestrFxTemplateName(const RendInstDesc &) { G_ASSERT_RETURN(false, nullptr); }
const char *getRIGenDestrName(const RendInstDesc &) { G_ASSERT_RETURN(false, nullptr); }
Point4 getRIGenBSphere(const RendInstDesc &) { G_ASSERT_RETURN(false, {}); }
BBox3 getRIGenBBox(const RendInstDesc &) { G_ASSERT_RETURN(false, {}); }
BBox3 getRIGenFullBBox(const RendInstDesc &) { G_ASSERT_RETURN(false, {}); }
bool moveRIGenExtra43(riex_handle_t, mat43f_cref, bool, bool) { G_ASSERT_RETURN(false, false); }
void applyTiledScenesUpdateForRIGenExtra(int, int) { G_ASSERT(0); }
void get_ri_color_infos(eastl::fixed_function<sizeof(void *) * 4, void(E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name)>)
{
  G_ASSERT(0);
}


int getRIGenExtraPoolsCount() { G_ASSERT_RETURN(false, 0); }
CollisionResource *getRIGenExtraCollRes(int) { G_ASSERT_RETURN(false, nullptr); }
const bbox3f *getRIGenExtraCollBb(int) { G_ASSERT_RETURN(false, nullptr); }
CollisionInfo getRiGenDestrInfo(const RendInstDesc &desc) { G_ASSERT_RETURN(false, CollisionInfo(desc)); }
bool traceTransparencyRayRIGenNormalized(const Point3 &, const Point3 &, float, float, PhysMat::MatID, RendInstDesc *, int *, float *,
  bool)
{
  G_ASSERT_RETURN(false, false);
}
bool traceTransparencyRayRIGenNormalizedWithDist(const Point3 &, const Point3 &, float &, float, PhysMat::MatID, RendInstDesc *, int *,
  float *, bool)
{
  G_ASSERT_RETURN(false, false);
}
void setRiGenExtraHp(riex_handle_t, float) { G_ASSERT(0); }
float getHP(riex_handle_t) { G_ASSERT_RETURN(false, 0.f); }
float getInitialHP(riex_handle_t) { G_ASSERT_RETURN(false, 0.f); }
bool isRiGenExtraValid(riex_handle_t) { G_ASSERT_RETURN(false, false); }
const char *getRIGenExtraName(uint32_t) { G_ASSERT_RETURN(false, nullptr); }
void iterateRIExtraMap(eastl::fixed_function<sizeof(void *) * 3, void(int, const char *)>) { G_ASSERT(0); }
void setRIGenExtraImmortal(uint32_t, bool) { G_ASSERT(0); }
bool isRIGenExtraImmortal(uint32_t) { G_ASSERT_RETURN(false, false); }
bool isRIGenExtraWalls(uint32_t) { G_ASSERT_RETURN(false, false); }
bool isPaintFxOnHit(riex_handle_t) { G_ASSERT_RETURN(false, false); }
vec4f getRIGenExtraBSphere(riex_handle_t) { G_ASSERT_RETURN(false, vec4f()); }
float getRIGenExtraBsphRad(uint32_t) { G_ASSERT_RETURN(false, 0.0f); }
Point3 getRIGenExtraBsphPos(uint32_t) { G_ASSERT_RETURN(false, Point3()); }
Point4 getRIGenExtraBSphereByTM(uint32_t, const TMatrix &) { G_ASSERT_RETURN(false, Point4()); }
int getRIGenExtraParentForDestroyedRiIdx(uint32_t) { G_ASSERT_RETURN(false, -1); }
bool isRIGenExtraDestroyedPhysResExist(uint32_t) { G_ASSERT_RETURN(false, false); }
int getRIGenExtraDestroyedRiIdx(uint32_t) { G_ASSERT_RETURN(false, -1); }
void moveToOriginalScene(riex_handle_t) { G_ASSERT(0); }
void removeFromTiledScene(riex_handle_t) { G_ASSERT(0); }
bool removeRIGenExtraFromGrid(riex_handle_t) { G_ASSERT_RETURN(false, false); }
bool restoreRIGenExtraInGrid(riex_handle_t) { G_ASSERT_RETURN(false, false); }

const DataBlock *registerRIGenExtraConfig(const DataBlock *) { G_ASSERT_RETURN(false, nullptr); }

RendInstDesc::RendInstDesc(riex_handle_t) { G_ASSERT(0); }

bool RendInstDesc::isDynamicRiExtra() const { G_ASSERT_RETURN(false, false); }

riex_handle_t RendInstDesc::getRiExtraHandle() const { G_ASSERT_RETURN(false, RIEX_HANDLE_NULL); }

void gpuobjects::erase_inside_sphere(const Point3 &, const float) { G_ASSERT(0); }

void initRiSoundOccluders(const dag::ConstSpan<eastl::pair<const char *, const char *>> &,
  const dag::ConstSpan<eastl::pair<const char *, float>> &)
{
  G_ASSERT(0);
}
float debugGetSoundOcclusion(const char *, float def_value)
{
  G_ASSERT(0);
  return def_value;
}
bool test_collision_ri(const CollisionObject &, const BSphere3 &, Tab<gamephys::CollisionContactData> &, const TraceMeshFaces *, int)
{
  G_ASSERT(0);
  return false;
}

} // namespace rendinst
