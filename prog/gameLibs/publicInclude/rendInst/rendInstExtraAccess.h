//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>
#include <rendInst/layerFlags.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>


// The functions in this file provide access to various properties of riExtra.
// WARNING: Despite their names, the functions below only work for riExtra!!!

class CollisionResource;

namespace rendinst
{

float get_riextra_destr_time_to_live(riex_handle_t);
float get_riextra_destr_default_time_to_live(riex_handle_t);
float get_riextra_destr_time_to_kinematic(riex_handle_t);
float get_riextra_destr_time_to_sink_underground(riex_handle_t);
bool get_riextra_immortality(riex_handle_t);

uint32_t get_riextra_instance_seed(riex_handle_t);
void set_riextra_instance_seed(riex_handle_t, int32_t data);

const mat43f &getRIGenExtra43(riex_handle_t id);
const mat43f &getRIGenExtra43(riex_handle_t id, uint32_t &seed);
void getRIGenExtra44(riex_handle_t id, mat44f &out_tm);

void getRIExtraCollInfo(riex_handle_t id, CollisionResource **out_collision, BSphere3 *out_bsphere);
const CollisionResource *getDestroyedRIExtraCollInfo(riex_handle_t handle);

void setRIGenExtraImmortal(uint32_t pool, bool value);
bool isRIGenExtraImmortal(uint32_t pool);
bool isRIGenExtraWalls(uint32_t pool);
float getRIGenExtraBsphRad(uint32_t pool);
Point3 getRIGenExtraBsphPos(uint32_t pool);
Point4 getRIGenExtraBSphereByTM(uint32_t pool, const TMatrix &tm);
int getRIGenExtraParentForDestroyedRiIdx(uint32_t pool);
bool isRIGenExtraDestroyedPhysResExist(uint32_t pool);
int getRIGenExtraDestroyedRiIdx(uint32_t pool);
vec4f getRIGenExtraBSphere(riex_handle_t id);
// special values: 0 (default HP with regen), -1 (default HP no regen), -2 (invincible)
void setRiGenExtraHp(riex_handle_t id, float hp);

float getInitialHP(riex_handle_t id);
float getHP(riex_handle_t id);
bool isPaintFxOnHit(riex_handle_t id);
bool isKillsNearEffects(riex_handle_t id);

const char *getRIGenExtraName(uint32_t res_idx);
int getRIGenExtraPreferrableLod(uint32_t res_idx, float squared_distance);
int getRIGenExtraPreferrableLod(uint32_t res_idx, float squared_distance, bool &over_max_lod, int &last_lod);
int getRIGenExtraPreferrableLodRawDistance(uint32_t res_idx, float squared_distance, bool &over_max_lod, int &last_lod);
bool isRIExtraGenPosInst(uint32_t res_idx);
bool updateRiExtraReqLod(uint32_t res_idx, unsigned lod);

bbox3f riex_get_lbb(int res_idx);
float riex_get_bsph_rad(int res_idx);

void setRiGenResMatId(uint32_t res_idx, int matId);
void setRiGenResHp(uint32_t res_idx, float hp);
void setRiGenResDestructionImpulse(uint32_t res_idx, float impulse);
bool hasRiLayer(int res_idx, LayerFlag layer);

vec4f getNodeBsphere(riex_handle_t handle, float &pool_rad);
void prefetchNode(riex_handle_t handle);
// hash is not filled out when the result is false!
bool isVisibleByLodEst(riex_handle_t id, vec4f view_pos, float dist_mul, const mat43f *&mat_out, vec3f &pos, uint32_t &hash);

float getCullDistSqMul();
struct AllRendinstExtraScenesLockImpl;
struct AllRendinstExtraScenesLock
{
  AllRendinstExtraScenesLock();
  ~AllRendinstExtraScenesLock();

private:
  AllRendinstExtraScenesLockImpl *ptr;
};
} // namespace rendinst
