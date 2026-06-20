#allow-compiler-internals

function _foo(x) {
  if (x) {
    local a = $${ return 1 }
    return a
  }
  else {
    local a = $${ return 1 }
    return a
  }
}
