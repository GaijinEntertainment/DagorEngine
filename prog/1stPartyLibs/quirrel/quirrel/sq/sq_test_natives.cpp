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
  module_mgr->addNativeModule("test.native", exports);
}
