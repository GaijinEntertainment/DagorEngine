//expect:w239

::userName <- require("sq3_sa_test").userName
::serverName <- require("sq3_sa_test").serverName


let function isLoggedIn() { //-declared-never-used
  if (::userName == "")
    return false;

  if (::serverName == "")
    return

  return true;
}
