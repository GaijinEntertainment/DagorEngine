//-file:undefined-global

function _foo(x) {
  while (x) {
    if (::a == x)
      ::h(::a, x)

    throw "err"
  }
}
