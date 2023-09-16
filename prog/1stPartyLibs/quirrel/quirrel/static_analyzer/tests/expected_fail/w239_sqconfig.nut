//expect:w239

::serverName <- require("sq3_sa_test").serverName


let function returnBoolFunctionName() { //-declared-never-used
  if (::serverName == "")
    return

  return true;
}
