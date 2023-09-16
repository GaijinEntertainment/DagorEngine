#include "shStateBlk.h"
#include "scriptSMat.h"
#include "scriptSElem.h"
#include <3d/dag_drv3d.h>
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
  close_shader_block_stateblocks(true);
  if (ccInited)
    ::destroy_critical_section(blockCritSec);
  ccInited = false;
}


void shaders_internal::close_fshader()
{
  for (int i = 0; i < shBinDump().fshId.size(); ++i)
    if (shBinDump().fshId[i] != BAD_FSHADER)
    {
      G_ASSERT(d3d::is_inited());
      if (shBinDump().fshId[i] & 0x10000000)
        d3d::delete_program(shBinDump().fshId[i] & 0x0FFFFFFF);
      else
        d3d::delete_pixel_shader(shBinDump().fshId[i]);
      shBinDumpRW().fshId[i] = BAD_FSHADER;
    }
}


void shaders_internal::close_vprog()
{
  for (int i = 0; i < shBinDump().vprId.size(); ++i)
    if (shBinDump().vprId[i] != BAD_VPROG)
    {
      G_ASSERT(d3d::is_inited());
      d3d::delete_vertex_shader(shBinDump().vprId[i]);
      shBinDumpRW().vprId[i] = BAD_VPROG;
    }
}
