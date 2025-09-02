// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_meshShaders.h>
#include <osApiWrappers/dag_localConv.h>
#include "shadersBinaryData.h"
#include "scriptSMat.h"
#include "scriptSElem.h"

MeshShaderElement *new_mesh_shader(const char *shader_name, const bool optional)
{
  const shaderbindump::ShaderClass *sc = shBinDump().findShaderClass(shader_name);
  if (!sc)
  {
    if (!optional)
      logerr("Mesh shader '%s' not found in bin dump", shader_name);
    return NULL;
  }

  if (dd_stricmp((const char *)sc->name, shader_name))
    DAG_FATAL("Mesh shader '%s' not found", shader_name);

  MaterialData m;
  ShaderMaterialProperties *smp = ShaderMaterialProperties::create(sc, m);
  ScriptedShaderMaterial *mat = ScriptedShaderMaterial::create(*smp);
  delete smp;

  ShaderElement *selem = mat ? mat->make_elem(true, "mesh") : nullptr;
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

  return new MeshShaderElement(mat, selem);
}

bool MeshShaderElement::dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z, bool set_states) const
{
  return elem->dispatchMesh(groups_x, groups_y, groups_z, set_states);
}

bool MeshShaderElement::dispatchThreads(uint32_t threads_x, uint32_t threads_y, uint32_t threads_z, bool set_states) const
{
  return elem->dispatchMesh(threads_x, threads_y, threads_z, set_states);
}

bool MeshShaderElement::dispatchIndirect(Sbuffer *args, uint32_t count, uint32_t offset, uint32_t stride, bool set_states) const
{
  return elem->dispatchMeshIndirect(args, count, offset, stride, set_states);
}

bool MeshShaderElement::dispatchIndirectCount(Sbuffer *args, uint32_t args_offset, uint32_t args_stride, Sbuffer *count,
  uint32_t count_offset, uint32_t max_count, bool set_states) const
{
  return elem->dispatchMeshIndirectCount(args, args_offset, args_stride, count, count_offset, max_count, set_states);
}
