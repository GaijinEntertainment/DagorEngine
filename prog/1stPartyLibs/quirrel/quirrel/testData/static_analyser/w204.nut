//expect:w204

let function foo(x){ //-declared-never-used
  if (x & 15 == 8)
    ::print("ok")
}
