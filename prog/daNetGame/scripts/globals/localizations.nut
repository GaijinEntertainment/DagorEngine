from "dagor.debug" import logerr, console_print
from "dagor.system" import DBGLEVEL
from "debug" import getstackinfos
import "dagor.localize" as dagorLocalize
from "%sqstd/functools.nut" import memoize
import "console" as console
from "nestdb" import ndbTryRead, ndbWrite

let nativeLoc = dagorLocalize.loc
let {doesLocTextExist} = dagorLocalize

let debugLocalizations = DBGLEVEL > 0 && __name__ != "__main__" && "__argv" not in getroottable()
let unlocalizedStrings = persist("unlocalizedStrings", @() {})
let defLocalizedStrings = persist("defLocalizedStrings", @() {})
const __useLocalizations__ = "__useLocalizations__"
let readUseLocalizations = @() ndbTryRead("__useLocalizations__") ?? true

function locWithCheck(locId, ...) {
  if (locId==null)
    return null
  if (!readUseLocalizations())
     return locId
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
    let t = type(v)
    if (v==null)
      keys[idx] = ""
    else if (t in const (["string", "float", "integer", "boolean"].totable()))
      keys[idx] = v
    else {
      let r = []
      foreach (k, val in v) {
        r.append(k, val)
      }
      keys[idx] = "_".join(r) //better to replace with fast hash here
    }
  }

  return $"{locId}{keys[0]}{keys[1]}"
}

let persistLocCache = persist("persistLocCache", @(){})
let memoizedLoc = memoize(locWithCheck, hashLocFunc, persistLocCache)
let checkedLoc = @[pure](locId, defLoc=null, params=null) memoizedLoc(locId, defLoc, params)

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
console.register_command(function() {
  ndbWrite(__useLocalizations__, !readUseLocalizations())
  console_print($"localizations: {readUseLocalizations()}")
}, "localization.toggle", "toggle localizations on/off")

return freeze({
  nativeLoc,
  locCheckWrapper = checkedLoc,
  loc = debugLocalizations ? checkedLoc : nativeLoc
})