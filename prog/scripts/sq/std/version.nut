let {logerr} = require("dagor.debug")
let regexp2 = require("regexp2")

let verTrim = regexp2(@"^\s+|\s+$")
let dotCase = regexp2(@"^\d+\.\d+\.\d+\.\d+$")
let dashCase = regexp2(@"^\d+_\d+_\d+_\d+$")

function mkVersionFromString(versionRaw){
  let version = verTrim.replace("", versionRaw)
  if (dotCase.match(version))
    return version.split(".")
  if (dashCase.match(version))
    return version.split("_")

  logerr($"CHANGELOG: Version string \"{versionRaw}\" has invalid format")
  return null
}

function mkVersionFromInt(version){
  return [version>>24, ((version>>16)&255), ((version>>8)&255), (version&255)]
}

function versionToInt(version){
  return version
    ? ((version[0]).tointeger() << 24) | ((version[1]).tointeger() << 16)
      | ((version[2]).tointeger() << 8) | (version[3]).tointeger()
    : -1
}

local class Version {
  version = null
  constructor(v){
    let t = type(v)

    if (t == "string")
      this.version = mkVersionFromString(v)
    else if (t == "array") {
      assert(v.len()==4)
      this.version = clone v
    }
    else if (t =="integer") {
      this.version = mkVersionFromInt(v)
    }
    else if (t=="float")
      this.version = mkVersionFromInt(v.tointeger())
    else if (t=="null")
      this.version = [0,0,0,0]
    else {
      this.version = [0,0,0,0]
      assert(false, "type is not supported")
    }
  }
  function toint(){
    return versionToInt(this.version)
  }

  function tostring(){
    return ".".join(this.version)
  }
}

return {
  Version
  versionToInt
  mkVersionFromInt
  mkVersionFromString
}
