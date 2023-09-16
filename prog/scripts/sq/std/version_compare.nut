let { logerr } = require("dagor.debug")
let {startsWith, toIntegerSafe} = require("string.nut")

let maskAny = { x = true, X = true, ["*"] = true }

let arrToInt = @(list) list.reduce(@(res, val)
  (res << 16) + toIntegerSafe(val), 0)

let typeMap = {
  undefined = @(a, b) b == a,
  [">="]    = @(a, b) b >= a,
  [">"]     = @(a, b) b > a,
  ["<="]    = @(a, b) b <= a,
  ["<"]     = @(a, b) b < a,
  ["="]     = @(a, b) b == a,
  ["!"]     = @(a, b) b != a,
}

let function checkVersion(verWildcard, verCurrent, compFn) {
  if (arrToInt(verCurrent) == 0)
    return true

  if (verWildcard.len() != verCurrent.len())
    return false

  let clrCheck = []
  let clrCurrent = []
  for (local i = 0; i < verWildcard.len(); ++i) {
    let subCheck = verWildcard[i]
    if (subCheck in maskAny)
      continue
    clrCheck.append(subCheck)
    clrCurrent.append(verCurrent[i])
  }
  let intCheck = arrToInt(clrCheck)
  let intCurrent = arrToInt(clrCurrent)
  return compFn(intCheck, intCurrent)
}

let function stripVerCondition(str) {
  let allowed = "0123456789Xx"
  for (local i = 0; i < str.len(); ++i) {
    let char = str.slice(i, i + 1)
    if (allowed.contains(char))
      return str.slice(i)
  }
  return ""
}

let function check_version(vermask, game_version) {
  if (type(vermask) != "string" || type(game_version) != "string") {
    logerr($"Try to call check_version with not string parameters. (vermask = {vermask}, game_version = {game_version})")
    return false
  }

  let maskVer = stripVerCondition(vermask).split(".")
  let checkVer = stripVerCondition(game_version).split(".")
  let cmpFn = typeMap.reduce(@(res, val, key)
    startsWith(vermask, key) ? val : res, typeMap.undefined)
  return checkVersion(maskVer, checkVer, cmpFn)
}

let function test() {
  assert(check_version("1.2.3.4", "1.2.3.4"))
  assert(check_version("1.2.x.4", "1.2.3.4"))
  assert(check_version("1.2.3.X", "1.2.3.5"))
  assert(!check_version("1.2.3.4", "1.2.2.4"))
  assert(!check_version("1.2.3.X", "1.3.3.4"))

  assert(check_version(">=1.2.3.4", "1.2.3.4"))
  assert(check_version(">=1.2.3.4", "1.2.4.4"))
  assert(check_version(">=1.2.3.X", "1.2.3.3"))
  assert(!check_version(">=1.2.3.4", "1.2.3.3"))
  assert(!check_version(">=1.2.3.X", "1.2.2.4"))

  assert(check_version(">1.2.3.4", "1.2.3.5"))
  assert(check_version(">1.2.3.4", "1.2.4.4"))
  assert(check_version(">1.2.3.4", "1.3.3.4"))
  assert(check_version(">1.2.3.4", "2.2.3.4"))
  assert(!check_version(">1.2.3.4", "1.2.3.2"))
  assert(!check_version(">1.2.*.4", "1.2.3.4"))

  assert(check_version("=1.2.3.4", "1.2.3.4"))
  assert(check_version("=1.x.3.4", "1.2.3.4"))
  assert(!check_version("=1.3.3.4", "1.2.3.4"))
  assert(!check_version("=1.3.*.4", "1.2.3.5"))

  assert(check_version("!1.2.3.4", "2.2.3.4"))
  assert(check_version("!x.2.3.4", "1.2.3.5"))
  assert(!check_version("!1.2.3.4", "1.2.3.4"))
  assert(!check_version("!X.2.3.4", "1.2.3.4"))
}

return {
  check_version
  test
}
