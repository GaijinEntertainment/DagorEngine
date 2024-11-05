// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>

#include <sqrat.h>

#include <math/dag_color.h>
#include <math/dag_math3d.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint4.h>
#include <3d/dag_render.h>
#include <memory/dag_framemem.h>


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

  T *obj = new T();
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
  Sqrat::ClassType<T>::SetManagedInstance(vm, 1, obj);
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

  T *obj = new T();
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
  Sqrat::ClassType<T>::SetManagedInstance(vm, 1, obj);
  return 0;
}


static SQInteger tmatrix_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top != 1 && top != 2)
    return sqstd_throwerrorf(vm, "Invalid arguments count: %d provided, 0 or 1 expected", top - 1);
  if (top == 2 && !Sqrat::check_signature<TMatrix *>(vm, 2))
    return sq_throwerror(vm, "Expected source matrix to copy from as argument");

  TMatrix *obj = new TMatrix();
  if (top == 1)
    obj->identity();
  else
    *obj = *Sqrat::Var<TMatrix *>(vm, 2).value;

  Sqrat::ClassType<TMatrix>::SetManagedInstance(vm, 1, obj);
  return 0;
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

  Quat *out = new Quat();
  memcpy(out, &q, sizeof(Quat));
  Sqrat::ClassType<Quat>::SetManagedInstance(vm, 1, out);
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

  Sqrat::ClassType<E3DCOLOR>::SetManagedInstance(vm, 1, new E3DCOLOR(c.u));
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

  Sqrat::ClassType<Color4>::SetManagedInstance(vm, 1, new Color4(c));
  return 0;
}


void sqrat_bind_dagor_math(SqModules *module_mgr)
{
  ///@module dagor.math
  G_ASSERT(module_mgr);
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Class<Point2> sqPoint2(vm, "Point2");
  ///@class Point2
  sqPoint2 //
    .SquirrelCtor(math_flt_vector_ctor<Point2, 2>, 0, ".x|nn")
    .Var("x", &Point2::x)
    .Var("y", &Point2::y)
    .Func("lengthSq", &Point2::lengthSq)
    .Func("length", &Point2::length)
    .Func("normalize", &Point2::normalize)
    .SquirrelFunc("_add", op_add<Point2>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<Point2>, 2, "xx")
    .SquirrelFunc("_mul", op_mul<Point2>, 2, "xx|n")
    .SquirrelFunc("_unm", op_unm<Point2>, 1, "x")
    /**/;

  /// @class Point3
  Sqrat::Class<Point3> sqPoint3(vm, "Point3");
  sqPoint3 //
    .SquirrelCtor(math_flt_vector_ctor<Point3, 3>, 0, ".x|nnn")
    .Var("x", &Point3::x)
    .Var("y", &Point3::y)
    .Var("z", &Point3::z)
    .Func("lengthSq", &Point3::lengthSq)
    .Func("length", &Point3::length)
    .Func("normalize", &Point3::normalize)
    .SquirrelFunc("_add", op_add<Point3>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<Point3>, 2, "xx")
    .SquirrelFunc("_mul", op_mul<Point3>, 2, "xx|n")
    .SquirrelFunc("_modulo", op_cross<Point3>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<Point3>, 1, "x")
    /**/;

  /// @class DPoint3
  Sqrat::Class<DPoint3> sqDPoint3(vm, "DPoint3");
  sqDPoint3 //
    .SquirrelCtor(math_flt_vector_ctor<DPoint3, 3>, 0, ".x|nnn")
    .Var("x", &DPoint3::x)
    .Var("y", &DPoint3::y)
    .Var("z", &DPoint3::z)
    .Func("lengthSq", &DPoint3::lengthSq)
    .Func("length", &DPoint3::length)
    .Func("normalize", &DPoint3::normalize)
    /**/;

  /// @class Point4
  Sqrat::Class<Point4> sqPoint4(vm, "Point4");
  sqPoint4 //
    .SquirrelCtor(math_flt_vector_ctor<Point4, 4>, 0, ".x|nnnn")
    .Var("x", &Point4::x)
    .Var("y", &Point4::y)
    .Var("z", &Point4::z)
    .Var("w", &Point4::w)
    .Func("lengthSq", &Point4::lengthSq)
    .Func("length", &Point4::length)
    .SquirrelFunc("_add", op_add<Point4>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<Point4>, 2, "xx")
    .SquirrelFunc("_mul", op_mul<Point4>, 2, "xx|n")
    .SquirrelFunc("_modulo", op_cross<Point4>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<Point4>, 1, "x")
    /**/;

  /// @class TMatrix
  Sqrat::Class<TMatrix> sqTMatrix(vm, "TMatrix");
  sqTMatrix //
    .SquirrelCtor(tmatrix_ctor, 0, ".x")
    .Func("orthonormalize", &TMatrix::orthonormalize)
    .SquirrelFunc("_mul", op_mul_tm<TMatrix, Point3>, 2, "xx|n")
    .SquirrelFunc("_add", op_add<TMatrix>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<TMatrix>, 2, "xx")
    .SquirrelFunc("_modulo", op_cross<TMatrix>, 2, "xx")
    .SquirrelFunc("_set", tm_setcol, -3, "xnx|n")
    .SquirrelFunc("setcol", tm_setcol, -3, "xnx|n")
    .SquirrelFunc("_unm", op_unm<TMatrix>, 1, "x")
    .GlobalFunc("inverse", (TMatrix(*)(const TMatrix &))inverse)
    .SquirrelFunc("_get", tm_getcol, 2, "x")
    .SquirrelFunc("getcol", tm_getcol, 2, "x")
    /**/;

  /// @class Quat
  Sqrat::Class<Quat> sqQuat(vm, "Quat");
  sqQuat //
    .SquirrelCtor(quat_ctor, 0, ".x|nnnn")
    .Var("x", &Quat::x)
    .Var("y", &Quat::y)
    .Var("z", &Quat::z)
    .Var("w", &Quat::w)
    .SquirrelFunc("_mul", op_mul<Quat, Quat>, 2, "xx|n")
    .SquirrelFunc("_mul", op_mul_tm<Quat, Point3>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<Quat>, 1, "x")
    .Func("normalize", &Quat::normalize)
    /**/;

  /// @class IPoint2
  Sqrat::Class<IPoint2> sqIPoint2(vm, "IPoint2");
  sqIPoint2 //
    .SquirrelCtor(math_int_vector_ctor<IPoint2, 2>, 0, ".x|nn")
    .Var("x", &IPoint2::x)
    .Var("y", &IPoint2::y)
    .SquirrelFunc("_add", op_add<IPoint2>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<IPoint2>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<IPoint2>, 1, "x")
    /**/;

  /// @class IPoint3
  Sqrat::Class<IPoint3> sqIPoint3(vm, "IPoint3");
  sqIPoint3 //
    .SquirrelCtor(math_int_vector_ctor<IPoint3, 3>, 0, ".x|nnn")
    .Var("x", &IPoint3::x)
    .Var("y", &IPoint3::y)
    .Var("z", &IPoint3::z)
    .SquirrelFunc("_add", op_add<IPoint3>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<IPoint3>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<IPoint3>, 1, "x")
    /**/;

  /// @class IPoint4
  Sqrat::Class<IPoint4> sqIPoint4(vm, "IPoint4");
  sqIPoint4 //
    .SquirrelCtor(math_int_vector_ctor<IPoint4, 4>, 0, ".x|nnnn")
    .Var("x", &IPoint4::x)
    .Var("y", &IPoint4::y)
    .Var("z", &IPoint4::z)
    .Var("w", &IPoint4::w)
    .SquirrelFunc("_add", op_add<IPoint4>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<IPoint4>, 2, "xx")
    .SquirrelFunc("_unm", op_unm<IPoint4>, 1, "x")
    /**/;

  /// @class E3DCOLOR
  Sqrat::Class<E3DCOLOR> sqE3DCOLOR(vm, "E3DCOLOR");
  sqE3DCOLOR //
    .SquirrelCtor(e3dcolor_ctor, 0, ".x|nnnn")
    .Var("r", &E3DCOLOR::r)
    .Var("g", &E3DCOLOR::g)
    .Var("b", &E3DCOLOR::b)
    .Var("a", &E3DCOLOR::a)
    .Var("u", &E3DCOLOR::u)
    /**/;

  /// @class Color3
  Sqrat::Class<Color3> sqColor3(vm, "Color3");
  sqColor3 //
    .SquirrelCtor(math_flt_vector_ctor<Color3, 3>, 0, ".x|nnnn")
    .Var("r", &Color3::r)
    .Var("g", &Color3::g)
    .Var("b", &Color3::b)
    .SquirrelFunc("_add", op_add<Color3>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<Color3>, 2, "xx")
    .SquirrelFunc("_mul", op_mul<Color3, Color3>, 2, "xx|n")
    .SquirrelFunc("set", c3_set, 3, "nnn")
    /**/;

  /// @class Color4
  Sqrat::Class<Color4> sqColor4(vm, "Color4");
  sqColor4 //
    .SquirrelCtor(color4_ctor, 0, ".x|nnnn")
    .Var("r", &Color4::r)
    .Var("g", &Color4::g)
    .Var("b", &Color4::b)
    .Var("a", &Color4::a)
    .SquirrelFunc("_add", op_add<Color4>, 2, "xx")
    .SquirrelFunc("_sub", op_sub<Color4>, 2, "xx")
    .SquirrelFunc("_mul", op_mul<Color4, Color4>, 2, "xx|n")
    .SquirrelFunc("set", c4_set, 4, "nnnn")
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
#undef BIND

  module_mgr->addNativeModule("dagor.math", nsTbl);
}

} // namespace bindquirrel
