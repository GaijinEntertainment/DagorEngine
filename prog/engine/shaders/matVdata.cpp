// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaders.h>
#include <3d/fileTexFactory.h>
#include "scriptSMat.h"

ShaderMatVdata::ShaderMatVdata(int tex_num, int mat_num, int vdata_num, int mvhdr_sz, unsigned model_type)
{
  char *base = ((char *)this) + sizeof(ShaderMatVdata);

  tex.set((TEXTUREID *)base, tex_num);
  mem_set_ff(tex);

  base += data_size(tex);
  mat.set((ShaderMaterial **)base, mat_num);
  mem_set_0(mat);

  base += data_size(mat);
  vdata.set((GlobalVertexData *)base, vdata_num);
  mem_set_0(vdata);

  matVdataHdrSz = mvhdr_sz;
  vdataFullCount = vdata_num;
  lodsAreSplit = 0;
  modelType = model_type;
  G_ASSERTF(vdataFullCount == vdata_num, "vdataFullCount=%d vdata_num=%d", vdataFullCount, vdata_num);
}
ShaderMatVdata::~ShaderMatVdata()
{
  if (_resv != 'L')
    return;
  vdata.set(vdata.data(), vdataFullCount);
  for (int i = vdata.size() - 1; i >= 0; i--)
    vdata[i].free();
  for (int i = mat.size() - 1; i >= 0; i--)
    if (mat[i])
      mat[i]->delRef();
}

void ShaderMatVdata::getTexIdx(const ShaderMatVdata &other_smvd)
{
  G_ASSERT(other_smvd.tex.size() == tex.size());
  mem_copy_to(other_smvd.tex, tex.data());
}

void ShaderMatVdata::preloadTex()
{
  for (int i = tex.size() - 1; i >= 0; i--)
    if (tex[i] != BAD_TEXTUREID && get_managed_texture_refcount(tex[i]) > 0)
    {
      acquire_managed_tex(tex[i]);
      release_managed_tex(tex[i]);
    }
}

ShaderMatVdata *ShaderMatVdata::create(int tex_num, int mat_num, int vdata_num, int mvhdr_sz, unsigned model_type)
{
  const size_t size = sizeof(ShaderMatVdata) + //
                      sizeof(decltype(ShaderMatVdata::tex)::value_type) * tex_num +
                      sizeof(decltype(ShaderMatVdata::mat)::value_type) * mat_num +
                      sizeof(decltype(ShaderMatVdata::vdata)::value_type) * vdata_num;

  void *mem = memalloc(size, midmem);
  ShaderMatVdata *smv = new (mem, _NEW_INPLACE) ShaderMatVdata(tex_num, mat_num, vdata_num, mvhdr_sz, model_type);
  return smv;
}

void ShaderMatVdata::finalizeMatRefs()
{
  for (int i = 0; i < mat.size(); i++)
    if (mat[i]->getRefCount() > 1)
      mat[i]->native().setNonSharedRefCount(mat[i]->getRefCount());
}

ShaderMatVdata *ShaderMatVdata::make_tmp_copy(ShaderMatVdata *src_smv, int apply_skip_first_lods_cnt)
{
  if (!src_smv)
    return nullptr;

  const size_t size = sizeof(ShaderMatVdata) + //
                      sizeof(decltype(ShaderMatVdata::vdata)::value_type) * src_smv->vdataFullCount;
  void *mem = memalloc(size, midmem);
  ShaderMatVdata *smv =
    new (mem, _NEW_INPLACE) ShaderMatVdata(0, 0, src_smv->vdataFullCount, src_smv->matVdataHdrSz, src_smv->modelType);
  smv->_resv = 0;
  smv->lodsAreSplit = src_smv->lodsAreSplit;
  smv->matVdataSrcRef = src_smv->matVdataSrcRef;
  for (int i = 0; i < src_smv->vdataFullCount; i++)
    smv->vdata[i].copyDescFrom(src_smv->vdata.data()[i]);
  if (apply_skip_first_lods_cnt >= 0)
    smv->applyFirstLodsSkippedCount(apply_skip_first_lods_cnt);
  return smv;
}
void ShaderMatVdata::update_vdata_from_tmp_copy(ShaderMatVdata *dest, ShaderMatVdata *src)
{
  dest->vdata.set(dest->vdata.data(), src->vdata.size());
  mem_copy_to(src->vdata, dest->vdata.data());
  mem_set_0(src->vdata);
  dest->matVdataSrc = eastl::move(src->matVdataSrc);
}
