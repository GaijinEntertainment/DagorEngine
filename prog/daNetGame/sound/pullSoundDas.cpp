// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>

void pull_sound_das()
{
  NEED_MODULE(SoundEventModule)
  NEED_MODULE(SoundStreamModule)
  NEED_MODULE(SoundPropsModule)
}
