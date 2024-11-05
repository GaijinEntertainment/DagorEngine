// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_capture.h>
#include <drv/3d/dag_commands.h>
#include <memory/dag_framemem.h>
#include <EASTL/string.h>

namespace d3d
{

bool start_capture(const char *name, const char *)
{
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  using WStringType = eastl::basic_string<wchar_t, framemem_allocator>;
  WStringType wname(WStringType::CtorConvert{}, name);
  return d3d::driver_command(Drv3dCommand::PIX_GPU_BEGIN_CAPTURE, (void *)0, const_cast<wchar_t *>(wname.c_str()));
}

void stop_capture()
{
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);
  d3d::driver_command(Drv3dCommand::PIX_GPU_END_CAPTURE);
}

} // namespace d3d