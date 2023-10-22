//expect:w239

let function returnBoolFunctionName() { //-declared-never-used
  if (::serverName == "")
    return

  return true;
}

//-file:undefined-global
