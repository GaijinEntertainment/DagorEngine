from "types" import *

class ApiError { msg = "api" }
class NetError { msg = "net" }

// return out of a try body guarded by two typed clauses (POPTRAP must pop both)
function early() {
  try { return "early" }
  catch (ApiError e) { return "BUG api" }
  catch (e) { return "BUG all" }
}
println(early())

// break out of a loop from inside a two-clause typed try
foreach (i in [0, 1, 2]) {
  try {
    if (i == 1) break
    println("iter " + i)
  }
  catch (ApiError e) { println("BUG api") }
  catch (e) { println("BUG all") }
}

// continue from inside a typed try
foreach (i in [0, 1, 2]) {
  try {
    if (i == 1) continue
    println("body " + i)
  }
  catch (NetError e) { println("BUG net") }
}

// generator: yield inside a typed try, then a throw caught by a typed clause
function gen() {
  try {
    yield 1
    throw NetError()
  }
  catch (ApiError e) { yield -1 }
  catch (NetError e) { yield 99 }
}
local g = gen()
println(resume g)
println(resume g)
