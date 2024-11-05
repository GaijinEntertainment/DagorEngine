// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if DAGOR_DBGLEVEL > 0

bool should_hide_ri_extra_object_with_id(int riex_id);

#else

// Will get inlined and any branches will get optimized away
__forceinline bool should_hide_ri_extra_object_with_id(int) { return false; }

#endif
