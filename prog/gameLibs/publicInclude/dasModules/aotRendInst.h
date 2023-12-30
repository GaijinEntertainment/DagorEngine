//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotCollisionResource.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/debugCollisionVisualization.h>
#include <rendInst/gpuObjects.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <math/dag_TMatrix.h>
#include <dasModules/aotDagorMath.h>

DAS_BASE_BIND_ENUM(rendinst::DrawCollisionsFlag, DrawCollisionsFlags, RendInst, RendInstExtra, All, Wireframe, Opacity, Invisible,
  RendInstCanopy, PhysOnly, TraceOnly)
DAS_BIND_ENUM_CAST(rendinst::DrawCollisionsFlag)

MAKE_TYPE_FACTORY(RiExtraComponent, RiExtraComponent);
MAKE_TYPE_FACTORY(RendInstDesc, rendinst::RendInstDesc);
MAKE_TYPE_FACTORY(CollisionInfo, rendinst::CollisionInfo);

namespace bind_dascript
{
inline bool traceTransparencyRayRIGenNormalized(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold)
{
  return rendinst::traceTransparencyRayRIGenNormalized(pos, dir, mint, transparency_threshold);
}
inline bool traceTransparencyRayRIGenNormalizedEx(const Point3 &pos, const Point3 &dir, float &t, float transparency_threshold,
  rendinst::RendInstDesc &ri_desc, int &pmid, float &transparency, int ray_mat_id = -1, bool check_canopy = true)
{
  return rendinst::traceTransparencyRayRIGenNormalizedWithDist(pos, dir, t, transparency_threshold, ray_mat_id, &ri_desc, &pmid,
    &transparency, check_canopy);
}
inline bool traceTransparencyRayRIGenNormalized_with_mat_id(const Point3 &pos, const Point3 &dir, float mint,
  float transparency_threshold, PhysMat::MatID ray_mat)
{
  return rendinst::traceTransparencyRayRIGenNormalized(pos, dir, mint, transparency_threshold, ray_mat);
}
inline int get_rigen_extra_res_idx(const char *ri_res_name) { return rendinst::getRIGenExtraResIdx(ri_res_name ? ri_res_name : ""); }

inline int rendinst_addRIGenExtraResIdx(const char *ri_res_name)
{
  return rendinst::addRIGenExtraResIdx(ri_res_name ? ri_res_name : "", -1, -1,
    rendinst::AddRIFlag::UseShadow | rendinst::AddRIFlag::Dynamic);
}

inline void get_rigen_extra_matrix(rendinst::riex_handle_t ri_handle, TMatrix &tm)
{
  tm = rendinst::getRIGenMatrix(rendinst::RendInstDesc(ri_handle));
}

inline Point4 get_ri_gen_extra_bsphere(rendinst::riex_handle_t ri_handle)
{
  const vec4f bsph = rendinst::getRIGenExtraBSphere(ri_handle);
  const float *data = (const float *)&bsph;
  return Point4(data[0], data[1], data[2], abs(data[3])); // radius is negative sometimes
}

inline void gather_ri_gen_extra_collidable(const BBox3 &viewBB,
  const das::TBlock<void, const das::TTemporary<const das::TArray<rendinst::riex_handle_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  rendinst::riex_collidable_t handles;
  rendinst::gatherRIGenExtraCollidable(handles, viewBB, true /*read_lock*/);

  das::Array arr;
  arr.data = (char *)handles.data();
  arr.size = uint32_t(handles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void gather_ri_gen_extra_collidable_in_transformed_box(const TMatrix &transform, const BBox3 &viewBB,
  const das::TBlock<void, const das::TTemporary<const das::TArray<rendinst::riex_handle_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  rendinst::riex_collidable_t handles;
  rendinst::gatherRIGenExtraCollidable(handles, transform, viewBB, true /*read_lock*/);

  das::Array arr;
  arr.data = (char *)handles.data();
  arr.size = uint32_t(handles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void get_ri_gen_extra_instances(int res_idx,
  const das::TBlock<void, const das::TTemporary<const das::TArray<rendinst::riex_handle_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<rendinst::riex_handle_t> handles(framemem_ptr());
  rendinst::getRiGenExtraInstances(handles, res_idx);

  das::Array arr;
  arr.data = (char *)handles.data();
  arr.size = uint32_t(handles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void get_ri_gen_extra_instances_by_box(int res_idx, const bbox3f &box,
  const das::TBlock<void, const das::TTemporary<const das::TArray<rendinst::riex_handle_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<rendinst::riex_handle_t> handles(framemem_ptr());
  rendinst::getRiGenExtraInstances(handles, res_idx, box);

  das::Array arr;
  arr.data = (char *)handles.data();
  arr.size = uint32_t(handles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float at_time, bool create_destr,
  const das::TBlock<void, const rendinst::riex_handle_t> &riex_destr_cb, bool is_client,
  const das::TBlock<bool, const rendinst::riex_handle_t> &should_damage, das::Context *context, das::LineInfoArg *at)
{
  auto riex_destr_cb_func = [&](rendinst::riex_handle_t riex_handle) {
    vec4f arg = das::cast<rendinst::riex_handle_t>::from(riex_handle);
    context->invoke(riex_destr_cb, &arg, nullptr, at);
  };

  auto should_damage_func = [&](rendinst::riex_handle_t riex_handle) {
    vec4f arg = das::cast<rendinst::riex_handle_t>::from(riex_handle);
    vec4f res = context->invoke(should_damage, &arg, nullptr, at);
    return das::cast<bool>::to(res);
  };

  ::rendinstdestr::damage_ri_in_sphere(pos, rad, dmg_near_far, at_time, create_destr,
    /*effect_cb*/ nullptr, riex_destr_cb_func, is_client, should_damage_func);
}

inline void doRIGenDamage(const BSphere3 &sphere, unsigned frame_no, const Point3 &axis)
{
  rendinst::doRIGenDamage(sphere, frame_no, axis);
}

inline void doRendinstDamage(const BSphere3 &sphere, uint32_t frame_no, float at_time, bool is_client, bool create_destr,
  const Point3 &view_pos)
{
  rendinstdestr::doRendinstDamage(sphere, false, frame_no, nullptr, at_time, is_client, create_destr, view_pos, nullptr,
    rendinst::DestrOptionFlag::AddDestroyedRi);
}

inline void getRiGenDestrInfo(const rendinst::RendInstDesc &desc, rendinst::CollisionInfo &res)
{
  res = rendinst::getRiGenDestrInfo(desc);
}

inline int rendinst_cloneRIGenExtraResIdx(const char *source_res_name, const char *new_res_name)
{
  return rendinst::cloneRIGenExtraResIdx(source_res_name ? source_res_name : "", new_res_name ? new_res_name : "");
}

inline void rendinst_foreachRIGenInBox(const BBox3 &box, bool test_riextra_col,
  const das::TBlock<void, const rendinst::RendInstDesc, const TMatrix, bool> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[3];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      struct ForeachCB final : public rendinst::ForeachCB
      {
        vec4f *args = nullptr;
        das::Context *context = nullptr;
        das::SimNode *code = nullptr;

        void executeForTm(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &tm) override
        {
          args[0] = das::cast<const rendinst::RendInstDesc &>::from(ri_desc);
          args[1] = das::cast<const TMatrix &>::from(tm);
          args[2] = das::cast<bool>::from(true);
          code->eval(*context);
        }

        void executeForPos(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &pos) override
        {
          args[0] = das::cast<const rendinst::RendInstDesc &>::from(ri_desc);
          args[1] = das::cast<const TMatrix &>::from(pos);
          args[2] = das::cast<bool>::from(false);
          code->eval(*context);
        }
      } cb;
      cb.args = args;
      cb.context = context;
      cb.code = code;

      rendinst::foreachRIGenInBox(box,
        test_riextra_col ? rendinst::GatherRiTypeFlag::RiGenAndExtra : rendinst::GatherRiTypeFlag::RiGenOnly, cb);
    },
    at);
}

inline void rendinst_foreachTreeInBox(const bbox3f &bbox, const das::TBlock<void, const rendinst::CollisionInfo> &block,
  das::Context *context, das::LineInfoArg *at)
{
  Point3_vec4 bmin;
  Point3_vec4 bmax;
  v_st(&bmin.x, bbox.bmin);
  v_st(&bmax.x, bbox.bmax);

  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, (bmin + bmax) * 0.5);
  BBox3 localBox(bmin - tm.getcol(3), bmax - tm.getcol(3));

  vec4f args[1];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      struct ForeachTreeCB final : public rendinst::RendInstCollisionCB
      {
        vec4f *args = nullptr;
        das::Context *context = nullptr;
        das::SimNode *code = nullptr;

        virtual void addCollisionCheck(const rendinst::CollisionInfo & /*coll_info*/) {}

        virtual void addTreeCheck(const rendinst::CollisionInfo &coll_info)
        {
          args[0] = das::cast<const rendinst::CollisionInfo &>::from(coll_info);
          code->eval(*context);
        }
      } cb;
      cb.args = args;
      cb.context = context;
      cb.code = code;

      rendinst::testObjToRIGenIntersection(localBox, tm, cb, rendinst::GatherRiTypeFlag::RiGenOnly);
    },
    at);
}

inline void get_ri_color_infos(const das::TBlock<void, E3DCOLOR, E3DCOLOR, das::TTemporary<const char *>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  rendinst::get_ri_color_infos([&](E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name) {
    vec4f args[] = {das::cast<E3DCOLOR>::from(colorFrom), das::cast<E3DCOLOR>::from(colorTo), das::cast<const char *>::from(name)};
    context->invoke(block, args, nullptr, at);
  });
}

inline void iterate_riextra_map(const das::TBlock<void, int, das::TTemporary<const char *>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  rendinst::iterateRIExtraMap([&](int id, const char *name) {
    vec4f args[] = {das::cast<E3DCOLOR>::from(id), das::cast<const char *>::from(name)};
    context->invoke(block, args, nullptr, at);
  });
}

} // namespace bind_dascript
