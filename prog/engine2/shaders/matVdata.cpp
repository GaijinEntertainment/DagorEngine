#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaders.h>
#include <3d/fileTexFactory.h>
#include "scriptSMat.h"

ShaderMatVdata::ShaderMatVdata(int tex_num, int mat_num, int vdata_num, int mvhdr_sz)
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

ShaderMatVdata *ShaderMatVdata::create(int tex_num, int mat_num, int vdata_num, int mvhdr_sz)
{
  ShaderMatVdata *smv = NULL;
  void *mem = memalloc(
    sizeof(ShaderMatVdata) + elem_size(smv->tex) * tex_num + elem_size(smv->mat) * mat_num + elem_size(smv->vdata) * vdata_num,
    midmem);
  smv = new (mem, _NEW_INPLACE) ShaderMatVdata(tex_num, mat_num, vdata_num, mvhdr_sz);
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

  ShaderMatVdata *smv = NULL;
  void *mem = memalloc(sizeof(ShaderMatVdata) + elem_size(src_smv->vdata) * src_smv->vdataFullCount, midmem);
  smv = new (mem, _NEW_INPLACE) ShaderMatVdata(0, 0, src_smv->vdataFullCount, src_smv->matVdataHdrSz);
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
