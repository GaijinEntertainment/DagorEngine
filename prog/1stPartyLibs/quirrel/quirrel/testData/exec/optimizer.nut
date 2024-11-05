println("OPTIMIZER OFF")

function test_opt_off() {
  #disable-optimizer
  for (local i = 0; i < (true ? 5 : 1); i++)
    println("X")
}

test_opt_off()


println("OPTIMIZER ON")

function test_opt_on() {
  #enable-optimizer
  for (local i = 0; i < (true ? 5 : 1); i++)
    println("X")
}

test_opt_on()
