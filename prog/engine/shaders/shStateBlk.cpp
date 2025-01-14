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
  shader_mats.insert(mat);
}

void shaders_internal::unregister_material(ScriptedShaderMaterial *mat)
{
  BlockAutoLock block;
  shader_mats.erase(mat);
}

void shaders_internal::register_mat_elem(ScriptedShaderElement *elem)
{
  BlockAutoLock block;
  shader_mat_elems.insert(elem);
}

void shaders_internal::unregister_mat_elem(ScriptedShaderElement *elem)
{
  BlockAutoLock block;
  if (!shader_mat_elems.size())
  {
    // unregister skipped, materials has been released already
#if DAGOR_DBGLEVEL > 0
    debug_dump_stack(String(0, "unregister_mat_elem(%p) called after mat system was shutdown", elem));
#else
    debug("unregister_mat_elem(%p) called after mat system was shutdown", elem);
#endif
    return;
  }
  shader_mat_elems.erase(elem);
}

bool shaders_internal::reload_shaders_materials(ScriptedShadersBinDumpOwner &prev_sh_owner)
{
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
  // destroy shaders
  for (int i = 0; i < prev_sh_owner.vprId.size(); i++)
    if (prev_sh_owner.vprId[i] != BAD_VPROG)
      d3d::delete_vertex_shader(prev_sh_owner.vprId[i]);
  for (int i = 0; i < prev_sh_owner.fshId.size(); i++)
    if (prev_sh_owner.fshId[i] != BAD_FSHADER)
    {
      if (prev_sh_owner.fshId[i] & 0x10000000)
        d3d::delete_program(prev_sh_owner.fshId[i] & 0x0FFFFFFF);
      else
        d3d::delete_pixel_shader(prev_sh_owner.fshId[i]);
    }

  // copy global variables
  for (int i = 0; i < prev_sh.gvMap.size(); i++)
  {
    const char *name = (const char *)prev_sh.varMap[prev_sh.gvMap[i] & 0xFFFF];
    if (name[strlen(name) - 1] == ']')
      continue; // skip array auxiliary vars since they are not used as a real global shader vars
    int varId = VariableMap::getVariableId(name);
    const int prevGId = prev_sh.gvMap[i] >> 16;
    auto const &vl = prev_sh.globVars;
    switch (vl.getType(prevGId))
    {
      case SHVT_INT: ShaderGlobal::set_int(varId, vl.get<int>(prevGId)); break;
      case SHVT_INT4: ShaderGlobal::set_int4(varId, vl.get<IPoint4>(prevGId)); break;
      case SHVT_REAL: ShaderGlobal::set_real(varId, vl.get<real>(prevGId)); break;
      case SHVT_COLOR4: ShaderGlobal::set_color4(varId, vl.get<Color4>(prevGId)); break;
      case SHVT_TEXTURE: ShaderGlobal::set_texture(varId, vl.getTex(prevGId).texId); break;
      case SHVT_BUFFER: ShaderGlobal::set_buffer(varId, vl.getBuf(prevGId).bufId); break;
      case SHVT_SAMPLER: ShaderGlobal::set_sampler(varId, vl.get<d3d::SamplerHandle>(prevGId)); break;
      case SHVT_TLAS: ShaderGlobal::set_tlas(varId, vl.get<RaytraceTopAccelerationStructure *>(prevGId)); break;
    }

    // copy global intervals
    int id = shBinDumpOwner().globvarIndexMap[varId];
    if (id == SHADERVAR_IDX_ABSENT)
      continue;
    int iid = shBinDumpOwner().globVarIntervalIdx[id];
    int prevIid = prev_sh_owner.globVarIntervalIdx[prevGId];
    if (iid >= 0 && prevIid >= 0)
      shBinDumpOwner().globIntervalNormValues[iid] = prev_sh_owner.globIntervalNormValues[prevIid];
  }

  // renew shader materials
  ska::flat_hash_set<ScriptedShaderElement *> elemPtrList;
  ska::flat_hash_set<ScriptedShaderMaterial *> mats = shader_mats;

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

  for (auto &m : mats)
    if (m)
    {
      m->recreateMat(false /* delete_programs */);
      if (m->getElem())
        elemPtrList.insert(m->getElem());
    }

  ska::flat_hash_set<ScriptedShaderElement *> elems = shader_mat_elems;
  for (auto e : elems)
    if (e && elemPtrList.find(e) == elemPtrList.end())
    {
      debug("detached elem %p (of %d/%d)", e, shader_mat_elems.size(), elemPtrList.size());
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

  // reset textures from previous global vars
  for (int i = 0, e = prev_sh.gvMap.size(); i < e; i++)
  {
    const int prevGId = prev_sh.gvMap[i] >> 16;
    auto const &vl = prev_sh.globVars;
    auto type = vl.getType(prevGId);
    if (type == SHVT_TEXTURE)
      release_managed_tex(vl.getTex(prevGId).texId);
    else if (type == SHVT_BUFFER)
      release_managed_res(vl.getBuf(prevGId).bufId);
  }

  close_shader_block_stateblocks(false);
#if DAGOR_DBGLEVEL > 0
  shaderbindump::resetInvalidVariantMarks();
#endif
  DEBUG_CP();
  return true;
}

bool shaders_internal::reload_shaders_materials_on_driver_reset()
{
  BlockAutoLock block;
  rebuild_shaders_stateblocks();
  auto &shOwner = shBinDumpOwner();

  DEBUG_CP();
  // destroy shaders
  eastl::fill(shOwner.vprId.begin(), shOwner.vprId.end(), BAD_VPROG);
  eastl::fill(shOwner.fshId.begin(), shOwner.fshId.end(), BAD_FSHADER);

  auto elems = shader_mat_elems;
  for (auto e : elems)
    if (e)
      e->native().resetShaderPrograms(false /* delete_programs */);
  DEBUG_CP();
  return true;
}
