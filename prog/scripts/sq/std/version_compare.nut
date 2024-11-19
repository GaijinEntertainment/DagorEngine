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

function checkVersion(verWildcard, verCurrent, compFn) {
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

function stripVerCondition(str) {
  let allowed = "0123456789Xx"
  for (local i = 0; i < str.len(); ++i) {
    let char = str.slice(i, i + 1)
    if (allowed.contains(char))
      return str.slice(i)
  }
  return ""
}

function check_version_impl(vermask, game_version) {
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

function check_version(version1, version2) {
  if (version1 != ""
      && ("><=!".indexof(version1.slice(0, 1)) != null
        || version1.indexof("X") != null
        || version1.indexof("x") != null))
    return check_version_impl(version1, version2)
  return check_version_impl(version2, version1)
}

function test() {
  let testCfg = [
    ["1.2.3.4", "1.2.3.4"],
    ["1.2.x.4", "1.2.3.4"],
    ["1.2.3.X", "1.2.3.5"],
    ["1.2.3.4", "1.2.2.4", false],
    ["1.2.3.X", "1.3.3.4", false],

    ["1.2.3.4", ">=1.2.3.4"],
    ["1.2.4.4", ">=1.2.3.4"],
    ["1.2.3.3", ">=1.2.3.X"],
    ["1.2.3.3", ">=1.2.3.4", false],
    ["1.2.2.4", ">=1.2.3.X", false],

    ["1.2.3.5", ">1.2.3.4"],
    ["1.2.4.4", ">1.2.3.4"],
    ["1.3.3.4", ">1.2.3.4"],
    ["2.2.3.4", ">1.2.3.4"],
    ["1.2.3.2", ">1.2.3.4", false],
    ["1.2.3.4", ">1.2.*.4", false],

    ["1.2.3.4", "=1.2.3.4"],
    ["1.2.3.4", "=1.x.3.4"],
    ["1.2.3.4", "=1.3.3.4", false],
    ["1.2.3.5", "=1.3.*.4", false],

    ["2.2.3.4", "!1.2.3.4"],
    ["1.2.3.5", "!x.2.3.4"],
    ["1.2.3.4", "!1.2.3.4", false],
    ["1.2.3.4", "!X.2.3.4", false],
  ]

  foreach(cfg in testCfg) {
    let res = cfg?[2] ?? true
    assert(check_version(cfg[0], cfg[1]) == res)
    assert(check_version(cfg[1], cfg[0]) == res)
  }
}

return {
  check_version
  test
}
