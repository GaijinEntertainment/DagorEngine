// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_computeShaders.h>
#include <osApiWrappers/dag_localConv.h>
#include "shadersBinaryData.h"
#include "scriptSMat.h"
#include "scriptSElem.h"

ComputeShaderElement *new_compute_shader(const char *shader_name, const bool optional)
{
  const shaderbindump::ShaderClass *sc = shBinDump().findShaderClass(shader_name);
  if (!sc)
  {
    if (!optional)
      logerr("Compute shader '%s' not found in bin dump", shader_name);
    return NULL;
  }

  if (dd_stricmp((const char *)sc->name, shader_name))
    DAG_FATAL("Compute shader '%s' not found", shader_name);

  MaterialData m;
  ShaderMaterialProperties *smp = ShaderMaterialProperties::create(sc, m);
  ScriptedShaderMaterial *mat = ScriptedShaderMaterial::create(*smp);
  delete smp;

  ShaderElement *selem = mat ? mat->make_elem(true, "compute") : nullptr;
  if (!selem)
  {
    if (!optional)
      logerr("Failed to make shader element for '%s'", shader_name);
    if (mat)
    {
      // Dirty hack here. It is possible that mat has a refcount of 0 here, and delRef will set it to -1, not deleting it.
      mat->addRef();
      mat->delRef();
    }
    return NULL;
  }

  return new ComputeShaderElement(mat, selem);
}

ComputeShaderElement::ComputeShaderElement(ScriptedShaderMaterial *ssm, ShaderElement *selem)
{
  mat = ssm;
  mat->addRef();
  if (!selem)
    selem = mat->make_elem(true, "compute");
  G_ASSERTF(selem, "invalid elem for compute shader %s", mat->getShaderClassName());
  elem = &selem->native();
  elem->addRef();
  elem->stageDest = STAGE_CS;
}
ComputeShaderElement::~ComputeShaderElement()
{
  elem->delRef();
  elem = NULL;
  mat->delRef();
  mat = NULL;
}

const char *ComputeShaderElement::getShaderClassName() const { return mat->getShaderClassName(); }
bool ComputeShaderElement::set_int_param(const int var_id, const int v) { return mat->set_int_param(var_id, v); }
bool ComputeShaderElement::set_real_param(const int var_id, const real v) { return mat->set_real_param(var_id, v); }
bool ComputeShaderElement::set_color4_param(const int var_id, const struct Color4 &v) { return mat->set_color4_param(var_id, v); }
bool ComputeShaderElement::set_texture_param(const int var_id, const TEXTUREID v) { return mat->set_texture_param(var_id, v); }
bool ComputeShaderElement::set_sampler_param(const int var_id, d3d::SamplerHandle v) { return mat->set_sampler_param(var_id, v); }
bool ComputeShaderElement::hasVariable(const int var_id) const { return mat->hasVariable(var_id); }
bool ComputeShaderElement::getColor4Variable(const int var_id, Color4 &v) const { return mat->getColor4Variable(var_id, v); }
bool ComputeShaderElement::getRealVariable(const int var_id, real &v) const { return mat->getRealVariable(var_id, v); }
bool ComputeShaderElement::getIntVariable(const int var_id, int &v) const { return mat->getIntVariable(var_id, v); }
bool ComputeShaderElement::getTextureVariable(const int var_id, TEXTUREID &v) const { return mat->getTextureVariable(var_id, v); }
bool ComputeShaderElement::getSamplerVariable(const int var_id, d3d::SamplerHandle &v) const
{
  return mat->getSamplerVariable(var_id, v);
}

bool ComputeShaderElement::setStates() const { return elem->setStatesDispatch(); }
eastl::array<uint16_t, 3> ComputeShaderElement::getThreadGroupSizes() const { return elem->getThreadGroupSizes(); }
bool ComputeShaderElement::dispatch(int tgx, int tgy, int tgz, GpuPipeline gpu_pipeline, bool set_states) const
{
  return elem->dispatchCompute(tgx, tgy, tgz, gpu_pipeline, set_states);
}
bool ComputeShaderElement::dispatchThreads(int threads_x, int threads_y, int threads_z, GpuPipeline gpu_pipeline,
  bool set_states) const
{
  return elem->dispatchComputeThreads(threads_x, threads_y, threads_z, gpu_pipeline, set_states);
}
uint32_t ComputeShaderElement::getWaveSize() const { return elem->getWaveSize(); }
bool ComputeShaderElement::dispatch_indirect(Sbuffer *args, int ofs, GpuPipeline gpu_pipeline, bool set_states) const
{
  return elem->dispatchComputeIndirect(args, ofs, gpu_pipeline, set_states);
}
