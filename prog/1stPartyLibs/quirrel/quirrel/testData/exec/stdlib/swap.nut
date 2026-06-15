class D {
  x = 1
  y = 2
}
local d = D()
local t = {"1": "123", "2": "asd"}
local a = [123, 456, 789]

d.swap("x", "y")
a.swap(1, 2)
t.swap("1", "2")

println($"{d["x"]} {d["y"]}")
println($"{t["1"]} {t["2"]}")
println($"{a[1]} {a[2]}")

// negative indices are supported (python-style)
local n = [10, 20, 30]
n.swap(0, -1)
println($"{n[0]} {n[1]} {n[2]}")

// large out-of-range index must throw, not silently truncate to 32-bit
local big = [1, 2, 3]
try {
  big.swap(0, 0x100000002) // low 32 bits = 2; must NOT swap index 2
  println("no error")
} catch (e) {
  println($"throws: {e}")
}
println($"{big[0]} {big[1]} {big[2]}")

local t2 = { a = 1, b = 2 }
local err = null
try {
  t2.swap("a", "nosuchkey")
} catch (e) {
  err = e
}
if (err)
  print("error thrown: " + err + "\n")
print("t2.a = " + t2.a + ", t2.b = " + t2.b + "\n")
if (err == null)
  print("BUG: swap() with missing key did not throw\n")
if (t2.a == null)
  print("BUG: t2.a was corrupted to null\n")
if (err != null && t2.a == 1)
  print("OK: swap threw and table intact\n")
