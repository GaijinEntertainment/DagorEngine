local arr = [1, 2, 3, null, 1, 2, 3, null, 1, 8, 9, null]
local index = 0;

let function fn() {
  return arr[index++]
}

::x <- "z"

for (;local a := fn();)
  print(a)

while (local b := fn())
  print(b)


if (local f := fn())
  print(f)

if ((local w := fn()) != (local k := fn())) {
  print(w)
  print(k)
}


print($"={x}")
