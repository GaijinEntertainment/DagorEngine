// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>

void pull_client_das()
{
  NEED_MODULE(AuthModule)
  NEED_MODULE(SystemInfoModule)
  NEED_MODULE(ClientNetModule)
  NEED_MODULE(ForceFeedbackRumbleModule)
  NEED_MODULE(CameraShakerModule)
  NEED_MODULE(DngCameraModule)
  NEED_MODULE(DngCameraShakerModule)
}
