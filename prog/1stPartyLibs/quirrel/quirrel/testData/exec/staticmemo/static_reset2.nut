let { reset_static_memos } = require("modules")

local a, s

function sw() { return a }


function func() {

  function xx() {
    s = 0
    for (local i = 0; i < 1000; i++)
      s += static sw()
  }

  reset_static_memos()
  reset_static_memos()

  a = 1
  xx()
  println(s)

  reset_static_memos()

  a = 2
  xx()
  println(s)

  reset_static_memos()
}

func()
func()
