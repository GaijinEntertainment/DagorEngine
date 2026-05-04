//-file:undefined-global

function foo(x){ //-declared-never-used
  while (x) {
    if (::a == x)
      ::h(::a, x)

    return
  }
}
