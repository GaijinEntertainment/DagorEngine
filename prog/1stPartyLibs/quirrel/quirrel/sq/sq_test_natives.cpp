// Test native module for native field testing.
// Registers "test.native" module with NativeVec class.

#include <sqmodules.h>
#include <sqrat.h>
#include <string.h>
#include <stdio.h>

namespace
{

struct TestNativeVec
{
  float x, y, z;
  int32_t w;
};

static SQInteger nativevec_ctor(HSQUIRRELVM vm)
{
  TestNativeVec *self = Sqrat::ClassType<TestNativeVec>::GetInstance(vm, 1);
  if (!self)
    return SQ_ERROR;
  memset(self, 0, sizeof(TestNativeVec));
  SQInteger top = sq_gettop(vm);
  if (top >= 2) { SQFloat f; sq_getfloat(vm, 2, &f); self->x = (float)f; }
  if (top >= 3) { SQFloat f; sq_getfloat(vm, 3, &f); self->y = (float)f; }
  if (top >= 4) { SQFloat f; sq_getfloat(vm, 4, &f); self->z = (float)f; }
  if (top >= 5) { SQInteger i; sq_getinteger(vm, 5, &i); self->w = (int32_t)i; }
  return 0;
}

static SQInteger test_raw_cmp(HSQUIRRELVM vm)
{
  sq_push(vm, 2);
  sq_push(vm, 3);
  SQInteger r = sq_cmp(vm);
  sq_pop(vm, 2);
  sq_reseterror(vm); // ObjCmp raises a compare error we intentionally ignore here
  sq_pushinteger(vm, r);
  return 1;
}

static SQInteger test_reserve_stack(HSQUIRRELVM vm)
{
  SQInteger n = 4096;
  if (sq_gettop(vm) >= 2)
    sq_getinteger(vm, 2, &n);
  SQRESULT r = sq_reservestack(vm, n);
  if (SQ_FAILED(r))
    return r;
  return 0;
}

static SQInteger test_ud_with_delegate(HSQUIRRELVM vm)
{
  sq_newuserdata(vm, 4);
  sq_newtable(vm);
  sq_pushstring(vm, "a", -1); sq_pushinteger(vm, 10); sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "b", -1); sq_pushinteger(vm, 20); sq_newslot(vm, -3, SQFalse);
  if (SQ_FAILED(sq_setdelegate(vm, -2))) // sets the table (top) as delegate of the userdata
    return SQ_ERROR;
  return 1; // userdata now on top
}

static SQInteger test_identity_i64(SQInteger x)
{
  return x;
}

} // namespace


template<>
struct Sqrat::InstanceToString<TestNativeVec>
{
  static SQInteger Format(HSQUIRRELVM vm)
  {
    TestNativeVec *self = Sqrat::ClassType<TestNativeVec>::GetInstance(vm, 1);
    if (!self)
      return SQ_ERROR;
    char buf[128];
    snprintf(buf, sizeof(buf), "NativeVec(%g, %g, %g, %d)", self->x, self->y, self->z, self->w);
    sq_pushstring(vm, buf, -1);
    return 1;
  }
};


void register_test_natives(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Class<TestNativeVec> cls(vm, "NativeVec");
  cls
    .UseInlineUserdata()
    .SquirrelCtor(nativevec_ctor, 0, ".nnnn")
    .NativeVar("x", &TestNativeVec::x)
    .NativeVar("y", &TestNativeVec::y)
    .NativeVar("z", &TestNativeVec::z)
    .NativeVar("w", &TestNativeVec::w);

  Sqrat::Table exports(vm);
  exports.Bind("NativeVec", cls);
  exports.SquirrelFunc("raw_cmp", test_raw_cmp, 3, "...");
  exports.SquirrelFunc("reserve_stack", test_reserve_stack, -1, ".n");
  exports.SquirrelFunc("ud_with_delegate", test_ud_with_delegate, 1, ".");
  exports.Func("identity_i64", test_identity_i64);
  module_mgr->addNativeModule("test.native", exports);
}
