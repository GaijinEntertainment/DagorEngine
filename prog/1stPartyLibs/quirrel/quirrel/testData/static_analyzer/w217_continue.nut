//expect:w217

function foo(x) { //-declared-never-used
  do {
    if (::a == x)
      ::h(::a, x)
    continue;
    x--;
  } while (x)
}

//-file:undefined-global
