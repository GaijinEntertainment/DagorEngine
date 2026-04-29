// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqmodules/sqmodules.h>
#include <util/dag_preprocessor.h>

#include <cpu_features/include/cpu_features_macros.h>
#if defined(CPU_FEATURES_ARCH_X86)
#include <cpu_features/include/cpuinfo_x86.h>
#define CPUINFO_ARCH      X86
#define CPUINFO_ARCH_CAPS X86
#elif defined(CPU_FEATURES_ARCH_AARCH64)
#include <cpu_features/include/cpuinfo_aarch64.h>
#define CPUINFO_ARCH      Aarch64
#define CPUINFO_ARCH_CAPS AARCH64
#else
// Append on demand
#endif

#ifdef CPUINFO_ARCH

#define CONCAT3(a, b, c) DAG_CONCAT(a, DAG_CONCAT(b, c))

static SQInteger get_cpu_features(HSQUIRRELVM vm)
{
  using namespace cpu_features;
  auto features = CONCAT3(Get, CPUINFO_ARCH, Info)().features;
  using FeaturesEnum = DAG_CONCAT(CPUINFO_ARCH, FeaturesEnum);
  Sqrat::Array flist(vm);
  flist.Resize(32);
  int j = 0;
  for (int i = 0; i < (int)DAG_CONCAT(CPUINFO_ARCH_CAPS, _LAST_); i++)
    if (CONCAT3(Get, CPUINFO_ARCH, FeaturesEnumValue)(&features, FeaturesEnum(i)))
    {
      if (j++ < 32)
        flist.SetValue(j - 1, CONCAT3(Get, CPUINFO_ARCH, FeaturesEnumName)(FeaturesEnum(i)));
      else
        flist.Append(CONCAT3(Get, CPUINFO_ARCH, FeaturesEnumName)(FeaturesEnum(i)));
    }
  flist.Resize(j);
  Sqrat::PushVar(vm, flist);
  return 1;
}

namespace bindquirrel
{
void register_cpuinfo(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());
  exports.SquirrelFuncDeclString(get_cpu_features, "get_cpu_features(): array");
  module_mgr->addNativeModule("cpuinfo", exports);
}
} // namespace bindquirrel
#else
namespace bindquirrel
{
void register_cpuinfo(class SqModules *) {}
} // namespace bindquirrel
#endif // CPUINFO_ARCH
