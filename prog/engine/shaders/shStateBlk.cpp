// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shStateBlk.h"
#include "scriptSMat.h"
#include "scriptSElem.h"
#include <shaders/shUtils.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texIdSet.h>
#include <debug/dag_debug.h>
#include "../lib3d/texMgrData.h"

#define debug(...) logmessage(_MAKE4C('SHST'), __VA_ARGS__)

bool shaders_internal::shader_reload_allowed = false;
int shaders_internal::nan_counter = 0;
int shaders_internal::shader_pad_for_reload = 1024;

void shaders_internal::register_material(ScriptedShaderMaterial *mat)
{
  BlockAutoLock block;
  get_shaders_dump_owner_mut(mat->props.dumpHandle).shaderMats.insert(mat);
}

void shaders_internal::unregister_material(ScriptedShaderMaterial *mat)
{
  BlockAutoLock block;
  get_shaders_dump_owner_mut(mat->props.dumpHandle).shaderMats.erase(mat);
}

void shaders_internal::register_mat_elem(ScriptedShaderElement *elem)
{
  BlockAutoLock block;
  get_shaders_dump_owner_mut(elem->dumpHandle).shaderMatElems.insert(elem);
}

void shaders_internal::unregister_mat_elem(ScriptedShaderElement *elem)
{
  BlockAutoLock block;
  auto &owner = get_shaders_dump_owner_mut(elem->dumpHandle);
  if (!owner.shaderMatElems.size())
  {
    // unregister skipped, materials has been released already
#if DAGOR_DBGLEVEL > 0
    debug_dump_stack(String(0, "unregister_mat_elem(%p) called after mat system was shutdown", elem));
#else
    debug("unregister_mat_elem(%p) called after mat system was shutdown", elem);
#endif
    return;
  }
  owner.shaderMatElems.erase(elem);
}

void shaders_internal::cleanup_shaders_on_reload(ScriptedShadersBinDumpOwner &prev_sh_owner)
{
  // destroy shaders
  for (auto &id : prev_sh_owner.vprId)
  {
    if (id != BAD_VPROG)
      d3d::delete_vertex_shader(id);
    id = BAD_VPROG;
  }
  for (auto &id : prev_sh_owner.fshId)
  {
    if (id != BAD_FSHADER)
      d3d::delete_pixel_shader(id);
    id = BAD_FSHADER;
  }
  for (auto &id : prev_sh_owner.cshId)
  {
    if (id != BAD_PROGRAM)
      d3d::delete_program(id);
    id = BAD_PROGRAM;
  }
}

bool shaders_internal::reload_shaders_materials( //
  ScriptedShadersBinDumpOwner &new_sh_owner,     //
  ScriptedShadersGlobalData *new_gdata,          //
  ScriptedShadersBinDumpOwner &prev_sh_owner,    //
  ScriptedShadersGlobalData *prev_gdata)
{
  G_ASSERT((new_gdata && prev_gdata) || (!new_gdata && !prev_gdata));
  bool const reloadGlobalData = new_gdata && prev_gdata;

  ScriptedShadersBinDump &prev_sh = *prev_sh_owner.getDump();
  if (!prev_sh.classes.size())
    return true;
  if (!shader_reload_allowed)
  {
    logerr("shaders reload prohibited");
    return false;
  }

  BlockAutoLock block;

  DEBUG_CP();

  cleanup_shaders_on_reload(prev_sh_owner);

  if (reloadGlobalData)
  {
    auto const *dump_v4 = new_sh_owner.getDumpV4();
    // copy global variables
    for (int i = 0; i < prev_sh.gvMap.size(); i++)
    {
      char const *name = (char const *)prev_sh.varMap[prev_sh.gvMap[i] & 0xFFFF];
      if (name[strlen(name) - 1] == ']')
        continue; // skip array auxiliary vars since they are not used as a real global shader vars
      int const varNid = VariableMap::getVariableId(name);
      int const newInternalVarId = new_gdata->globvarIndexMap[varNid];
      if (newInternalVarId == SHADERVAR_IDX_ABSENT || newInternalVarId == SHADERVAR_IDX_INVALID)
        continue;
      int const prevInternalVarId = prev_sh.gvMap[i] >> 16;
      if (dump_v4 && dump_v4->globVarsMetadata[newInternalVarId].isLiteral)
        continue;
      auto const &state = prev_gdata->globVarsState;
      auto const &vars = prev_sh.globVars;

      switch (vars.getType(prevInternalVarId))
      {
        case SHVT_INT: ShaderGlobal::set_int(varNid, state.get<int>(prevInternalVarId)); break;
        case SHVT_INT4: ShaderGlobal::set_int4(varNid, state.get<IPoint4>(prevInternalVarId)); break;
        case SHVT_REAL: ShaderGlobal::set_float(varNid, state.get<real>(prevInternalVarId)); break;
        case SHVT_COLOR4: ShaderGlobal::set_float4(varNid, state.get<Color4>(prevInternalVarId)); break;
        case SHVT_TEXTURE:
        {
          const auto &tex = state.get<shaders_internal::Tex>(prevInternalVarId);

          if (tex.isTextureManaged())
          {
            ShaderGlobal::set_texture(varNid, tex.texId);
          }
          else
          {
            ShaderGlobal::set_texture(varNid, tex.tex);
          }
          break;
        };
        case SHVT_BUFFER:
        {
          const auto &buf = state.get<shaders_internal::Buf>(prevInternalVarId);
          if (buf.isBufferManaged())
          {
            ShaderGlobal::set_buffer(varNid, buf.bufId);
          }
          else
          {
            ShaderGlobal::set_buffer(varNid, buf.buf);
          }
          break;
        }
        case SHVT_SAMPLER: ShaderGlobal::set_sampler(varNid, state.get<d3d::SamplerHandle>(prevInternalVarId)); break;
        case SHVT_TLAS: ShaderGlobal::set_tlas(varNid, state.get<RaytraceTopAccelerationStructure *>(prevInternalVarId)); break;
      }

      // copy global intervals
      int id = new_gdata->globvarIndexMap[varNid];
      if (id == SHADERVAR_IDX_ABSENT)
        continue;
      int iid = new_gdata->globVarIntervalIdx[id];
      int prevIid = prev_gdata->globVarIntervalIdx[prevInternalVarId];
      if (iid >= 0 && prevIid >= 0)
        new_gdata->globIntervalNormValues[iid] = prev_gdata->globIntervalNormValues[prevIid];
    }
  }

  // renew shader materials
  ska::flat_hash_set<ScriptedShaderElement *> elemPtrList;

  Tab<unsigned> texIdMap(tmpmem);
  unsigned max_idx = get_max_managed_texture_id().index() + 1;
  texIdMap.resize((max_idx + 31) / 32);
  mem_set_0(texIdMap);

  for (D3DRESID i = first_managed_d3dres(1); i != BAD_D3DRESID; i = next_managed_d3dres(i, 1))
  {
    unsigned idx = i.index();
    texIdMap[idx / 32] |= 1u << (idx % 32);
    // debug("0x%x: %s %d", i, get_managed_res_name(i), get_managed_res_refcount(i));

    // don't call acquire_managed_res(i) here to avoid calling createTexture() for broken tex
    texmgr_internal::RMGR.incRefCount(idx);
    max_idx = idx + 1;
  }

  for (auto &m : prev_sh_owner.shaderMats)
    if (m)
    {
      m->recreateMat(false /* delete_programs */);
      if (m->getElem())
        elemPtrList.insert(m->getElem());
    }

  for (auto e : prev_sh_owner.shaderMatElems)
    if (e && elemPtrList.find(e) == elemPtrList.end())
    {
      debug("detached elem %p (of %d/%d)", e, prev_sh_owner.shaderMatElems.size(), elemPtrList.size());
      e->native().detachElem();
    }
    else if (e)
      e->acquireTexRefs();

  for (unsigned idx = 0; idx < max_idx; idx++)
    if (texIdMap[idx / 32] & (1u << (idx % 32)))
    {
      D3DRESID rid = D3DRESID::fromIndex(idx);
      release_managed_res(rid);
      // debug("0x%x: %s %d", rid, get_managed_texture_name(rid), get_managed_texture_refcount(rid));
    }

  new_sh_owner.shaderMats = eastl::move(prev_sh_owner.shaderMats);
  new_sh_owner.shaderMatElems = eastl::move(prev_sh_owner.shaderMatElems);

  if (reloadGlobalData)
  {
    // reset textures from previous global vars
    for (int i = 0, e = prev_sh.gvMap.size(); i < e; i++)
    {
      const int prevGId = prev_sh.gvMap[i] >> 16;
      auto const &state = prev_gdata->globVarsState;
      auto const &vl = prev_sh.globVars;
      auto type = vl.getType(prevGId);
      if (type == SHVT_TEXTURE)
      {
        const auto &tex = state.get<shaders_internal::Tex>(prevGId);
        if (tex.isTextureManaged())
        {
          release_managed_tex(state.get<shaders_internal::Tex>(prevGId).texId);
        }
      }
      else if (type == SHVT_BUFFER)
      {
        const auto &buf = state.get<shaders_internal::Buf>(prevGId);
        if (buf.isBufferManaged())
        {
          release_managed_res(buf.bufId);
        }
      }
    }
  }

#if DAGOR_DBGLEVEL > 0
  shaderbindump::resetInvalidVariantMarks(new_sh_owner.selfHandle);
#endif
  DEBUG_CP();
  return true;
}

bool shaders_internal::reload_shaders_materials_on_driver_reset()
{
  BlockAutoLock block;
  rebuild_shaders_stateblocks();

  return true;
}
