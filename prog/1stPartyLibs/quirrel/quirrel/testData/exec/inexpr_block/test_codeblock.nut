#allow-compiler-internals
let x = 0

try {

  let a = $${
    local t = 5 + x
    if (t > 4)
      t /= x
    return t
  }

  println(a)

} catch (e) {
  println(e)
}

