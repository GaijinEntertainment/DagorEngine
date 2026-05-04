// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqmodules/sqmodules.h>
#include <sqext.h>

#include <sqrat.h>
#include <util/dag_string.h>

#include <math/dag_color.h>
#include <math/dag_math3d.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint4.h>
#include <3d/dag_render.h>
#include <memory/dag_framemem.h>
#include <math/dag_catmullRom.h>

inline Point3 sq_matrix_to_euler(const TMatrix &tm)
{
  Point3 e;
  matrix_to_euler(tm, e.x, e.y, e.z);
  return e;
}
inline Point3 sq_quat_to_euler(const Quat &quat)
{
  Point3 e;
  quat_to_euler(quat, e.x, e.y, e.z);
  return e;
}
inline TMatrix sq_quat_to_matrix(const Quat &quat) { return quat_to_matrix(quat); }
inline Quat sq_euler_to_quat(const Point3 &e)
{
  Quat q;
  euler_to_quat(e.x, e.y, e.z, q);
  return q;
}
inline Quat sq_dir_to_quat(const Point3 &p) { return dir_to_quat(p); }

namespace Sqrat
{

#define FORMAT_MATH_INSTANCE(type, fmt_string, ...) \
  template <>                                       \
  struct InstanceToString<type>                     \
  {                                                 \
    static SQInteger Format(HSQUIRRELVM vm)         \
    {                                               \
      if (!Sqrat::check_signature<type *>(vm))      \
        return SQ_ERROR;                            \
      Sqrat::Var<type *> self(vm, 1);               \
      type &v = *self.value;                        \
      String s(framemem_ptr());                     \
      s.printf(0, fmt_string, __VA_ARGS__);         \
      sq_pushstring(vm, s.c_str(), s.length());     \
      return 1;                                     \
    }                                               \
  };


FORMAT_MATH_INSTANCE(Point2, "Point2(%g, %g)", v.x, v.y);
FORMAT_MATH_INSTANCE(Point3, "Point3(%g, %g, %g)", v.x, v.y, v.z);
FORMAT_MATH_INSTANCE(Point4, "Point4(%g, %g, %g, %g)", v.x, v.y, v.z, v.w);
FORMAT_MATH_INSTANCE(IPoint2, "IPoint2(%d, %d)", v.x, v.y);
FORMAT_MATH_INSTANCE(IPoint3, "IPoint3(%d, %d, %d)", v.x, v.y, v.z);
FORMAT_MATH_INSTANCE(IPoint4, "IPoint4(%d, %d, %d, %d)", v.x, v.y, v.z, v.w);
FORMAT_MATH_INSTANCE(DPoint3, "DPoint3(%lg, %lg, %lg)", v.x, v.y, v.z);
FORMAT_MATH_INSTANCE(Color3, "Color3(%g, %g, %g)", v.r, v.g, v.b);
FORMAT_MATH_INSTANCE(Color4, "Color4(%g, %g, %g, %g)", v.r, v.g, v.b, v.a);
FORMAT_MATH_INSTANCE(Quat, "Quat(%g, %g, %g, %g)", v.x, v.y, v.z, v.w);
// Use increased precision for basis vectors since this data might be used as actual source data in various places
// and it's important for matrices to be orthonormalized
FORMAT_MATH_INSTANCE(TMatrix, "TMatrix([%.9g, %.9g, %.9g] [%.9g, %.9g, %.9g] [%.9g, %.9g, %.9g] [%g, %g, %g])", v.m[0][0], v.m[0][1],
  v.m[0][2], v.m[1][0], v.m[1][1], v.m[1][2], v.m[2][0], v.m[2][1], v.m[2][2], v.m[3][0], v.m[3][1], v.m[3][2]);


} // namespace Sqrat


namespace bindquirrel
{

template <typename T>
static SQInteger op_add(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *, T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<const T *> a(vm, 1);
  Sqrat::Var<const T *> b(vm, 2);
  Sqrat::Var<T>::push(vm, *a.value + *b.value);
  return 1;
}

template <typename T>
static SQInteger op_sub(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *, T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<const T *> a(vm, 1);
  Sqrat::Var<const T *> b(vm, 2);
  Sqrat::Var<T>::push(vm, *a.value - *b.value);
  return 1;
}

template <typename T>
static SQInteger op_unm(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<const T *> a(vm, 1);
  Sqrat::Var<T>::push(vm, -*a.value);
  return 1;
}


template <typename T, typename Res = float>
static SQInteger op_mul(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T> a(vm, 1);
  HSQOBJECT ho;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &ho)));

  if (sq_type(ho) == OT_INSTANCE && Sqrat::ClassType<T>::IsObjectOfClass(&ho))
  {
    Sqrat::Var<T> b(vm, 2);
    Sqrat::Var<Res>::push(vm, a.value * b.value);
  }
  else if (sq_isnumeric(ho))
  {
    float mul;
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 2, &mul)));
    Sqrat::Var<T>::push(vm, a.value * mul);
  }
  else
    return sq_throwerror(vm, "Invalid multiplier type");
  return 1;
}

template <typename T, typename U>
static SQInteger op_mul_tm(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T> a(vm, 1);

  HSQOBJECT ho;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &ho)));

  if (sq_type(ho) == OT_INSTANCE && Sqrat::ClassType<T>::IsObjectOfClass(&ho))
  {
    Sqrat::Var<T> b(vm, 2);
    Sqrat::Var<T>::push(vm, a.value * b.value);
  }
  else if (sq_type(ho) == OT_INSTANCE && Sqrat::ClassType<U>::IsObjectOfClass(&ho))
  {
    Sqrat::Var<U> b(vm, 2);
    Sqrat::Var<U>::push(vm, a.value * b.value);
  }
  else if (sq_isnumeric(ho))
  {
    float mul;
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 2, &mul)));
    Sqrat::Var<T>::push(vm, a.value * mul);
  }
  else
    return sq_throwerror(vm, "Invalid multiplier type");
  return 1;
}


template <typename T>
static SQInteger op_cross(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<T *, T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<const T *> a(vm, 1);
  Sqrat::Var<const T *> b(vm, 2);
  Sqrat::Var<T>::push(vm, *a.value % *b.value);
  return 1;
}

static SQInteger c3_set(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Color3 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Color3 &> c(vm, 1);
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 2, &c.value.r)));
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 3, &c.value.g)));
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 4, &c.value.b)));
  return 1;
}

static SQInteger c4_set(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Color4 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Color4 &> c(vm, 1);
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 2, &c.value.r)));
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 3, &c.value.g)));
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 4, &c.value.b)));
  G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 5, &c.value.a)));
  return 1;
}


static SQInteger tm_setcol(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TMatrix *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TMatrix *> tm(vm, 1);
  SQInteger col = -1;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &col)));

  if (col < 0 || col > 3)
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }

  if (sq_gettop(vm) == 3)
  {
    if (!Sqrat::check_signature<Point3 *>(vm, 3))
      return SQ_ERROR;

    Sqrat::Var<Point3 *> p3(vm, 3);
    tm.value->setcol(col, *p3.value);
  }
  else
  {
    float x, y, z;
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 3, &x)));
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 4, &y)));
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 5, &z)));
    tm.value->setcol(col, x, y, z);
  }
  return 0;
}


static SQInteger tm_getcol(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TMatrix *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TMatrix *> tm(vm, 1);

  SQInteger idx = -1;
  if (sq_gettype(vm, 2) & SQOBJECT_NUMERIC)
    sq_getinteger(vm, 2, &idx);

  if (idx < 0 || idx > 3)
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }

  Sqrat::Var<Point3>::push(vm, tm.value->getcol(idx));
  return 1;
}


template <typename T, int n_comp>
static SQInteger math_flt_vector_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2 && top != n_comp + 1)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0, 1 or %d expected", top - 1, n_comp);
  if (top == 2 && !Sqrat::check_signature<T *>(vm, 2))
    return sq_throwerror(vm, "Invalid type passed to copy ctor");

  T *obj = Sqrat::ClassType<T>::AllocInstanceData(vm, 1);
  if (top == 1)
    memset(obj, 0, sizeof(T));
  else if (top == 2)
    *obj = *(Sqrat::Var<T *>(vm, 2).value);
  else
  {
    SQFloat f;
    for (int i = 0; i < n_comp; ++i)
    {
      sq_getfloat(vm, i + 2, &f);
      (*obj)[i] = (decltype((*obj)[0]))f;
    }
  }
  return 0;
}


template <typename T, int n_comp>
static SQInteger math_int_vector_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2 && top != n_comp + 1)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0, 1 or %d expected", top - 1, n_comp);
  if (top == 2 && !Sqrat::check_signature<T *>(vm, 2))
    return sq_throwerror(vm, "Invalid type passed to copy ctor");

  T *obj = Sqrat::ClassType<T>::AllocInstanceData(vm, 1);
  if (top == 1)
    memset(obj, 0, sizeof(T));
  else if (top == 2)
    *obj = *(Sqrat::Var<T *>(vm, 2).value);
  else
  {
    SQInteger n;
    for (int i = 0; i < n_comp; ++i)
    {
      sq_getinteger(vm, i + 2, &n);
      (*obj)[i] = (decltype((*obj)[0]))n;
    }
  }
  return 0;
}


static SQInteger tmatrix_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0 or 1 expected", top - 1);
  if (top == 2 && !Sqrat::check_signature<TMatrix *>(vm, 2))
    return sq_throwerror(vm, "Expected source matrix to copy from as argument");

  TMatrix *obj = Sqrat::ClassType<TMatrix>::AllocInstanceData(vm, 1);
  if (top == 1)
    obj->identity();
  else
    *obj = *Sqrat::Var<TMatrix *>(vm, 2).value;
  return 0;
}


template <typename T>
static SQInteger raw_float_setstate(HSQUIRRELVM vm)
{
  G_STATIC_ASSERT(sizeof(T) % sizeof(float) == 0);

  SQInteger top = sq_gettop(vm);
  if (top != 2 && top != 3)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 1 or 2 expected", top - 1);

  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T *> obj(vm, 1);
  Sqrat::Var<Sqrat::Array> arr(vm, 2);
  SQInteger arrSize = sq_getsize(vm, 2);

  const int count = sizeof(T) / sizeof(float);
  if (count != arrSize)
    return sqstd_throwerrorf(vm, "raw_float_setstate: invalid data size = %d bytes, expected %d", int(arrSize * sizeof(float)),
      int(sizeof(T)));

  float *ptr = (float *)obj.value;

  for (int i = 0; i < count; i++)
    ptr[i] = arr.value.GetValue<float>(i);

  return 0;
}

template <typename T>
static SQInteger raw_float_getstate(HSQUIRRELVM vm)
{
  G_STATIC_ASSERT(sizeof(T) % sizeof(float) == 0);

  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 1 or 2 expected", top - 1);

  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T *> obj(vm, 1);
  float *ptr = (float *)obj.value;

  const int count = sizeof(T) / sizeof(float);
  Sqrat::Array result(vm, count);

  for (int i = 0; i < count; i++)
    result.SetValue(i, SQFloat(ptr[i]));

  Sqrat::PushVar(vm, result);
  return 1;
}


template <typename T>
static SQInteger raw_int32_setstate(HSQUIRRELVM vm)
{
  G_STATIC_ASSERT(sizeof(T) % sizeof(int32_t) == 0);

  SQInteger top = sq_gettop(vm);
  if (top != 2 && top != 3)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 1 or 2 expected", top - 1);

  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T *> obj(vm, 1);
  Sqrat::Var<Sqrat::Array> arr(vm, 2);
  SQInteger arrSize = sq_getsize(vm, 2);

  const int count = sizeof(T) / sizeof(int32_t);
  if (count != arrSize)
    return sqstd_throwerrorf(vm, "raw_int32_setstate: invalid data size = %d bytes, expected %d", int(arrSize * sizeof(int32_t)),
      int(sizeof(T)));

  int32_t *ptr = (int32_t *)obj.value;

  for (int i = 0; i < count; i++)
    ptr[i] = arr.value.GetValue<int32_t>(i);

  return 0;
}

template <typename T>
static SQInteger raw_int32_getstate(HSQUIRRELVM vm)
{
  G_STATIC_ASSERT(sizeof(T) % sizeof(int32_t) == 0);

  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 1 or 2 expected", top - 1);

  if (!Sqrat::check_signature<T *>(vm))
    return SQ_ERROR;

  Sqrat::Var<T *> obj(vm, 1);
  int32_t *ptr = (int32_t *)obj.value;

  const int count = sizeof(T) / sizeof(int32_t);
  Sqrat::Array result(vm, count);

  for (int i = 0; i < count; i++)
    result.SetValue(i, ptr[i]);

  Sqrat::PushVar(vm, result);
  return 1;
}


static SQInteger quat_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  Quat q;

  if (top == 1)
    q.identity();
  else if (top == 2)
  {
    if (Sqrat::check_signature<TMatrix *>(vm, 2))
      q = Quat(*Sqrat::Var<TMatrix *>(vm, 2).value);
    else if (Sqrat::check_signature<Quat *>(vm, 2))
      memcpy(&q, Sqrat::Var<Quat *>(vm, 2).value, sizeof(q));
    else
      return sq_throwerror(vm, "Expected source TMatrix or Quat as argument");
  }
  else if (top == 5)
  {
    SQFloat f;
    for (int i = 0; i < 4; ++i)
    {
      if (SQ_FAILED(sq_getfloat(vm, i + 2, &f)))
        return sq_throwerror(vm, "All components must be numbers");
      q[i] = f;
    }
  }
  else
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0, 1 or 4 expected", top - 1);

  Quat *out = Sqrat::ClassType<Quat>::AllocInstanceData(vm, 1);
  memcpy(out, &q, sizeof(Quat));
  return 0;
}


static SQInteger e3dcolor_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  E3DCOLOR c;

  if (top == 1)
    c.u = 0;
  else if (top == 2)
  {
    if (sq_gettype(vm, 2) & SQOBJECT_NUMERIC)
    {
      SQInteger n;
      sq_getinteger(vm, 2, &n);
      c.u = n;
    }
    else if (Sqrat::check_signature<E3DCOLOR *>(vm, 2))
      c = Sqrat::Var<E3DCOLOR>(vm, 2).value;
    else
      return sq_throwerror(vm, "Invalid argument for copy ctor");
  }
  else if (top == 4 || top == 5)
  {
    SQInteger x;
    sq_getinteger(vm, 2, &x);
    c.r = x;
    sq_getinteger(vm, 3, &x);
    c.g = x;
    sq_getinteger(vm, 4, &x);
    c.b = x;
    if (top == 4)
      c.a = 255;
    else
    {
      sq_getinteger(vm, 5, &x);
      c.a = x;
    }
  }
  else
    return sq_throwerror(vm, "Wrong ctor arguments count");

  E3DCOLOR *out = Sqrat::ClassType<E3DCOLOR>::AllocInstanceData(vm, 1);
  *out = c;
  return 0;
}


static SQInteger color4_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  Color4 c;

  if (top == 1)
    memset(&c, 0, sizeof(c));
  else if (top == 2)
  {
    if (Sqrat::check_signature<Color4 *>(vm, 2))
      c = Sqrat::Var<Color4>(vm, 2).value;
    else
      return sq_throwerror(vm, "Invalid argument for copy ctor");
  }
  else if (top == 4 || top == 5)
  {
    SQFloat x;
    sq_getfloat(vm, 2, &x);
    c.r = x;
    sq_getfloat(vm, 3, &x);
    c.g = x;
    sq_getfloat(vm, 4, &x);
    c.b = x;
    if (top == 4)
      c.a = 1;
    else
    {
      sq_getfloat(vm, 5, &x);
      c.a = x;
    }
  }
  else
    return sq_throwerror(vm, "Wrong ctor arguments count");

  Color4 *out = Sqrat::ClassType<Color4>::AllocInstanceData(vm, 1);
  *out = c;
  return 0;
}

static SQInteger catmull_rom_2d_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top != 1)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0 expected", top - 1);

  Sqrat::ClassType<CatmullRomSplineBuilder2D>::AllocInstanceData(vm, 1);
  return 0;
}

static SQInteger catmull_rom_2d_build(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  // arguments: this, points, [is_closed, tension]
  if (top < 2 || top > 4)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 1..3 expected", top - 1);

  if (!Sqrat::check_signature<CatmullRomSplineBuilder2D *>(vm))
    return SQ_ERROR;

  CatmullRomSplineBuilder2D *obj = Sqrat::Var<CatmullRomSplineBuilder2D *>(vm, 1).value;

  SQBool isClosed = false;
  if (top >= 3)
    G_VERIFY(SQ_SUCCEEDED(sq_getbool(vm, 3, &isClosed)));

  float tension = 0.0f;
  if (top >= 4)
    G_VERIFY(SQ_SUCCEEDED(sq_getfloat(vm, 4, &tension)));

  Sqrat::Var<Sqrat::Array> sqPoints(vm, 2);
  SQInteger nPts = sq_getsize(vm, 2) / 2;

  Tab<Point2> keyPoints(framemem_ptr());
  keyPoints.resize(nPts);

  if (sq_ext_get_array_floats(sqPoints.value.GetObject(), 0, nPts * 2, &keyPoints[0].x) != SQ_OK)
    return sq_throwerror(vm, "Failed to parse points");

  if (nPts < 2)
    return sq_throwerror(vm, "At least 2 points are required");

  bool result = obj->build(keyPoints, isClosed, tension);
  sq_pushbool(vm, result);
  return 1;
}


void sqrat_bind_dagor_math(SqModules *module_mgr)
{
  ///@module dagor.math
  G_ASSERT(module_mgr);
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Class<Point2> sqPoint2(vm, "Point2");
  ///@class Point2
  sqPoint2
    .UseInlineUserdata() //
    .SquirrelCtor(math_flt_vector_ctor<Point2, 2>, 0, ".x|nn")
    .NativeVar("x", &Point2::x)
    .NativeVar("y", &Point2::y)
    .Func("lengthSq", &Point2::lengthSq)
    .Func("length", &Point2::length)
    .Func("normalize", &Point2::normalize)
    .SquirrelFuncDeclString(op_add<Point2>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<Point2>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_mul<Point2>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(op_unm<Point2>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_float_setstate<Point2>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Point2>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class Point3
  Sqrat::Class<Point3> sqPoint3(vm, "Point3");
  sqPoint3
    .UseInlineUserdata() //
    .SquirrelCtor(math_flt_vector_ctor<Point3, 3>, 0, ".x|nnn")
    .NativeVar("x", &Point3::x)
    .NativeVar("y", &Point3::y)
    .NativeVar("z", &Point3::z)
    .Func("lengthSq", &Point3::lengthSq)
    .Func("length", &Point3::length)
    .Func("normalize", &Point3::normalize)
    .SquirrelFuncDeclString(op_add<Point3>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<Point3>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_mul<Point3>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(op_cross<Point3>, "instance._modulo(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<Point3>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_float_setstate<Point3>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Point3>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class DPoint3
  Sqrat::Class<DPoint3> sqDPoint3(vm, "DPoint3");
  sqDPoint3
    .UseInlineUserdata() //
    .SquirrelCtor(math_flt_vector_ctor<DPoint3, 3>, 0, ".x|nnn")
    .NativeVar("x", &DPoint3::x)
    .NativeVar("y", &DPoint3::y)
    .NativeVar("z", &DPoint3::z)
    .Func("lengthSq", &DPoint3::lengthSq)
    .Func("length", &DPoint3::length)
    .Func("normalize", &DPoint3::normalize)
    .SquirrelFuncDeclString(raw_int32_setstate<DPoint3>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_int32_getstate<DPoint3>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class Point4
  Sqrat::Class<Point4> sqPoint4(vm, "Point4");
  sqPoint4
    .UseInlineUserdata() //
    .SquirrelCtor(math_flt_vector_ctor<Point4, 4>, 0, ".x|nnnn")
    .NativeVar("x", &Point4::x)
    .NativeVar("y", &Point4::y)
    .NativeVar("z", &Point4::z)
    .NativeVar("w", &Point4::w)
    .Func("lengthSq", &Point4::lengthSq)
    .Func("length", &Point4::length)
    .SquirrelFuncDeclString(op_add<Point4>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<Point4>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_mul<Point4>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(op_cross<Point4>, "instance._modulo(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<Point4>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_float_setstate<Point4>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Point4>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class TMatrix
  Sqrat::Class<TMatrix> sqTMatrix(vm, "TMatrix");
  sqTMatrix
    .UseInlineUserdata() //
    .SquirrelCtor(tmatrix_ctor, 0, ".x")
    .Func("orthonormalize", &TMatrix::orthonormalize)
    .SquirrelFuncDeclString(op_mul_tm<TMatrix, Point3>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(op_add<TMatrix>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<TMatrix>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_cross<TMatrix>, "instance._modulo(other: instance): instance")
    .SquirrelFuncDeclString(tm_setcol, "instance._set(col: int, arg: instance|number, ...): null")
    .SquirrelFuncDeclString(tm_setcol, "instance.setcol(col: int, arg: instance|number, ...): null")
    .SquirrelFuncDeclString(op_unm<TMatrix>, "instance._unm(): instance")
    .GlobalFunc("inverse", (TMatrix(*)(const TMatrix &))inverse)
    .SquirrelFuncDeclString(tm_getcol, "instance._get(idx: any): instance")
    .SquirrelFuncDeclString(tm_getcol, "instance.getcol(idx: any): instance")
    .SquirrelFuncDeclString(raw_float_setstate<TMatrix>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<TMatrix>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class Quat
  Sqrat::Class<Quat> sqQuat(vm, "Quat");
  sqQuat
    .UseInlineUserdata() //
    .SquirrelCtor(quat_ctor, 0, ".x|nnnn")
    .NativeVar("x", &Quat::x)
    .NativeVar("y", &Quat::y)
    .NativeVar("z", &Quat::z)
    .NativeVar("w", &Quat::w)
    .SquirrelFuncDeclString(op_mul<Quat, Quat>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(op_mul_tm<Quat, Point3>, "instance._mul(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<Quat>, "instance._unm(): instance")
    .Func("normalize", &Quat::normalize)
    .SquirrelFuncDeclString(raw_float_setstate<Quat>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Quat>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class IPoint2
  Sqrat::Class<IPoint2> sqIPoint2(vm, "IPoint2");
  sqIPoint2
    .UseInlineUserdata() //
    .SquirrelCtor(math_int_vector_ctor<IPoint2, 2>, 0, ".x|nn")
    .NativeVar("x", &IPoint2::x)
    .NativeVar("y", &IPoint2::y)
    .SquirrelFuncDeclString(op_add<IPoint2>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<IPoint2>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<IPoint2>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_int32_setstate<IPoint2>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_int32_getstate<IPoint2>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class IPoint3
  Sqrat::Class<IPoint3> sqIPoint3(vm, "IPoint3");
  sqIPoint3
    .UseInlineUserdata() //
    .SquirrelCtor(math_int_vector_ctor<IPoint3, 3>, 0, ".x|nnn")
    .NativeVar("x", &IPoint3::x)
    .NativeVar("y", &IPoint3::y)
    .NativeVar("z", &IPoint3::z)
    .SquirrelFuncDeclString(op_add<IPoint3>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<IPoint3>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<IPoint3>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_int32_setstate<IPoint3>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_int32_getstate<IPoint3>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class IPoint4
  Sqrat::Class<IPoint4> sqIPoint4(vm, "IPoint4");
  sqIPoint4
    .UseInlineUserdata() //
    .SquirrelCtor(math_int_vector_ctor<IPoint4, 4>, 0, ".x|nnnn")
    .NativeVar("x", &IPoint4::x)
    .NativeVar("y", &IPoint4::y)
    .NativeVar("z", &IPoint4::z)
    .NativeVar("w", &IPoint4::w)
    .SquirrelFuncDeclString(op_add<IPoint4>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<IPoint4>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_unm<IPoint4>, "instance._unm(): instance")
    .SquirrelFuncDeclString(raw_int32_setstate<IPoint4>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_int32_getstate<IPoint4>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class E3DCOLOR
  Sqrat::Class<E3DCOLOR> sqE3DCOLOR(vm, "E3DCOLOR");
  sqE3DCOLOR
    .UseInlineUserdata() //
    .SquirrelCtor(e3dcolor_ctor, 0, ".x|nnnn")
    .Var("r", &E3DCOLOR::r)
    .Var("g", &E3DCOLOR::g)
    .Var("b", &E3DCOLOR::b)
    .Var("a", &E3DCOLOR::a)
    .Var("u", &E3DCOLOR::u)
    .SquirrelFuncDeclString(raw_int32_setstate<E3DCOLOR>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_int32_getstate<E3DCOLOR>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class Color3
  Sqrat::Class<Color3> sqColor3(vm, "Color3");
  sqColor3
    .UseInlineUserdata() //
    .SquirrelCtor(math_flt_vector_ctor<Color3, 3>, 0, ".x|nnnn")
    .NativeVar("r", &Color3::r)
    .NativeVar("g", &Color3::g)
    .NativeVar("b", &Color3::b)
    .SquirrelFuncDeclString(op_add<Color3>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<Color3>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_mul<Color3, Color3>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(c3_set, "instance.set(r: number, g: number, b: number): any")
    .SquirrelFuncDeclString(raw_float_setstate<Color3>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Color3>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class Color4
  Sqrat::Class<Color4> sqColor4(vm, "Color4");
  sqColor4
    .UseInlineUserdata() //
    .SquirrelCtor(color4_ctor, 0, ".x|nnnn")
    .NativeVar("r", &Color4::r)
    .NativeVar("g", &Color4::g)
    .NativeVar("b", &Color4::b)
    .NativeVar("a", &Color4::a)
    .SquirrelFuncDeclString(op_add<Color4>, "instance._add(other: instance): instance")
    .SquirrelFuncDeclString(op_sub<Color4>, "instance._sub(other: instance): instance")
    .SquirrelFuncDeclString(op_mul<Color4, Color4>, "instance._mul(other: instance|number): instance")
    .SquirrelFuncDeclString(c4_set, "instance.set(r: number, g: number, b: number, a: number): any")
    .SquirrelFuncDeclString(raw_float_setstate<Color4>, "instance.__setstate(state: array, [dummy: any]): null")
    .SquirrelFuncDeclString(raw_float_getstate<Color4>, "instance.__getstate([dummy: any]): array")
    /**/;

  /// @class CatmullRomSplineBuilder2D
  Sqrat::Class<CatmullRomSplineBuilder2D> sqCatmullRomSplineBuilder2D(vm, "CatmullRomSplineBuilder2D");
  sqCatmullRomSplineBuilder2D
    .UseInlineUserdata() //
    .SquirrelCtor(catmull_rom_2d_ctor, 0, ".")
    .SquirrelFuncDeclString(catmull_rom_2d_build, "instance.build(points: array, [is_closed: bool, tension: number]): bool")
    .Func("getTotalSplineLength", &CatmullRomSplineBuilder2D::getTotalSplineLength)
    .Func("getMonotonicPoint", &CatmullRomSplineBuilder2D::getMonotonicPoint)
    .Func("getRawPoint", &CatmullRomSplineBuilder2D::getRawPoint)
    .Func("getMonotonicNormal", &CatmullRomSplineBuilder2D::getMonotonicPoint)
    .Func("getRawNormal", &CatmullRomSplineBuilder2D::getRawPoint)
    /**/;

  Sqrat::Table nsTbl(vm);
  ///@resetscope
  ///@module dagor.math
  nsTbl //
    .Func("matrix_to_euler", sq_matrix_to_euler)
    .Func("euler_to_quat", sq_euler_to_quat)
    .Func("dir_to_quat", sq_dir_to_quat)
    .Func("quat_to_euler", sq_quat_to_euler)
    .Func("quat_to_matrix", sq_quat_to_matrix)
    .Func("quat_rotation_arc", quat_rotation_arc)
    .Func("qinterp", qinterp)
    .Func("cvt", cvt)
    .Func("make_tm_quat", (TMatrix(*)(const Quat &))makeTM)
    .Func("make_tm_axis", (TMatrix(*)(const Point3 &, float))makeTM)
    .Func("norm_s_ang", norm_s_ang)
    /**/;

#define BIND(cn) nsTbl.Bind(#cn, sq##cn);
  BIND(Point2);
  BIND(Point3);
  BIND(Point4);
  BIND(TMatrix);
  BIND(IPoint2);
  BIND(IPoint3);
  BIND(IPoint4);
  BIND(DPoint3);
  BIND(E3DCOLOR);
  BIND(Color3);
  BIND(Color4);
  BIND(Quat);
  BIND(CatmullRomSplineBuilder2D);
#undef BIND

  module_mgr->addNativeModule("dagor.math", nsTbl);
}

} // namespace bindquirrel
