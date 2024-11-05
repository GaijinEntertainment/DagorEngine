// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace sndsys
{
typedef void (*SoundSuspendHandler)(bool suspend);
void registerIOSNotifications(SoundSuspendHandler handler);
} // namespace sndsys
