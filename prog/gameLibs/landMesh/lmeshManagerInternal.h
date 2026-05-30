// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_texMgr.h>                     // get_managed_texture_id, TEXTUREID
#include <shaders/dag_shaderMeshTexLoadCtrl.h> // IShaderMatVdataTexLoadCtrl
#include <util/dag_string.h>                   // String

// Shared by lmeshManager.cpp (dump / tile / vertical-cliff texture loading) and
// lmeshLandClasses.cpp (land-class detail-texture loading): resolve a managed texture id
// after running the shader-mat-vdata texture-name preprocessor.
static inline TEXTUREID get_managed_texture_id_pp(const char *name)
{
  String tmp_storage;
  return ::get_managed_texture_id(IShaderMatVdataTexLoadCtrl::preprocess_tex_name(name, tmp_storage));
}
