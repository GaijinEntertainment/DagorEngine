//expect:w239

let function isLoggedIn() { //-declared-never-used
  if (::userName == "")
    return false;

  if (::serverName == "")
    return

  return true;
}
