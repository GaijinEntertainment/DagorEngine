let arr = [1]
arr.map(@(v) v) // the run time does not check nodiscard
println("OK")
