//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shSkinMesh.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <generic/dag_ptrTab.h>

//************************************************************************
//* load flags for shader resources
//************************************************************************
enum ShaderMeshResourceFlags
{
  // don't build shader mesh data, when loading
  // SRLOAD_NOBUILD = 0x01,

  // get textures from symbolic factory, they are just named ids to be replaced later
  SRLOAD_SYMTEX = 0x02,

  // read VB/IB data into sysmem (accessible via GlobalVertexData::getVBMem() and GlobalVertexData::getIBMem())
  SRLOAD_TO_SYSMEM = 0x04,

  // load materials without acquiring tex reference
  SRLOAD_NO_TEX_REF = 0x08,

  // load only vdata src (packed block) and don't load it ot vertex/index buffers
  SRLOAD_SRC_ONLY = 0x10,
};


decl_dclass_and_id(ShaderMeshResource, DObject, 0xD89B17A4)
public:
  /// creates resource by loading from stream
  static ShaderMeshResource *loadResource(IGenLoad & crd, ShaderMatVdata & smvd, int res_sz = -1);

  /// clones resource
  virtual ShaderMeshResource *clone() const;

  /// gathers all textures referenced by materials
  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  /// gathers all materials referenced in elements
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;

  /// replaces texture id in materials used by mesh
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  /// return mesh
  inline ShaderMesh *getMesh() { return &mesh; }
  inline const ShaderMesh *getMesh() const { return &mesh; }

  static inline int dumpStartOfs() { return offsetof(ShaderMeshResource, mesh); }
  void *dumpStartPtr() { return &mesh; }

protected:
  int resSize;
  PATCHABLE_64BIT_PAD32(_resv);
  Ptr<ShaderMatVdata> smvdRef;

  ShaderMesh mesh;

  // ctor/dtor
  ShaderMeshResource() {} //-V730
  ShaderMeshResource(const ShaderMeshResource &from);

  // patches data after resource dump loading
  void patchData(int res_sz, ShaderMatVdata &smvd);
end_dclass_decl();


decl_dclass_and_id(ShaderSkinnedMeshResource, DObject, 0x37db46e3)
public:
  /// creates resource by loading from stream
  static ShaderSkinnedMeshResource *loadResource(IGenLoad & crd, ShaderMatVdata & smvd, int res_sz = -1);

  /// clones resource
  virtual ShaderSkinnedMeshResource *clone() const;

  /// gathers all textures referenced by materials
  void gatherUsedTex(TextureIdSet & tex_id_list) const;
  /// gathers all materials referenced in elements
  void gatherUsedMat(Tab<ShaderMaterial *> & mat_list) const;

  /// replaces texture id in materials used by mesh
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new);

  /// return mesh
  inline ShaderSkinnedMesh *getMesh() { return &mesh; }
  inline const ShaderSkinnedMesh *getMesh() const { return &mesh; }

  static inline int dumpStartOfs() { return offsetof(ShaderSkinnedMeshResource, mesh); }
  void *dumpStartPtr() { return &mesh; }

protected:
  int resSize;
  PATCHABLE_64BIT_PAD32(_resv);
  Ptr<ShaderMatVdata> smvdRef;

  ShaderSkinnedMesh mesh;

  // ctor/dtor
  ShaderSkinnedMeshResource() {} //-V730
  ShaderSkinnedMeshResource(const ShaderSkinnedMeshResource &from);

  // patches data after resource dump loading
  void patchData(int res_sz, ShaderMatVdata &smvd);

end_dclass_decl();
