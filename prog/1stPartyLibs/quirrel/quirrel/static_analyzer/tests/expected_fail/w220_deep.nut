//expect:w220

let function foo(a){ //-declared-never-used
  local container = a?.y()
  foreach(x in container) {
    print(x)
  }
}
