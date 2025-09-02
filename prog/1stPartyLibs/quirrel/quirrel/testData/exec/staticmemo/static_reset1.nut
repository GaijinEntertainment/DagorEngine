let { reset_static_memos } = require("modules")

local a, s

function sw() { return a }


function func() {

  function xx1() {
    for (local i = 0; i < 1000; i++)
      s += (static(sw())) + (static sw())
  }

  function xx2() {
    for (local i = 0; i < 1000; i++)
      s += static sw()
  }

  s = 0
  xx1()
  xx2()
  println(s)


  s = 0
  xx1()
  xx2()
  println(s)

}

a = 1
func()

reset_static_memos()
println("")

a = 2
func()
