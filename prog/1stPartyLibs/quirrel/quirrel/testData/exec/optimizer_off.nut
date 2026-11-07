#disable-optimizer

println("OPTIMIZER OFF")

function test_opt_off() {
  for (local i = 0; i < (true ? 5 : 1); i++)
    println("X")
}

test_opt_off()
