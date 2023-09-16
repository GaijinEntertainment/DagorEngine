template<class v4si>
v4si __builtin_ia32_pshufd (v4si, int);

template<class v4si>
v4si __builtin_ia32_shufps (v4si, v4si, int);

template<class __v16qu>
__v16qu __builtin_elementwise_min (__v16qu, __v16qu);

template<class __v16qu>
__v16qu __builtin_elementwise_max (__v16qu, __v16qu);

// Forward declaration for BuiltinStringId, because the EDG in clang mode doesn't like the typeless enum;
// and give "declaration of enum type is nonstandard" error.
// https://stackoverflow.com/a/72599

namespace dagui
{
  enum BuiltinStringId : int;
}

// Forward declaration for normalizeDef with rvalue override, because the EDG in clang mode falsely
// selects the (Point3 &, const Point3 &) in case of xvalue is the first parameter.

class Point3;
inline Point3 normalizeDef(const Point3 &a, const Point3 & def);
inline Point3 normalizeDef(Point3 &&a, const Point3 & def);
inline void normalizeDef(Point3 &a, const Point3 & def);
