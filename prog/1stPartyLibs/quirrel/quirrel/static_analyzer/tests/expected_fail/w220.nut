//expect:w220

let function foo(a){ //-declared-never-used
  foreach(x in a?.y()) {
    print(x)
  }
}