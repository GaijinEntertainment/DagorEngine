//expect:w239

::userName <- require("empty.txt")?.userName
::serverName <- require("empty.txt")?.serverName


let function isLoggedIn() { //-declared-never-used -all-paths-return-value
  if (::userName == "")
    return false;

  if (::serverName == "")
    return

  return true;
}
