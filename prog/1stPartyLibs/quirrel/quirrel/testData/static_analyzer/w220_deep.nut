//expect:w220

function foo(a){ //-declared-never-used
  local container = a?.y()
  foreach(x in container) {
    ::print(x)
  }
}
