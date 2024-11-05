// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/fileTexFactory.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texPackMgr2.h>
#include <osApiWrappers/dag_urlLike.h>
#include <util/fnameMap.h>
#include "texMgrData.h"
#include <debug/dag_log.h>
#include <EASTL/fixed_string.h>
#include <memory/dag_framemem.h>

#include <debug/dag_debug.h>

FileTextureFactory FileTextureFactory::self;


FileTextureFactory::FileTextureFactory() : missingTexId(BAD_TEXTUREID)
{
  missingTex = NULL;
  useMissingTex = false;
}

void FileTextureFactory::setMissingTexture(const char *tex_name, bool use_missing_tex)
{

  if (missingTexId != BAD_TEXTUREID)
    ::release_managed_tex(missingTexId);
  if (useMissingTex)
  {
    missingTexId = ::add_managed_texture(tex_name);
    missingTex = NULL;
  }
  useMissingTex = use_missing_tex;
}

void FileTextureFactory::setMissingTextureUsage(bool use) { useMissingTex = use; }

TEXTUREID FileTextureFactory::getReplaceBadTexId()
{
  if (!useMissingTex)
    return BAD_TEXTUREID;

  if (missingTexId != BAD_TEXTUREID)
    return missingTexId;

  missingTexId = ::add_managed_texture("tex/missing");
  return missingTexId;
}

BaseTexture *FileTextureFactory::createTexture(TEXTUREID id)
{
  // Creating a copy of this texture is a workaround to an issue:
  // the texture manager can invalidate any returned texture name pointers
  // when it optimizes the memory layout of stored texture names.
  // Creating a copy of this texture makes it very unlikely that it would cause a crash it this function,
  // which has been happening previously in WTM
  eastl::fixed_string<char, 128, true, framemem_allocator> textureName = get_managed_texture_name(id);
  if (textureName.empty())
    return NULL;

  BaseTexture *tex = create_texture(textureName.c_str(), TEXCF_RGB | TEXCF_ABEST, 0, false);

  if (!tex)
  {
    if (!useMissingTex)
    {
      logmessage(df_is_fname_url_like(textureName.c_str()) ? LOGLEVEL_WARN : LOGLEVEL_ERR, "missing texture '%s'",
        textureName.c_str());
      return NULL;
    }
    texmgr_internal::acquire_texmgr_lock();
    // add "missing" texture id if not added yet
    if (missingTexId == BAD_TEXTUREID)
      missingTexId = ::add_managed_texture("tex/missing");

    // return "missing" texture, unless it's missing
    texmgr_internal::release_texmgr_lock();
    if (id == missingTexId)
      return NULL;

    logmessage(df_is_fname_url_like(textureName.c_str()) ? LOGLEVEL_WARN : LOGLEVEL_ERR, "missing texture '%s'", textureName.c_str());

    BaseTexture *t = ::acquire_managed_tex(missingTexId);
    texmgr_internal::acquire_texmgr_lock();
    if (!missingTex)
    {
      missingTex = t;
      t = NULL;
    }
    texmgr_internal::release_texmgr_lock();
    del_d3dres(t);
    return missingTex;
  }

  return tex;
}


// Release created texture
void FileTextureFactory::releaseTexture(BaseTexture *texture, TEXTUREID id)
{
  if (texture == missingTex && id != missingTexId)
  {
    // it's replacement for missing texture, free missingTex instead
    ::release_managed_tex(missingTexId);
    return;
  }

  using texmgr_internal::RMGR;
  int idx = RMGR.toIndex(id);
  if (RMGR.texDesc[idx].dim.stubIdx >= 0)
  {
    RMGR.setD3dRes(idx, nullptr);
    if (RMGR.resQS[idx].isReading())
      RMGR.cancelReading(idx);
    RMGR.markUpdated(idx, 0);
  }

  if (texture)
    texture->destroy();

  if (texture == missingTex)
    missingTex = NULL;
}

bool FileTextureFactory::scheduleTexLoading(TEXTUREID tid, TexQL req_ql)
{
  using texmgr_internal::RMGR;
  int idx = RMGR.toIndex(tid);
  if (idx < 0 || req_ql >= TQL__COUNT)
    return false;

  G_ASSERTF_RETURN(RMGR.texDesc[idx].dim.stubIdx >= 0, false, "tid=0x%x stubIdx=%d %s", tid, RMGR.texDesc[idx].dim.stubIdx,
    RMGR.getName(idx));

  auto *tex_desc_p = RMGR.texDesc[idx].packRecIdx;
  G_ASSERTF_RETURN(tex_desc_p[TQL_base].pack == 0x7FFF, false, "tid=0x%x bq_pack=%d", tid, tex_desc_p[TQL_base].pack);
  G_ASSERTF_RETURN(tex_desc_p[req_ql].pack >= 0, false, "tid=0x%x  ql=%d from=0x%x:%d", tid, req_ql, tex_desc_p[req_ql].pack,
    tex_desc_p[req_ql].rec);

  if (req_ql == TQL_base && tex_desc_p[TQL_base].rec == tex_desc_p[TQL_high].rec)
  {
    unsigned max_lev = min<unsigned>(RMGR.resQS[idx].getMaxLev(), RMGR.resQS[idx].getQLev());
    unsigned rd_lev = min<unsigned>(RMGR.resQS[idx].getMaxReqLev(), max_lev);
    if (RMGR.resQS[idx].getRdLev() != rd_lev)
      RMGR.resQS[idx].setRdLev(rd_lev);
  }
  // debug("enqueue 0x%d:%d req_ql=%d maxReqLev=%d", tex_desc_p[req_ql].pack, tex_desc_p[req_ql].rec, req_ql,
  // RMGR.resQS[idx].getMaxReqLev());
  ddsx::enqueue_ddsx_load(RMGR.baseTexture(idx), tid, dagor_fname_map_resolve_id(tex_desc_p[req_ql].rec), dgs_tex_quality,
    RMGR.pairedBaseTexId[idx]);
  return true;
}
