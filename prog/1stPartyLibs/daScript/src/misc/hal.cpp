#include "daScript/misc/platform.h"

#include "daScript/misc/hal.h"

// The out-of-line half of misc/hal.h. Only 32-bit MSVC needs one: there the
// header declares v_ldu_ptr instead of defining it, because that compiler
// generates flawed code for the inline form. Every other toolchain gets the
// inline definition and this file compiles to nothing.
//
// It lives here rather than in the simulator so that a build without one - the
// minimal runtime under nano/ - still resolves it.

#if defined(_MSC_VER) && !defined(__clang__) && INTPTR_MAX == INT32_MAX
VECTORCALL vec4i v_ldu_ptr(const void * a) {return v_seti_x((int32_t)a);}
#endif
