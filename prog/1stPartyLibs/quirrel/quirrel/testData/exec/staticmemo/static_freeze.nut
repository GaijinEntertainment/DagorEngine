let t = static {x = null}
try {
  t.x = "fail"
  println(t.x)
} catch (e) {
  println("success")
}
