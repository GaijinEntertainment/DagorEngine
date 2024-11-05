//expect:w204

function foo(x){ //-declared-never-used
  if (x & 15 == 8)
    ::print("ok")
}
