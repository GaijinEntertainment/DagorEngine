function fn(x) {
  x++
  println(static x)
  x++
  println(x)
}

fn(10)
fn(20)

require("static_reset1.nut")

fn(30)
fn(40)
