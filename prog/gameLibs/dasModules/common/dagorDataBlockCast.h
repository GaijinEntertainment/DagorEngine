#pragma once

#include <daScript/daScript.h>

class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;

namespace das
{
// need this function to handle false positive warning
// A call of the 'memcpy' function will lead to underflow of the buffer '& src'
template <typename T>
static void memcpy_silenced(T &dest, vec4f src)
{
  memcpy(&dest, &src, sizeof(T)); //-V512
}

// here we try to extend rvalue lifetime by const ref in daScript
// we need special cast for pass floatN to const PointN &
// It need only for interpretation, aot no need this cast, because it native for c++

#define CAST_CONST_REF_ARG(TYPE)                              \
  template <>                                                 \
  struct cast_arg<const TYPE &>                               \
  {                                                           \
    static __forceinline TYPE to(Context &ctx, SimNode *node) \
    {                                                         \
      vec4f res = node->eval(ctx);                            \
      TYPE v4;                                                \
      memcpy_silenced(v4, res);                               \
      return v4;                                              \
    }                                                         \
  };

CAST_CONST_REF_ARG(Point2)
CAST_CONST_REF_ARG(Point3)
CAST_CONST_REF_ARG(Point4)
CAST_CONST_REF_ARG(IPoint2)
CAST_CONST_REF_ARG(IPoint3)

#undef CAST_CONST_REF_ARG
} // namespace das
