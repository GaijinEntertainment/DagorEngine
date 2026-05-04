// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shStateBlk.h"
#include "shBindumpsPrivate.h"
#include "scriptSMat.h"
#include "scriptSElem.h"
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_critSec.h>
namespace shaders_internal
{
CritSecStorage blockCritSec;
static bool ccInited = false;
}; // namespace shaders_internal

void shaders_internal::init_stateblocks()
{
  if (ccInited)
    return;
  ::create_critical_section(blockCritSec, "shaderStBlockCritSec");
  ccInited = true;
}

void shaders_internal::lock_block_critsec()
{
  if (ccInited)
    ::enter_critical_section(blockCritSec);
}

void shaders_internal::unlock_block_critsec()
{
  if (ccInited)
    ::leave_critical_section(blockCritSec);
}

void shaders_internal::close_stateblocks()
{
  close_vprog();
  close_fshader();
  if (ccInited)
    ::destroy_critical_section(blockCritSec);
  ccInited = false;
}

// TODO: exchange might need to become interlocked in the future
void shaders_internal::close_fshader()
{
  iterate_all_shader_dumps(
    [](ScriptedShadersBinDumpOwner &sh_owner) {
      for (auto &idref : sh_owner.fshId)
        if (auto id = eastl::exchange(idref, BAD_FSHADER); id != BAD_FSHADER)
        {
          G_ASSERT(d3d::is_inited());
          d3d::delete_pixel_shader(id);
        }
    },
    false);
}

void shaders_internal::close_cshader()
{
  iterate_all_shader_dumps(
    [](ScriptedShadersBinDumpOwner &sh_owner) {
      for (auto &idref : sh_owner.cshId)
        if (auto id = eastl::exchange(idref, BAD_PROGRAM); id != BAD_PROGRAM)
        {
          G_ASSERT(d3d::is_inited());
          d3d::delete_program(id);
        }
    },
    false);
}

void shaders_internal::close_vprog()
{
  iterate_all_shader_dumps(
    [](ScriptedShadersBinDumpOwner &sh_owner) {
      for (auto &idref : sh_owner.vprId)
        if (auto id = eastl::exchange(idref, BAD_VPROG); id != BAD_VPROG)
        {
          G_ASSERT(d3d::is_inited());
          d3d::delete_vertex_shader(id);
        }
    },
    false);
}
