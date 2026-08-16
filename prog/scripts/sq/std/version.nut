import "regexp2" as regexp2
from "dagor.debug" import logerr
from "types" import String, Array, Integer, Float, Null
let verTrim = regexp2(@"^\s+|\s+$")
let dotCase = regexp2(@"^\d+\.\d+\.\d+\.\d+$")
let dashCase = regexp2(@"^\d+_\d+_\d+_\d+$")

function mkVersionFromString(versionRaw): array|null {
  let version = verTrim.replace("", versionRaw)
  if (dotCase.match(version))
    return version.split(".")
  if (dashCase.match(version))
    return version.split("_")

  logerr($"CHANGELOG: Version string \"{versionRaw}\" has invalid format")
  return null
}

function mkVersionFromInt(version: int): array {
  return [version>>24, ((version>>16)&255), ((version>>8)&255), (version&255)]
}

function versionToInt(version): int {
  return version
    ? ((version[0]).tointeger() << 24) | ((version[1]).tointeger() << 16)
      | ((version[2]).tointeger() << 8) | (version[3]).tointeger()
    : -1
}

local class Version {
  version = null
  constructor(v){
    if (v instanceof String)
      this.version = mkVersionFromString(v)
    else if (v instanceof Array) {
      assert(v.len()==4)
      this.version = clone v
    }
    else if (v instanceof Integer) {
      this.version = mkVersionFromInt(v)
    }
    else if (v instanceof Float)
      this.version = mkVersionFromInt(v.tointeger())
    else if (v instanceof Null)
      this.version = [0,0,0,0]
    else {
      this.version = [0,0,0,0]
      assert(false, "type is not supported")
    }
  }
  function toint(): int {
    return versionToInt(this.version)
  }

  function tostring(): string {
    return this.version != null ? ".".join(this.version) : ""
  }
}

return {
  Version
  versionToInt
  mkVersionFromInt
  mkVersionFromString
}
