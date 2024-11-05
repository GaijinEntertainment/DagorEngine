//expect:w239

function returnBoolFunctionName() { //-declared-never-used
  if (::serverName == "")
    return

  return true;
}

//-file:undefined-global
