// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>

#include "render.h"

namespace drv3d_metal
{
  MTLRenderPipelineDescriptor* Program::pipelineStateDescriptor = NULL;

  Program::Program(Shader* vshdr, Shader* pshdr , VDecl* decl)
  {
    G_ASSERT(vshdr);
    if (vshdr->mesh_shader)
    {
      vshader = nullptr;
      mshader = vshdr->mesh_shader;
      ashader = vshdr->amplification_shader;
    }
    else
      vshader = vshdr;
    pshader = pshdr;
    cshader = NULL;

    vdecl = decl;
  }

  Program::Program(Shader* cshdr)
  {
    vshader = NULL;
    pshader = NULL;
    cshader = cshdr;

    csPipeline = render.shadersPreCache.compilePipeline(cshader->shader_hash);
  }

  void Program::release()
  {
    delete this;
  }
}
