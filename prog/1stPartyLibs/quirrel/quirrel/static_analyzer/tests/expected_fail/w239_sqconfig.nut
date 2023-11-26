//expect:w239

::serverName <- require("empty.txt")?.serverName


let function returnBoolFunctionName() { //-declared-never-used -all-paths-return-value
  if (::serverName == "")
    return

  return true;
}
