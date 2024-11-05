//expect:w239

function isLoggedIn() { //-declared-never-used
  if (::userName == "")
    return false;

  if (::serverName == "")
    return

  return true;
}

//-file:undefined-global
