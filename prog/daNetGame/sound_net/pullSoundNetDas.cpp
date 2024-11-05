// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>

void pull_sound_net_das()
{
  NEED_MODULE(SoundSystemModule)
  NEED_MODULE(SoundHashModule)
  NEED_MODULE(SoundNetPropsModule)
}
