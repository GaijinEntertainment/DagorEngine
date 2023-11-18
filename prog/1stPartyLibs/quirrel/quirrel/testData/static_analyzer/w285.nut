 //-file:undefined-global
 //-file:declared-never-used


local uu = ::sys.gh("fff") ?? ""
if ((uu ?? "") != "")
  ::print($"x: {uu}")


local regions = ::unlock?.meta.regions ?? [::unlock?.meta.region] ?? []

local regions2 = ::x ? [] : {}
let _g = regions2 ?? 123


local regions3 = ::x ? 2 : 4
let _h = regions3 != null