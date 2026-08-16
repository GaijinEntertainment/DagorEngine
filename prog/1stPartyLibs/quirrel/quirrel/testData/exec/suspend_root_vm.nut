println("before")
try {
  suspend()
}
catch (err) {
  println(err)
}
println("after")
