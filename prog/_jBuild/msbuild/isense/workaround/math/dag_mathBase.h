// the EDG in clang mode doesn't like define X /undef X + define X in another header, e.g in dagorInclude\math\dag_Point2.h the INLINE
// I guess the EDG using a token based preprocessor and falsely identifies the undef as last, and doesn't handle the same define later.

#define INLINE inline
#pragma push_macro("INLINE")
#include_next <math\dag_mathBase.h>
#pragma pop_macro("INLINE")