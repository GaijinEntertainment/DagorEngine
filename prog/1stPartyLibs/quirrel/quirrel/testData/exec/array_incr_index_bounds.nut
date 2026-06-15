// Regression: the array fast path for ++/-- (DerefInc) must reject out-of-range
// and negative indices instead of truncating the 64-bit index to 32 bits.
local a = [10, 20, 30]

// normal in-range increment/decrement still works
a[0]++
a[2]--
print("in-range: " + a[0] + " " + a[1] + " " + a[2] + "\n")

// a 33-bit index must NOT wrap to a valid slot (used to silently hit a[0])
local b = [10]
try { b[0x100000000]++; print("FAIL: no error\n") }
catch (e) { print("big index rejected: b[0]=" + b[0] + "\n") }

// negative index must error too
local c = [10]
try { c[-1]++; print("FAIL: no error\n") }
catch (e) { print("negative index rejected: c[0]=" + c[0] + "\n") }
