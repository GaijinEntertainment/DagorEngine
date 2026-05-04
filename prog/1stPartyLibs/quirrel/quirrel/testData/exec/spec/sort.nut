// Basic ascending sort with comparator
let a = [4, 2, 5]
a.sort(@(a, b) a<=>b)
println("Basic ascending:")
a.each(@(v) println(v))

// Descending sort
let b = [4, 2, 5, 1, 3]
b.sort(@(a, b) b<=>a)
println("\nDescending:")
b.each(@(v) println(v))

// Default sort (no comparator)
let c = [8, 3, 9, 1, 5]
c.sort()
println("\nDefault sort:")
c.each(@(v) println(v))

// Sorting strings
let d = ["zebra", "apple", "mango", "banana"]
d.sort(@(a, b) a<=>b)
println("\nString sort:")
d.each(@(v) println(v))

// Already sorted array
let e = [1, 2, 3, 4, 5]
e.sort(@(a, b) a<=>b)
println("\nAlready sorted:")
e.each(@(v) println(v))

// Array with duplicates
let f = [3, 1, 4, 1, 5, 9, 2, 6, 5]
f.sort(@(a, b) a<=>b)
println("\nWith duplicates:")
f.each(@(v) println(v))

// Single element
let g = [42]
g.sort(@(a, b) a<=>b)
println("\nSingle element:")
g.each(@(v) println(v))

// Empty array
let h = []
h.sort(@(a, b) a<=>b)
println("\nEmpty array:")
h.each(@(v) println(v))

// Negative numbers
let i = [-5, 3, -2, 0, 7, -1]
i.sort(@(a, b) a<=>b)
println("\nNegative numbers:")
i.each(@(v) println(v))

// Floats
let j = [3.14, 2.71, 1.41, 2.5]
j.sort(@(a, b) a<=>b)
println("\nFloats:")
j.each(@(v) println(v))

// Custom comparator - sort by absolute value
let k = [-5, 3, -2, 0, 7, -1]
k.sort(@(a, b) (a < 0 ? -a : a) <=> (b < 0 ? -b : b))
println("\nBy absolute value:")
k.each(@(v) println(v))

// Sort array of tables by a field
let m = [
  {name = "Charlie", age = 25},
  {name = "Alice", age = 30},
  {name = "Bob", age = 20}
]
m.sort(@(a, b) a.age <=> b.age)
println("\nTables by age:")
m.each(@(v) println(v.name + ": " + v.age))

// Reverse sorted array
let n = [9, 7, 5, 3, 1]
n.sort(@(a, b) a<=>b)
println("\nReverse sorted input:")
n.each(@(v) println(v))

// Large array
let p = []
for (local x = 10; x >= 1; x--)
  p.append(x)
p.sort(@(a, b) a<=>b)
println("\nLarge array (10 elements):")
p.each(@(v) println(v))

// Resize is forbidden
println("\nTrying to resize during sorting:")
try {
  p.sort(function(a, b) {
    p.resize(17)
    return a<=>b
  })
} catch (e) {
  println(e)
}
