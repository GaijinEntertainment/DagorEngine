// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "drv_log_defs.h"

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
    {
      vshader = vshdr;

      name = String(0, "%s @ %s", vshader->name.c_str(), pshdr ? pshdr->name.c_str() : "");
    }
    pshader = pshdr;
    cshader = NULL;

    vdecl = decl;
  }

  Program::Program(Shader* cshdr)
  {
    G_ASSERT(cshdr);
    vshader = NULL;
    pshader = NULL;
    cshader = cshdr;

    csPipeline = render.shadersPreCache.compilePipeline(cshader->shader_hash);
    if (csPipeline && csPipeline.maxTotalThreadsPerThreadgroup < (cshader->tgrsz_x*cshader->tgrsz_y*cshader->tgrsz_z))
      D3D_ERROR("Shader %s threadgroup size %d is greater than max %d", cshdr->name.c_str(), (cshader->tgrsz_x*cshader->tgrsz_y*cshader->tgrsz_z), int(csPipeline.maxTotalThreadsPerThreadgroup));
    name = cshdr->name;
  }

  void Program::release()
  {
    delete this;
  }
}
