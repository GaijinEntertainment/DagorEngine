
#include <math/dag_TMatrix.h>

#include "render.h"

namespace drv3d_metal
{
  MTLRenderPipelineDescriptor* Program::pipelineStateDescriptor = NULL;

  Program::Program(Shader* vshdr, Shader* pshdr , VDecl* decl)
  {
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
