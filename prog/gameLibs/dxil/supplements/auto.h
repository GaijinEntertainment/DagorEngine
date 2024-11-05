// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/**
 * This header tries to figure out which windows sdk version is
 * used and fills in the gaps to make DX12 compile with it.
 * This assumes that the sdk is win7 or later.
 */

#include <d3dcommon.h>

#if !defined(NTDDI_WIN8)
#endif

#if !defined(NTDDI_WINBLUE)
#include "winblue.h"
#endif

#if !defined(NTDDI_WINTHRESHOLD)
#include "winth.h"
#else
#include <d3d12shader.h>
#endif

#if !defined(NTDDI_WIN10_TH2)
#endif

#if !defined(NTDDI_WIN10_RS1)
#endif

#if !defined(NTDDI_WIN10_RS2)
#endif

#if !defined(NTDDI_WIN10_RS3)
#endif

#if !defined(NTDDI_WIN10_RS4)
#endif

#if !defined(NTDDI_WIN10_RS4)
#endif

#if !defined(NTDDI_WIN10_RS5)
#endif

#if !defined(NTDDI_WIN10_19H1)
#endif
