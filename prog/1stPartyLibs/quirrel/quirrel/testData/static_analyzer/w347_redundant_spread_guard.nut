//-file:declared-never-used

let WND_PARAMS = { a = 1 }
let BASE = [1, 2]

function mkWnd(wnd = null, extra = null, arr = null) {
  let plain     = { ...WND_PARAMS, ...wnd ?? {} }            //warning:redundant-spread-guard
  let inlined   = { ...WND_PARAMS, ...wnd ?? const {} }      //warning:redundant-spread-guard
  let parens    = { ...WND_PARAMS, ...(wnd ?? ({})) }        //warning:redundant-spread-guard
  let arrGuard  = [...BASE, ...arr ?? []]                    //warning:redundant-spread-guard
  let arrConst  = [...BASE, ...arr ?? const []]              //warning:redundant-spread-guard

  let keptTable = { ...WND_PARAMS, ...wnd ?? extra }         // ok, a real fallback
  let keptFull  = { ...WND_PARAMS, ...wnd ?? { b = 2 } }     // ok, the fallback adds something
  let keptBare  = { ...WND_PARAMS, ...wnd }                  // ok, nothing to drop
  let outside   = wnd ?? {}                                  // ok, not a spread
  return [plain, inlined, parens, arrGuard, arrConst, keptTable, keptFull, keptBare, outside]
}

println(mkWnd().len())
