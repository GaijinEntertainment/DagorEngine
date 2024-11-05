//expect:w217

function foo(x){ //-declared-never-used
  while (x) {
    if (::a == x)
      ::h(::a, x)

    return
  }
}

//-file:undefined-global
