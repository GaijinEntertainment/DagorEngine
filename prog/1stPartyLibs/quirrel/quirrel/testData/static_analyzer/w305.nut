if (__name__ == "__analysis__")
  return

const B = true
local x = 1
local y = 1

let b1 = true
let b2 = false

print(b1 > b2)

if (B == x > y)
  ::print("a")

if ((B == x) > y)
  ::print("a")