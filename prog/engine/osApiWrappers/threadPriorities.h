// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define NO_PRIORITIES           0
#define PRIORITY_ONLY_HIGHER    1
#define PRIORITY_ALL_PRIORITIES 2
#if _TARGET_C1

#elif _TARGET_C2

#else
// MS thread priority boost makes "priority only higher" option kind of useless - you still have at least 3 distinct priorities:
// normal, above normal, boosted above normal (highest).
#define HAS_THREAD_PRIORITY PRIORITY_ALL_PRIORITIES
#endif
