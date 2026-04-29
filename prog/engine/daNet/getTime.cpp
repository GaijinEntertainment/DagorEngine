// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/getTime.h>
#include <enet/enet.h>

namespace danet
{

// Note for Windows platform:
// QueryPerfomanceCounter() (and hence get_time_msec() that implemented via it) is considered unreliable
// See comments & code in:
// https://chromium.googlesource.com/chromium/src.git/+/28ec0e0c2425d480693e8d40510b8093cef1f89f/base/time/time_win.cc
// So we use enet wrapper here that uses reliable timeGetTime() instead
DaNetTime GetTime() { return enet_time_get(); }

} // namespace danet
