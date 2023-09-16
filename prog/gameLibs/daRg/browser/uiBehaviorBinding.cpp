#include "bhvBrowser.h"
#include <bindQuirrelEx/autoBind.h>

namespace darg
{

void bind_browser_behavior(HSQUIRRELVM vm)
{
  Sqrat::ConstTable constTbl(vm);

  Sqrat::Table tblBhv = Sqrat::Table(constTbl).RawGetSlot("Behaviors");
  if (tblBhv.IsNull())
    tblBhv = Sqrat::Table(vm);

  tblBhv.SetValue("Browser", (darg::Behavior *)&bhv_browser);

  Sqrat::Table(constTbl).SetValue("Behaviors", tblBhv);

  Sqrat::Class<BhvBrowserData, Sqrat::NoConstructor<BhvBrowserData>> sqBhvBrowserDataname(vm, "BhvBrowserData");
}

} // namespace darg
