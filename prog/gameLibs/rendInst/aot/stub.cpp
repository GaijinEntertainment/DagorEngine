#include <ecs/rendInst/riExtra.h>
#include <math/dag_TMatrix.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/debugCollisionVisualization.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/gpuObjects.h>


ecs::EntityId find_ri_extra_eid(rendinst::riex_handle_t)
{
  G_ASSERT(0);
  return ecs::INVALID_ENTITY_ID;
}
bool replace_ri_extra_res(ecs::EntityId, const char *, bool, bool, bool)
{
  G_ASSERT(0);
  return false;
}

namespace rendinst
{
uint32_t getRiGenExtraResCount()
{
  G_ASSERT(0);
  return 0;
}
bbox3f riex_get_lbb(int)
{
  G_ASSERT(0);
  return bbox3f();
}
int getRIGenExtraResIdx(const char *)
{
  G_ASSERT(0);
  return 0;
}
TMatrix getRIGenMatrix(const RendInstDesc &)
{
  G_ASSERT(0);
  return TMatrix::IDENT;
}
int getRiGenExtraInstances(Tab<riex_handle_t> &, uint32_t)
{
  G_ASSERT(0);
  return 0;
}
int getRiGenExtraInstances(Tab<riex_handle_t> &, uint32_t, const bbox3f &)
{
  G_ASSERT(0);
  return 0;
}
void gatherRIGenExtraCollidable(riex_collidable_t &, const BBox3 &, bool) { G_ASSERT(0); }
void drawDebugCollisions(DrawCollisionsFlags, mat44f_cref, const Point3 &, bool, float, float) { G_ASSERT(0); }
void doRIGenDamage(const BSphere3 &, unsigned, ri_damage_effect_cb, const Point3 &, bool) { G_ASSERT(0); }
bool isRIGenDestr(const RendInstDesc &)
{
  G_ASSERT(0);
  return false;
}
bool isRIGenPosInst(const RendInstDesc &)
{
  G_ASSERT(0);
  return false;
}
bool isRIExtraGenPosInst(uint32_t)
{
  G_ASSERT(0);
  return false;
}
bool updateRiExtraReqLod(uint32_t, unsigned)
{
  G_ASSERT(0);
  return false;
}
bool isDestroyedRIExtraFromNextRes(const RendInstDesc &)
{
  G_ASSERT(0);
  return false;
}
int getRIExtraNextResIdx(const int)
{
  G_ASSERT(0);
  return -1;
}
void setRiGenResMatId(uint32_t, int) { G_ASSERT(0); }
void setRiGenResHp(uint32_t, float) { G_ASSERT(0); }
void setRiGenResDestructionImpulse(uint32_t, float) { G_ASSERT(0); }
int addRIGenExtraResIdx(const char *, int, int, AddRIFlags)
{
  G_ASSERT(0);
  return 0;
}
void addRiGenExtraDebris(uint32_t, int) { G_ASSERT(0); }
void reloadRIExtraResources(const char *) { G_ASSERT(0); }
int cloneRIGenExtraResIdx(const char *, const char *)
{
  G_ASSERT(0);
  return 0;
}
bool delRIGenExtra(riex_handle_t)
{
  G_ASSERT(0);
  return false;
}
void foreachRIGenInBox(const BBox3 &, GatherRiTypeFlags, ForeachCB &) { G_ASSERT(0); }
void testObjToRIGenIntersection(const BBox3 &, const TMatrix &, RendInstCollisionCB &, GatherRiTypeFlags, const TraceMeshFaces *,
  PhysMat::MatID, bool)
{
  G_ASSERT(0);
}
const CollisionResource *getRiGenCollisionResource(const RendInstDesc &)
{
  G_ASSERT(0);
  return nullptr;
}
void getRIGenExtra44(riex_handle_t, mat44f &) { G_ASSERT(0); }
const char *getRIGenResName(const RendInstDesc &)
{
  G_ASSERT(0);
  return nullptr;
}
const char *getRIGenDestrName(const RendInstDesc &)
{
  G_ASSERT(0);
  return nullptr;
}
BBox3 getRIGenBBox(const RendInstDesc &)
{
  G_ASSERT(0);
  return {};
}
BBox3 getRIGenFullBBox(const RendInstDesc &)
{
  G_ASSERT(0);
  return {};
}
bool moveRIGenExtra44(riex_handle_t, mat44f_cref, bool, bool)
{
  G_ASSERT(0);
  return false;
}
void applyTiledScenesUpdateForRIGenExtra(int, int) { G_ASSERT(0); }
void get_ri_color_infos(eastl::fixed_function<sizeof(void *) * 4, void(E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name)>)
{
  G_ASSERT(0);
}


int getRIGenExtraPoolsCount()
{
  G_ASSERT(0);
  return 0;
}
CollisionResource *getRIGenExtraCollRes(int)
{
  G_ASSERT(0);
  return nullptr;
}
const bbox3f *getRIGenExtraCollBb(int)
{
  G_ASSERT(0);
  return nullptr;
}
CollisionInfo getRiGenDestrInfo(const RendInstDesc &desc)
{
  G_ASSERT(0);
  return CollisionInfo(desc);
}
bool traceTransparencyRayRIGenNormalized(const Point3 &, const Point3 &, float, float, PhysMat::MatID, RendInstDesc *, int *, float *,
  bool)
{
  G_ASSERT(0);
  return false;
}
bool traceTransparencyRayRIGenNormalizedWithDist(const Point3 &, const Point3 &, float &, float, PhysMat::MatID, RendInstDesc *, int *,
  float *, bool)
{
  G_ASSERT(0);
  return false;
}
void setRiGenExtraHp(riex_handle_t, float) { G_ASSERT(0); }
float getHP(riex_handle_t)
{
  G_ASSERT(0);
  return 0.f;
}
float getInitialHP(riex_handle_t)
{
  G_ASSERT(0);
  return 0.f;
}
bool isRiGenExtraValid(riex_handle_t)
{
  G_ASSERT(0);
  return false;
}
const char *getRIGenExtraName(uint32_t)
{
  G_ASSERT(0);
  return nullptr;
}
void iterateRIExtraMap(eastl::fixed_function<sizeof(void *) * 3, void(int, const char *)>) { G_ASSERT(0); }
void setRIGenExtraImmortal(uint32_t, bool) { G_ASSERT(0); }
bool isRIGenExtraImmortal(uint32_t)
{
  G_ASSERT(0);
  return false;
}
bool isRIGenExtraWalls(uint32_t)
{
  G_ASSERT(0);
  return false;
}
bool isPaintFxOnHit(riex_handle_t)
{
  G_ASSERT(0);
  return false;
}
vec4f getRIGenExtraBSphere(riex_handle_t)
{
  G_ASSERT(0);
  return vec4f();
}
float getRIGenExtraBsphRad(uint32_t)
{
  G_ASSERT(0);
  return 0.0f;
}
Point3 getRIGenExtraBsphPos(uint32_t)
{
  G_ASSERT(0);
  return Point3();
}
Point4 getRIGenExtraBSphereByTM(uint32_t, const TMatrix &)
{
  G_ASSERT(0);
  return Point4();
}
int getRIGenExtraParentForDestroyedRiIdx(uint32_t)
{
  G_ASSERT(0);
  return -1;
}
bool isRIGenExtraDestroyedPhysResExist(uint32_t)
{
  G_ASSERT(0);
  return false;
}
int getRIGenExtraDestroyedRiIdx(uint32_t)
{
  G_ASSERT(0);
  return -1;
}
void moveToOriginalScene(riex_handle_t) { G_ASSERT(0); }


const DataBlock *registerRIGenExtraConfig(const DataBlock *)
{
  G_ASSERT(0);
  return nullptr;
}

RendInstDesc::RendInstDesc(riex_handle_t) { G_ASSERT(0); }

riex_handle_t RendInstDesc::getRiExtraHandle() const
{
  G_ASSERT(0);
  return RIEX_HANDLE_NULL;
}

void gpuobjects::erase_inside_sphere(const Point3 &, const float) { G_ASSERT(0); }


} // namespace rendinst
