#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>

#include <sqrat.h>

#include <gamePhys/collision/collisionLib.h>

namespace bindquirrel
{

static bool sq_dacoll_rayhit_normalized(const Point3 &from, const Point3 &dir, float t)
{
  return dacoll::rayhit_normalized(from, dir, t);
}

static SQInteger sq_dacoll_traceray_normalized(HSQUIRRELVM vm)
{
  HSQOBJECT hObjFrom, hObjDir;
  sq_getstackobj(vm, 2, &hObjFrom);
  sq_getstackobj(vm, 3, &hObjDir);

  if (!Sqrat::ClassType<Point3>::IsObjectOfClass(&hObjFrom) || !Sqrat::ClassType<Point3>::IsObjectOfClass(&hObjDir))
    return sq_throwerror(vm, "Source point and direction must be Point3 instances");

  Sqrat::Var<const Point3 *> varFrom(vm, 2);
  Sqrat::Var<const Point3 *> varDir(vm, 3);
  float t;
  sq_getfloat(vm, 4, &t);

  if (dacoll::traceray_normalized(*varFrom.value, *varDir.value, t, /*out_pmid*/ NULL, /*out_norm*/ NULL))
    sq_pushfloat(vm, t);
  else
    sq_pushnull(vm);

  return 1;
}

void sqrat_bind_dacoll_trace_lib(SqModules *module_mgr)
{
  G_ASSERT(module_mgr);
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Table nsTbl(vm);
  ///@module dacoll.trace
  nsTbl.Func("rayhit_normalized", sq_dacoll_rayhit_normalized)
    .SquirrelFunc("traceray_normalized", sq_dacoll_traceray_normalized, 4, ".xxf|i") // traceray_normalized(from:Point3, dir:Point3,
                                                                                     // dist:float) -> float|null
    ;

  module_mgr->addNativeModule("dacoll.trace", nsTbl);
}

} // namespace bindquirrel
