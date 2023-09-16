//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaderMesh.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_DObject.h>


class TMatrix;
class IGenLoad;


// shader mesh that will be able to instantiate (for now, just wrapper for ShaderMesh)
class InstShaderMesh
{
public:
  DAG_DECLARE_NEW(midmem)

  InstShaderMesh() {}
  ~InstShaderMesh() { clearData(); }


  // patch mesh data after loading from dump
  void patchData(void *base, ShaderMatVdata &smvd) { mesh.patchData(base, smvd); }

  // rebase and clone data (useful for data copies)
  void rebaseAndClone(void *new_base, const void *old_base) { mesh.rebaseAndClone(new_base, old_base); }

  // explicit destructor
  void clearData() { mesh.clearData(); }


  void render() { mesh.render(); }
  void renderTrans() { mesh.render_trans(); }

  void gatherUsedTex(TextureIdSet &tex_id_list) const { mesh.gatherUsedTex(tex_id_list); }
  void gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const { mesh.gatherUsedMat(mat_list); }
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new) { return mesh.replaceTexture(tex_id_old, tex_id_new); }

  int calcTotalFaces() const { return mesh.calcTotalFaces(); }

  inline ShaderMesh *getMesh() { return &mesh; }
  inline const ShaderMesh *getMesh() const { return &mesh; }

protected:
  ShaderMesh mesh;
};


decl_dclass_and_id(InstShaderMeshResource, DObject, 0x68808EB7u)
public:
  /// creates resource by loading from stream
  static InstShaderMeshResource *loadResource(IGenLoad & crd, ShaderMatVdata & smvd, int res_sz = -1);

  /// clones resource
  virtual InstShaderMeshResource *clone() const;

  /// replaces texture id in materials used by mesh
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  /// return mesh
  inline InstShaderMesh *getMesh() { return &mesh; }
  inline const InstShaderMesh *getMesh() const { return &mesh; }

protected:
  int resSize;
  PATCHABLE_64BIT_PAD32(_resv);
  Ptr<ShaderMatVdata> smvdRef;

  InstShaderMesh mesh;

  // ctor/dtor
  InstShaderMeshResource() {} //-V730
  InstShaderMeshResource(const InstShaderMeshResource &from);

  // patches data after resource dump loading
  void patchData(int res_sz, ShaderMatVdata &smvd);

  static inline int dumpStartOfs() { return offsetof(InstShaderMeshResource, mesh); }
  void *dumpStartPtr() { return &mesh; }

end_dclass_decl();
