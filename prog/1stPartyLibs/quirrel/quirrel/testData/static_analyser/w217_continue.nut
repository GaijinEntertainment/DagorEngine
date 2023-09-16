//expect:w217

let function foo(x) { //-declared-never-used
  do {
    if (::a == x)
      ::h(::a, x)
    continue;
    x--;
  } while (x)
}
