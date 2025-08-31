class D {
  x = 1
  y = 2
}
local d = D()
local t = {"1": "123", "2": "asd"}
local a = [123, 456, 789]

function fn(n) {
  d.swap("x", "y")
  local x = n - 1
  t.swap("1", "2")
  a.swap(1, 2)
  if (n > 0)
    fn(x)
  println(x)
}

fn(10)

println($"{d["x"]} {d["y"]}")
println($"{t["1"]} {t["2"]}")
println($"{a[1]} {a[2]}")
