let { logerr } = require("dagor.debug")
let { getstackinfos } = require("debug")
let { DBGLEVEL } = require("dagor.system")
let dagorLocalize = require("dagor.localize")
let nativeLoc = dagorLocalize.loc
let {doesLocTextExist} = dagorLocalize
let {memoize} = require("%sqstd/functools.nut")
let console = require("console")

let unlocalizedStrings = persist("unlocalizedStrings", @() {})
let defLocalizedStrings = persist("defLocalizedStrings", @() {})

function locWithCheck(locId, ...) {
  if (locId==null)
    return null

  local defaultLoc
  foreach (v in vargv) {
    if (type(v) == "string")
      defaultLoc = v
  }

  if (!doesLocTextExist(locId)){
    let {src=null, line=null} = getstackinfos(4)
    if (defaultLoc!=null && locId not in defLocalizedStrings)
      defLocalizedStrings[locId] <- {stack = src ? $"{src}:{line}" : "no stack", defaultLoc}
    else if (locId not in unlocalizedStrings)
      unlocalizedStrings[locId] <- $"{src}:{line}"
  }
  return nativeLoc.acall([null, locId].extend(vargv))
}

function hashLocFunc(locId, ...) {
  let keys = ["", ""]

  foreach (idx, v in vargv) {
    if (type(v)=="table")
      keys[idx] = v.reduce(@(a,val, key) "_".concat(a, key, val), "")
    else
      keys[idx] = v ?? ""
  }

  return $"{locId}{keys[0]}{keys[1]}"
}

let persistLocCache = persist("persistLocCache", @(){})
let memoizedLoc = memoize(locWithCheck, hashLocFunc, persistLocCache)
let checkedLoc = @(locId, defLoc=null, params=null) memoizedLoc(locId, defLoc, params)
let debugLocalizations = DBGLEVEL > 0 && __name__ != "__main__" && "__argv" not in getroottable()

function dumpLocalizationErrors(){
  let unlocalizedStringsN = unlocalizedStrings.len() + defLocalizedStrings.len()
  if (unlocalizedStringsN > 0) {
    logerr($"[LANG] {unlocalizedStringsN} strings has no localizations")
    print("[LANG] not localized strings\n")
    foreach(locId, stackinfo in unlocalizedStrings)
      if (stackinfo==null)
        print(locId)
      else
        print($"{locId}: {stackinfo}")
    print("[LANG] localized with default localizations\n")
    foreach(locId, info in defLocalizedStrings)
      print($"{locId}: defLoc = {info.defLoc}, stack = {info.stack}")
  }
}

console.register_command(dumpLocalizationErrors, "localization.checkErrors", "dump all unlocalized strings")

return {
  nativeLoc,
  locCheckWrapper = checkedLoc,
  loc = debugLocalizations ? checkedLoc : nativeLoc
}