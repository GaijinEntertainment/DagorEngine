//expect:w220

function foo(a){ //-declared-never-used
  foreach(x in a?.y()) {
    ::print(x)
  }
}