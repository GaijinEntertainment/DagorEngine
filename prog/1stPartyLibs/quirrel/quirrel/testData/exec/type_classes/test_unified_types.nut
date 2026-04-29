from "types" import *

println("\nTest: instanceof with built-in types")
println("  5 instanceof Integer:", 5 instanceof Integer)
println("  3.14 instanceof Float:", 3.14 instanceof Float)
println("  true instanceof Bool:", true instanceof Bool)
println("  \"hello\" instanceof String:", "hello" instanceof String)
println("  [1,2,3] instanceof Array:", [1,2,3] instanceof Array)
println("  {a=1} instanceof Table:", {a=1} instanceof Table)
println("  null instanceof Null:", null instanceof Null)

println("\nTest: Cross-type checks (should be false)")
println("  5 instanceof String:", 5 instanceof String)
println("  \"hello\" instanceof Integer:", "hello" instanceof Integer)

println("\nTest: classof() function")
println("  classof(5)==Integer:", classof(5)==Integer)
println("  classof(\"hi\")==String:", classof("hi")==String)
println("  classof(true)==Bool:", classof(true)==Bool)
println("  classof([])==Array:", classof([])==Array)

println("\nTest: Type methods still work")
println("  \"hello\".len():", "hello".len())
println("  [1,2,3].len():", [1,2,3].len())

