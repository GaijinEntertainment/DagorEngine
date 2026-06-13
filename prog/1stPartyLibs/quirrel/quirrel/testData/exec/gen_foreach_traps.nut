function gen() {
  try {
    yield 1
    yield 2
  }
  catch (e) {
    println("caught")
  }
}

foreach (v in gen())
  println(v)

println("done")
