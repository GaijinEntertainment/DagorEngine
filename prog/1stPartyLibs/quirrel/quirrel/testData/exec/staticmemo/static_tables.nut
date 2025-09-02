local add = 0

function fn() {
  let x = static {a = 222 + add}
  println(x.a)

  let y = static({a = 222 + add})
  println(y.a)

  let z = static {a = 222 + add}.__update({a = 333 + add})
  println(z.a)

  let w = static {a = 222 + add}.__update(static {a = 333 + add})
  println(z.a)

  println("");
}

fn()
add = 1
fn()
