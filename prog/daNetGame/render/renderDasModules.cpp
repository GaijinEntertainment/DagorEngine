// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
void pull_render_das()
{
  NEED_MODULE(DagorTexture3DModule)
  NEED_MODULE(DagorResPtr)
  NEED_MODULE(DagorDriver3DModule)
  NEED_MODULE(DagorShaders)
  NEED_MODULE(PriorityShadervarModule)
  NEED_MODULE(DaSkiesModule)
  NEED_MODULE(FxModule)
  NEED_MODULE(ProjectiveDecalsModule)
  NEED_MODULE(SceneModule)
  NEED_MODULE(WorldRendererModule)
  NEED_MODULE(ClipmapDecalsModule)
  NEED_MODULE(DagorMaterials)
  NEED_MODULE(DagorStdGuiRender)
  NEED_MODULE(DaFgCoreModule)
  NEED_MODULE(ResourceSlotCoreModule)
  NEED_MODULE(HeightmapQueryManagerModule)
  NEED_MODULE(PortalRendererModule)
  NEED_MODULE(RenderLibsAllowedModule)
}
