template <class v4si>
v4si __builtin_ia32_pshufd(v4si, int);

template <class v4si>
v4si __builtin_ia32_shufps(v4si, v4si, int);

template <class v4si>
int __builtin_ia32_vec_ext_v4si(v4si, const int);

template <class v2di>
long long __builtin_ia32_vec_ext_v2di(v2di, const int);

template <class v2di>
v2di __builtin_ia32_psrldqi128_byteshift(v2di, const int);

template <class v2di>
v2di __builtin_ia32_pslldqi128_byteshift(v2di, const int);

template <class v8hi>
short __builtin_ia32_vec_ext_v8hi(v8hi, const int);

template <class T>
T __builtin_elementwise_min(T, T);

template <class T>
T __builtin_elementwise_max(T, T);

template <class T>
T __builtin_elementwise_add_sat(T, T);

template <class T>
T __builtin_elementwise_sub_sat(T, T);

// Forward declaration for BuiltinStringId, because the EDG in clang mode doesn't like the typeless enum;
// and give "declaration of enum type is nonstandard" error.
// https://stackoverflow.com/a/72599

namespace dagui
{
enum BuiltinStringId : int;
}

// Forward declaration for normalizeDef with rvalue override, because the EDG in clang mode falsely
// selects the (Point3 &, const Point3 &) in case of xvalue is the first parameter.

class Point2;
inline Point2 normalizeDef(const Point2 &a, const Point2 &def);
inline Point2 normalizeDef(Point2 &&a, const Point2 &def);
inline void normalizeDef(Point2 &a, const Point2 &def);

class Point3;
inline Point3 normalizeDef(const Point3 &a, const Point3 &def);
inline Point3 normalizeDef(Point3 &&a, const Point3 &def);
inline void normalizeDef(Point3 &a, const Point3 &def);
