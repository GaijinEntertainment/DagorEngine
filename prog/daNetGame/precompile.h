// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//
// Precompiled headers (this file implicitly included in all modules of current project)
//
// Good candidates for inclusion should match these rules:
// * Rarely changes (good rule of thumb - once per month or rarer)
// * Used by most of code
// * Not too big
// Bad examples:
//  windows.h - used by few, too big
//  daECS/core/entityManager.h - changes too frequently (at least at the time of writing)
//  boost/include/* - used by few, too big
// See also
//  https://docs.microsoft.com/en-us/cpp/build/creating-precompiled-header-files
//  https://www.viva64.com/en/b/0265/
//  --- force recompile counter: 144

#include <util/dag_safeArg.h> // required to force eastl::basic_string deterministic declaration

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cerrno>
#include <malloc.h>
#if _TARGET_SIMD_SSE
#include <emmintrin.h>
#endif

#include <EASTL/type_traits.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/tuple.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/algorithm.h>
#include <EASTL/bitset.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_function.h>
#include <EASTL/bitvector.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_set.h>
#include <EASTL/queue.h>
#include <EASTL/deque.h>
#include <EASTL/optional.h>
#include <EASTL/bonus/tuple_vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <memory/dag_memBase.h>
#include <memory/dag_mem.h>
#include <memory/dag_framemem.h>

#include <vecmath/dag_vecMath.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_DObject.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <util/dag_baseDef.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_hash.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point4.h>
#include <math/dag_bounds3.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_adjpow2.h>
#include <math/dag_bits.h>

#include <daECS/core/entityId.h>
