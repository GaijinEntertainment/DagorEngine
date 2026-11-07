#enable-optimizer

println("OPTIMIZER ON")

function test_opt_on() {
  for (local i = 0; i < (true ? 5 : 1); i++)
    println("X")
}

test_opt_on()
