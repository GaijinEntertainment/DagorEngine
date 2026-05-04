from "string" import *

local INT_MAX = (-1) >>> 1
local INT_MIN = -INT_MAX - 1
local BITS = format("%x", INT_MAX).len() * 4
local MAXBIT = BITS - 1  // 63 for 64-bit, 31 for 32-bit

println(format("INT_MAX = %d, INT_MIN = %d, BITS = %d\n", INT_MAX, INT_MIN, BITS))


local total = 0
local passed = 0
local failed = 0

function check_eq(got, expected, desc) {
  total++
  if (got == expected) {
    passed++
    println(format("  OK   %-62s == %20d", desc, got))
  } else {
    failed++
    println(format("  FAIL %-62s == %20d  (expected %d)", desc, got, expected))
  }
}



// ================================================================
//  SECTION 1: Tests with LOCAL VARIABLES
// ================================================================

println("==============================================================")
println("  SECTION 1: Shifts with local variables")
println("==============================================================\n")

// --- << (left shift) with local variables ---

println("\n[local <<] basic")
{
  local x, y
  x = 1;  y = 0;   check_eq(x << y,           1, "x=1  << y=0")
  x = 1;  y = 1;   check_eq(x << y,           2, "x=1  << y=1")
  x = 1;  y = 2;   check_eq(x << y,           4, "x=1  << y=2")
  x = 1;  y = 10;  check_eq(x << y,        1024, "x=1  << y=10")
  x = 3;  y = 4;   check_eq(x << y,          48, "x=3  << y=4")
  x = 0xFF; y = 8; check_eq(x << y,       65280, "x=0xFF << y=8")
}

println("\n[local <<] zero value")
{
  local x = 0, y
  y = 0;       check_eq(x << y, 0, "x=0 << y=0")
  y = 1;       check_eq(x << y, 0, "x=0 << y=1")
  y = MAXBIT;  check_eq(x << y, 0, format("x=0 << y=%d", MAXBIT))
  y = -1;      check_eq(x << y, 0, "x=0 << y=-1")
}

println("\n[local <<] negative shift (direction reversal)")
{
  local x, y
  x = 8;    y = -1;  check_eq(x << y,  4, "x=8    << y=-1")
  x = 8;    y = -2;  check_eq(x << y,  2, "x=8    << y=-2")
  x = 8;    y = -3;  check_eq(x << y,  1, "x=8    << y=-3")
  x = 1024; y = -10; check_eq(x << y,  1, "x=1024 << y=-10")
  x = 16;   y = -1;  check_eq(x << y,  8, "x=16   << y=-1")
}

println(format("\n[local <<] overflow (shift >= %d)", BITS))
{
  local x, y
  x = 1;  y = BITS;      check_eq(x << y, 0, format("x=1  << y=%d", BITS))
  x = 1;  y = BITS + 1;  check_eq(x << y, 0, format("x=1  << y=%d", BITS + 1))
  x = 1;  y = BITS * 3;  check_eq(x << y, 0, format("x=1  << y=%d", BITS * 3))
  x = -1; y = BITS;      check_eq(x << y, 0, format("x=-1 << y=%d", BITS))
  x = INT_MAX; y = BITS; check_eq(x << y, 0, format("x=INT_MAX << y=%d", BITS))
}

println("\n[local <<] negative value")
{
  local x = -1, y
  y = 0;       check_eq(x << y,      -1, "x=-1 << y=0")
  y = 1;       check_eq(x << y,      -2, "x=-1 << y=1")
  y = MAXBIT;  check_eq(x << y, INT_MIN, format("x=-1 << y=%d", MAXBIT))
}

println("\n[local <<] extreme values")
{
  local x, y
  x = INT_MAX; y = 0;     check_eq(x << y, INT_MAX, "x=INT_MAX << y=0")
  x = INT_MAX; y = 1;     check_eq(x << y,      -2, "x=INT_MAX << y=1")
  x = INT_MAX; y = BITS;  check_eq(x << y,       0, format("x=INT_MAX << y=%d", BITS))
  x = INT_MIN; y = 0;     check_eq(x << y, INT_MIN, "x=INT_MIN << y=0")
  x = INT_MIN; y = 1;     check_eq(x << y,       0, "x=INT_MIN << y=1")
  x = INT_MIN; y = BITS;  check_eq(x << y,       0, format("x=INT_MIN << y=%d", BITS))
  x = 1;  y = INT_MIN;    check_eq(x << y,       0, "x=1  << y=INT_MIN")
  x = -1; y = INT_MIN;    check_eq(x << y,      -1, "x=-1 << y=INT_MIN")
  x = 1;  y = INT_MAX;    check_eq(x << y,       0, "x=1  << y=INT_MAX")
  x = -1; y = INT_MAX;    check_eq(x << y,       0, "x=-1 << y=INT_MAX")
  x = INT_MAX; y = -1;    check_eq(x << y, INT_MAX / 2, "x=INT_MAX << y=-1")
  x = INT_MIN; y = -1;    check_eq(x << y, INT_MIN / 2, "x=INT_MIN << y=-1")  // arithmetic >> 1 of 0x800...0
  x = 1; y = -BITS;       check_eq(x << y, 0, format("x=1 << y=-%d", BITS))
  x = 1; y = -(BITS + 1); check_eq(x << y, 0, format("x=1 << y=-%d", BITS + 1))
  x = 1; y = -(BITS * 3); check_eq(x << y, 0, format("x=1 << y=-%d", BITS * 3))
}

println("\n[local <<] boundary 31/32/33 and 63/64/65")
{
  local x, y
  // positive shifts
  x = 1; y = 31; check_eq(x << y,  2147483648, "x=1  << y=31")
  x = 1; y = 32; check_eq(x << y,  4294967296, "x=1  << y=32")
  x = 1; y = 33; check_eq(x << y,  8589934592, "x=1  << y=33")
  x = 1; y = 63; check_eq(x << y,     INT_MIN, "x=1  << y=63")
  x = 1; y = 64; check_eq(x << y,           0, "x=1  << y=64")
  x = 1; y = 65; check_eq(x << y,           0, "x=1  << y=65")

  x = -1; y = 31; check_eq(x << y, -2147483648, "x=-1 << y=31")
  x = -1; y = 32; check_eq(x << y, -4294967296, "x=-1 << y=32")
  x = -1; y = 33; check_eq(x << y, -8589934592, "x=-1 << y=33")
  x = -1; y = 63; check_eq(x << y,     INT_MIN, "x=-1 << y=63")
  x = -1; y = 64; check_eq(x << y,           0, "x=-1 << y=64")
  x = -1; y = 65; check_eq(x << y,           0, "x=-1 << y=65")

  // negative shifts (reverse to >>)
  x = INT_MIN; y = -31; check_eq(x << y, -4294967296, "x=INT_MIN << y=-31")  // arithmetic >> 31
  x = INT_MIN; y = -32; check_eq(x << y, -2147483648, "x=INT_MIN << y=-32")
  x = INT_MIN; y = -33; check_eq(x << y, -1073741824, "x=INT_MIN << y=-33")
  x = INT_MIN; y = -63; check_eq(x << y,          -1, "x=INT_MIN << y=-63")
  x = INT_MIN; y = -64; check_eq(x << y,          -1, "x=INT_MIN << y=-64")
  x = INT_MIN; y = -65; check_eq(x << y,          -1, "x=INT_MIN << y=-65")
}


// --- >> (arithmetic right shift) with local variables ---

println("\n[local >>] basic")
{
  local x, y
  x = 8;    y = 0;  check_eq(x >> y,   8, "x=8    >> y=0")
  x = 8;    y = 1;  check_eq(x >> y,   4, "x=8    >> y=1")
  x = 8;    y = 2;  check_eq(x >> y,   2, "x=8    >> y=2")
  x = 8;    y = 3;  check_eq(x >> y,   1, "x=8    >> y=3")
  x = 8;    y = 4;  check_eq(x >> y,   0, "x=8    >> y=4")
  x = 1024; y = 10; check_eq(x >> y,   1, "x=1024 >> y=10")
  x = 0xFF00; y = 8; check_eq(x >> y, 255, "x=0xFF00 >> y=8")
}

println("\n[local >>] zero value")
{
  local x = 0, y
  y = 0;       check_eq(x >> y, 0, "x=0 >> y=0")
  y = 1;       check_eq(x >> y, 0, "x=0 >> y=1")
  y = MAXBIT;  check_eq(x >> y, 0, format("x=0 >> y=%d", MAXBIT))
  y = -1;      check_eq(x >> y, 0, "x=0 >> y=-1")
}

println("\n[local >>] negative shift (direction reversal)")
{
  local x, y
  x = 1; y = -1;  check_eq(x >> y,    2, "x=1 >> y=-1")
  x = 1; y = -2;  check_eq(x >> y,    4, "x=1 >> y=-2")
  x = 1; y = -10; check_eq(x >> y, 1024, "x=1 >> y=-10")
  x = 3; y = -4;  check_eq(x >> y,   48, "x=3 >> y=-4")
}

println("\n[local >>] overflow positive")
{
  local x, y
  x = 1;       y = BITS;      check_eq(x >> y, 0, format("x=1       >> y=%d", BITS))
  x = 1;       y = BITS + 1;  check_eq(x >> y, 0, format("x=1       >> y=%d", BITS + 1))
  x = 1;       y = BITS * 3;  check_eq(x >> y, 0, format("x=1       >> y=%d", BITS * 3))
  x = INT_MAX; y = BITS;      check_eq(x >> y, 0, format("x=INT_MAX >> y=%d", BITS))
}

println("\n[local >>] overflow negative (sign fill)")
{
  local x, y
  x = -1;      y = BITS;      check_eq(x >> y, -1, format("x=-1      >> y=%d", BITS))
  x = -1;      y = BITS + 1;  check_eq(x >> y, -1, format("x=-1      >> y=%d", BITS + 1))
  x = -1;      y = BITS * 3;  check_eq(x >> y, -1, format("x=-1      >> y=%d", BITS * 3))
  x = INT_MIN; y = BITS;      check_eq(x >> y, -1, format("x=INT_MIN >> y=%d", BITS))
  x = -42;     y = BITS;      check_eq(x >> y, -1, format("x=-42     >> y=%d", BITS))
}

println("\n[local >>] negative value")
{
  local x, y
  x = -1;      y = 1; check_eq(x >> y,          -1, "x=-1      >> y=1")
  x = -2;      y = 1; check_eq(x >> y,          -1, "x=-2      >> y=1")
  x = INT_MIN; y = 1; check_eq(x >> y, INT_MIN / 2, "x=INT_MIN >> y=1")
}

println("\n[local >>] extreme values")
{
  local x, y
  x = INT_MAX; y = 0;        check_eq(x >> y,  INT_MAX, "x=INT_MAX >> y=0")
  x = INT_MIN; y = 0;        check_eq(x >> y,  INT_MIN, "x=INT_MIN >> y=0")
  x = 1;       y = INT_MIN;  check_eq(x >> y,        0, "x=1       >> y=INT_MIN")
  x = -1;      y = INT_MIN;  check_eq(x >> y,       -1, "x=-1      >> y=INT_MIN")
  x = INT_MIN; y = INT_MIN;  check_eq(x >> y,       -1, "x=INT_MIN >> y=INT_MIN")
  x = 1;       y = INT_MAX;  check_eq(x >> y,        0, "x=1       >> y=INT_MAX")
  x = -1;      y = INT_MAX;  check_eq(x >> y,       -1, "x=-1      >> y=INT_MAX")
  x = 1;       y = -1;       check_eq(x >> y,        2, "x=1       >> y=-1")
  x = INT_MAX; y = -1;       check_eq(x >> y,       -2, "x=INT_MAX >> y=-1")
  x = 1;       y = -BITS;    check_eq(x >> y,        0, format("x=1  >> y=-%d", BITS))
  x = -1;      y = -BITS;    check_eq(x >> y,       -1, format("x=-1 >> y=-%d", BITS))
  x = 1;       y = -(BITS*3);check_eq(x >> y,        0, format("x=1  >> y=-%d", BITS * 3))
}

println("\n[local >>] boundary 31/32/33 and 63/64/65")
{
  local x, y
  x = 1; y = 31; check_eq(x >> y,           0, "x=1  >> y=31")
  x = 1; y = 32; check_eq(x >> y,           0, "x=1  >> y=32")
  x = 1; y = 33; check_eq(x >> y,           0, "x=1  >> y=33")
  x = 1; y = 63; check_eq(x >> y,           0, "x=1  >> y=63")
  x = 1; y = 64; check_eq(x >> y,           0, "x=1  >> y=64")
  x = 1; y = 65; check_eq(x >> y,           0, "x=1  >> y=65")

  x = -1; y = 31; check_eq(x >> y,          -1, "x=-1 >> y=31")
  x = -1; y = 32; check_eq(x >> y,          -1, "x=-1 >> y=32")
  x = -1; y = 33; check_eq(x >> y,          -1, "x=-1 >> y=33")
  x = -1; y = 63; check_eq(x >> y,          -1, "x=-1 >> y=63")
  x = -1; y = 64; check_eq(x >> y,          -1, "x=-1 >> y=64")
  x = -1; y = 65; check_eq(x >> y,          -1, "x=-1 >> y=65")

  x = INT_MIN; y = 31; check_eq(x >> y, -4294967296, "x=INT_MIN >> y=31")
  x = INT_MIN; y = 32; check_eq(x >> y, -2147483648, "x=INT_MIN >> y=32")
  x = INT_MIN; y = 33; check_eq(x >> y, -1073741824, "x=INT_MIN >> y=33")
  x = INT_MIN; y = 63; check_eq(x >> y,          -1, "x=INT_MIN >> y=63")
  x = INT_MIN; y = 64; check_eq(x >> y,          -1, "x=INT_MIN >> y=64")
  x = INT_MIN; y = 65; check_eq(x >> y,          -1, "x=INT_MIN >> y=65")

  // negative shifts (reverse to <<)
  x = 1; y = -31; check_eq(x >> y,  2147483648, "x=1  >> y=-31")
  x = 1; y = -32; check_eq(x >> y,  4294967296, "x=1  >> y=-32")
  x = 1; y = -33; check_eq(x >> y,  8589934592, "x=1  >> y=-33")
  x = 1; y = -63; check_eq(x >> y,     INT_MIN, "x=1  >> y=-63")
  x = 1; y = -64; check_eq(x >> y,           0, "x=1  >> y=-64")
  x = 1; y = -65; check_eq(x >> y,           0, "x=1  >> y=-65")

  x = INT_MIN; y = -31; check_eq(x >> y,   0, "x=INT_MIN >> y=-31")
  x = INT_MIN; y = -32; check_eq(x >> y,   0, "x=INT_MIN >> y=-32")
  x = INT_MIN; y = -33; check_eq(x >> y,   0, "x=INT_MIN >> y=-33")
  x = INT_MIN; y = -63; check_eq(x >> y,   0, "x=INT_MIN >> y=-63")
  x = INT_MIN; y = -64; check_eq(x >> y,  -1, "x=INT_MIN >> y=-64")
  x = INT_MIN; y = -65; check_eq(x >> y,  -1, "x=INT_MIN >> y=-65")
}


// --- >>> (unsigned right shift) with local variables ---

println("\n[local >>>] basic")
{
  local x, y
  x = 8;      y = 0; check_eq(x >>> y,   8, "x=8      >>> y=0")
  x = 8;      y = 1; check_eq(x >>> y,   4, "x=8      >>> y=1")
  x = 8;      y = 2; check_eq(x >>> y,   2, "x=8      >>> y=2")
  x = 8;      y = 3; check_eq(x >>> y,   1, "x=8      >>> y=3")
  x = 8;      y = 4; check_eq(x >>> y,   0, "x=8      >>> y=4")
  x = 0xFF00; y = 8; check_eq(x >>> y, 255, "x=0xFF00 >>> y=8")
}

println("\n[local >>>] zero value")
{
  local x = 0, y
  y = 0;       check_eq(x >>> y, 0, "x=0 >>> y=0")
  y = 1;       check_eq(x >>> y, 0, "x=0 >>> y=1")
  y = MAXBIT;  check_eq(x >>> y, 0, format("x=0 >>> y=%d", MAXBIT))
  y = -1;      check_eq(x >>> y, 0, "x=0 >>> y=-1")
}

println("\n[local >>>] negative shift (direction reversal)")
{
  local x, y
  x = 1; y = -1;  check_eq(x >>> y,    2, "x=1 >>> y=-1")
  x = 1; y = -2;  check_eq(x >>> y,    4, "x=1 >>> y=-2")
  x = 1; y = -10; check_eq(x >>> y, 1024, "x=1 >>> y=-10")
}

println(format("\n[local >>>] overflow (shift >= %d)", BITS))
{
  local x, y
  x = 1;       y = BITS;     check_eq(x >>> y, 0, format("x=1       >>> y=%d", BITS))
  x = 1;       y = BITS + 1; check_eq(x >>> y, 0, format("x=1       >>> y=%d", BITS + 1))
  x = -1;      y = BITS;     check_eq(x >>> y, 0, format("x=-1      >>> y=%d", BITS))
  x = INT_MIN; y = BITS;     check_eq(x >>> y, 0, format("x=INT_MIN >>> y=%d", BITS))
  x = INT_MAX; y = BITS;     check_eq(x >>> y, 0, format("x=INT_MAX >>> y=%d", BITS))
}

println("\n[local >>>] negative value")
{
  local x, y
  x = -1;      y = 1;       check_eq(x >>> y, INT_MAX, "x=-1      >>> y=1")
  x = INT_MIN; y = 1;       check_eq(x >>> y, 1 << (MAXBIT - 1), "x=INT_MIN >>> y=1")
  x = -1;      y = MAXBIT;  check_eq(x >>> y, 1, format("x=-1 >>> y=%d", MAXBIT))
}

println("\n[local >>>] extreme values")
{
  local x, y
  x = INT_MAX; y = 0;         check_eq(x >>> y,  INT_MAX, "x=INT_MAX >>> y=0")
  x = INT_MIN; y = 0;         check_eq(x >>> y,  INT_MIN, "x=INT_MIN >>> y=0")
  x = 1;       y = INT_MIN;   check_eq(x >>> y,        0, "x=1       >>> y=INT_MIN")
  x = -1;      y = INT_MIN;   check_eq(x >>> y,        0, "x=-1      >>> y=INT_MIN")
  x = INT_MIN; y = INT_MIN;   check_eq(x >>> y,        0, "x=INT_MIN >>> y=INT_MIN")
  x = 1;       y = INT_MAX;   check_eq(x >>> y,        0, "x=1       >>> y=INT_MAX")
  x = -1;      y = INT_MAX;   check_eq(x >>> y,        0, "x=-1      >>> y=INT_MAX")
  x = 1;       y = -1;        check_eq(x >>> y,        2, "x=1       >>> y=-1")
  x = INT_MAX; y = -1;        check_eq(x >>> y,       -2, "x=INT_MAX >>> y=-1")
  x = 1;       y = -BITS;     check_eq(x >>> y,        0, format("x=1  >>> y=-%d", BITS))
  x = -1;      y = -BITS;     check_eq(x >>> y,        0, format("x=-1 >>> y=-%d", BITS))
  x = 1;       y = -(BITS*3); check_eq(x >>> y,        0, format("x=1  >>> y=-%d", BITS * 3))
}

println("\n[local >>>] boundary 31/32/33 and 63/64/65")
{
  local x, y
  x = 1; y = 31; check_eq(x >>> y,           0, "x=1  >>> y=31")
  x = 1; y = 32; check_eq(x >>> y,           0, "x=1  >>> y=32")
  x = 1; y = 33; check_eq(x >>> y,           0, "x=1  >>> y=33")
  x = 1; y = 63; check_eq(x >>> y,           0, "x=1  >>> y=63")
  x = 1; y = 64; check_eq(x >>> y,           0, "x=1  >>> y=64")
  x = 1; y = 65; check_eq(x >>> y,           0, "x=1  >>> y=65")

  x = -1; y = 31; check_eq(x >>> y,  8589934591, "x=-1 >>> y=31")
  x = -1; y = 32; check_eq(x >>> y,  4294967295, "x=-1 >>> y=32")
  x = -1; y = 33; check_eq(x >>> y,  2147483647, "x=-1 >>> y=33")
  x = -1; y = 63; check_eq(x >>> y,           1, "x=-1 >>> y=63")
  x = -1; y = 64; check_eq(x >>> y,           0, "x=-1 >>> y=64")
  x = -1; y = 65; check_eq(x >>> y,           0, "x=-1 >>> y=65")

  x = INT_MIN; y = 31; check_eq(x >>> y,  4294967296, "x=INT_MIN >>> y=31")
  x = INT_MIN; y = 32; check_eq(x >>> y,  2147483648, "x=INT_MIN >>> y=32")
  x = INT_MIN; y = 33; check_eq(x >>> y,  1073741824, "x=INT_MIN >>> y=33")
  x = INT_MIN; y = 63; check_eq(x >>> y,           1, "x=INT_MIN >>> y=63")
  x = INT_MIN; y = 64; check_eq(x >>> y,           0, "x=INT_MIN >>> y=64")
  x = INT_MIN; y = 65; check_eq(x >>> y,           0, "x=INT_MIN >>> y=65")

  // negative shifts (reverse to <<)
  x = 1; y = -31; check_eq(x >>> y,  2147483648, "x=1  >>> y=-31")
  x = 1; y = -32; check_eq(x >>> y,  4294967296, "x=1  >>> y=-32")
  x = 1; y = -33; check_eq(x >>> y,  8589934592, "x=1  >>> y=-33")
  x = 1; y = -63; check_eq(x >>> y,     INT_MIN, "x=1  >>> y=-63")
  x = 1; y = -64; check_eq(x >>> y,           0, "x=1  >>> y=-64")
  x = 1; y = -65; check_eq(x >>> y,           0, "x=1  >>> y=-65")

  x = INT_MIN; y = -31; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-31")
  x = INT_MIN; y = -32; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-32")
  x = INT_MIN; y = -33; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-33")
  x = INT_MIN; y = -63; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-63")
  x = INT_MIN; y = -64; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-64")
  x = INT_MIN; y = -65; check_eq(x >>> y,  0, "x=INT_MIN >>> y=-65")
}

// --- cross-function with local variables ---

println("\n[local cross] left/right inverse")
{
  local x, y
  x = 1;  y = 10; check_eq((x << y) >> y,   1, "(x=1  << y=10) >> y=10")
  x = 42; y = 5;  check_eq((x << y) >> y,  42, "(x=42 << y=5)  >> y=5")
}

println(format("\n[local cross] shift by %d (boundary)", MAXBIT))
{
  local x, y
  x = 1;       y = MAXBIT; check_eq(x << y,   INT_MIN, format("x=1       << y=%d", MAXBIT))
  x = INT_MIN; y = MAXBIT; check_eq(x >> y,        -1, format("x=INT_MIN >> y=%d", MAXBIT))
  x = INT_MIN; y = MAXBIT; check_eq(x >>> y,        1, format("x=INT_MIN >>> y=%d", MAXBIT))
  x = -1;      y = MAXBIT; check_eq(x >> y,        -1, format("x=-1      >> y=%d", MAXBIT))
  x = -1;      y = MAXBIT; check_eq(x >>> y,        1, format("x=-1      >>> y=%d", MAXBIT))
}


// ================================================================
//  SECTION 2: Tests with NUMERIC CONSTANTS (literal expressions)
// ================================================================

println("\n==============================================================")
println("  SECTION 2: Shifts with numeric constants")
println("==============================================================\n")

// --- << with constants ---

println("\n[const expr <<] basic")
check_eq(1 << 0,           1, "1 << 0")
check_eq(1 << 1,           2, "1 << 1")
check_eq(1 << 2,           4, "1 << 2")
check_eq(1 << 10,       1024, "1 << 10")
check_eq(3 << 4,          48, "3 << 4")
check_eq(0xFF << 8,    65280, "0xFF << 8")

println("\n[const expr <<] zero value")
check_eq(0 << 0,  0, "0 << 0")
check_eq(0 << 1,  0, "0 << 1")
check_eq(0 << -1, 0, "0 << -1")

println("\n[const expr <<] negative shift")
check_eq(8 << -1,      4, "8 << -1")
check_eq(8 << -2,      2, "8 << -2")
check_eq(8 << -3,      1, "8 << -3")
check_eq(1024 << -10,  1, "1024 << -10")
check_eq(16 << -1,     8, "16 << -1")

println("\n[const expr <<] negative value")
check_eq(-1 << 0,  -1, "-1 << 0")
check_eq(-1 << 1,  -2, "-1 << 1")

println("\n[const expr <<] 32-bit range")
// These are valid (non-overflow) shifts in 64-bit
check_eq(1 << 31,   2147483648,    "1 << 31")
check_eq(1 << 32,   4294967296,    "1 << 32")
check_eq(-1 << 31, -2147483648,    "-1 << 31")
check_eq(-1 << 32, -4294967296,    "-1 << 32")

println("\n[const expr <<] overflow (shift >= BITS)")
{
  local x
  x = 1;  check_eq(x << BITS,       0, format("1 << %d", BITS))
  x = -1; check_eq(x << BITS,       0, format("-1 << %d", BITS))
  x = 1;  check_eq(x << (BITS + 1), 0, format("1 << %d", BITS + 1))
}

println("\n[const expr <<] extreme")
check_eq(1 << -32,   0, "1 << -32")
check_eq(1 << -33,   0, "1 << -33")
check_eq(1 << -100,  0, "1 << -100")
{
  local x
  x = INT_MAX; check_eq(x << 0,  INT_MAX, "INT_MAX << 0")
  x = INT_MAX; check_eq(x << 1,       -2, "INT_MAX << 1")
  x = INT_MIN; check_eq(x << 0,  INT_MIN, "INT_MIN << 0")
  x = INT_MIN; check_eq(x << 1,        0, "INT_MIN << 1")
}

println("\n[const expr <<] boundary 31/32/33 and 63/64/65")
check_eq(1 << 33,   8589934592,  "1 << 33")
check_eq(1 << 63,   INT_MIN,     "1 << 63")
check_eq(1 << 64,   0,           "1 << 64")
check_eq(1 << 65,   0,           "1 << 65")
check_eq(-1 << 33, -8589934592,  "-1 << 33")
check_eq(-1 << 63,  INT_MIN,     "-1 << 63")
check_eq(-1 << 64,  0,           "-1 << 64")
check_eq(-1 << 65,  0,           "-1 << 65")
{
  local x
  x = INT_MIN; check_eq(x << -31, -4294967296, "INT_MIN << -31")
  x = INT_MIN; check_eq(x << -32, -2147483648, "INT_MIN << -32")
  x = INT_MIN; check_eq(x << -33, -1073741824, "INT_MIN << -33")
  x = INT_MIN; check_eq(x << -63,          -1, "INT_MIN << -63")
  x = INT_MIN; check_eq(x << -64,          -1, "INT_MIN << -64")
  x = INT_MIN; check_eq(x << -65,          -1, "INT_MIN << -65")
}

// --- >> with constants ---

println("\n[const expr >>] basic")
check_eq(8 >> 0,       8, "8 >> 0")
check_eq(8 >> 1,       4, "8 >> 1")
check_eq(8 >> 2,       2, "8 >> 2")
check_eq(8 >> 3,       1, "8 >> 3")
check_eq(8 >> 4,       0, "8 >> 4")
check_eq(1024 >> 10,   1, "1024 >> 10")
check_eq(0xFF00 >> 8, 255, "0xFF00 >> 8")

println("\n[const expr >>] zero value")
check_eq(0 >> 0,  0, "0 >> 0")
check_eq(0 >> 1,  0, "0 >> 1")
check_eq(0 >> -1, 0, "0 >> -1")

println("\n[const expr >>] negative shift")
check_eq(1 >> -1,     2, "1 >> -1")
check_eq(1 >> -2,     4, "1 >> -2")
check_eq(1 >> -10, 1024, "1 >> -10")
check_eq(3 >> -4,    48, "3 >> -4")

println("\n[const expr >>] overflow")
{
  local x
  x = 1;       check_eq(x >> BITS,       0, format("1       >> %d", BITS))
  x = 1;       check_eq(x >> (BITS + 1), 0, format("1       >> %d", BITS + 1))
  x = INT_MAX; check_eq(x >> BITS,       0, format("INT_MAX >> %d", BITS))
  x = -1;      check_eq(x >> BITS,      -1, format("-1      >> %d", BITS))
  x = -1;      check_eq(x >> (BITS + 1),-1, format("-1      >> %d", BITS + 1))
  x = INT_MIN; check_eq(x >> BITS,      -1, format("INT_MIN >> %d", BITS))
  x = -42;     check_eq(x >> BITS,      -1, format("-42     >> %d", BITS))
}

println("\n[const expr >>] negative value")
{
  local x
  x = -1;      check_eq(x >> 1,          -1, "-1      >> 1")
  x = -2;      check_eq(x >> 1,          -1, "-2      >> 1")
  x = INT_MIN; check_eq(x >> 1, INT_MIN / 2, "INT_MIN >> 1")
}

println("\n[const expr >>] extreme")
check_eq(1 >> -32,  4294967296, "1 >> -32")   // reverses to 1 << 32, valid in 64-bit
check_eq(1 >> -100,          0, "1 >> -100")  // reverses to 1 << 100, overflow
{
  local x
  x = INT_MAX; check_eq(x >> 0, INT_MAX, "INT_MAX >> 0")
  x = INT_MIN; check_eq(x >> 0, INT_MIN, "INT_MIN >> 0")
}

println("\n[const expr >>] boundary 31/32/33 and 63/64/65")
{
  local x
  // x=-1 positive shifts
  x = -1; check_eq(x >> 31,          -1, "-1 >> 31")
  x = -1; check_eq(x >> 32,          -1, "-1 >> 32")
  x = -1; check_eq(x >> 33,          -1, "-1 >> 33")
  x = -1; check_eq(x >> 63,          -1, "-1 >> 63")
  x = -1; check_eq(x >> 64,          -1, "-1 >> 64")
  x = -1; check_eq(x >> 65,          -1, "-1 >> 65")

  x = INT_MIN; check_eq(x >> 31, -4294967296, "INT_MIN >> 31")
  x = INT_MIN; check_eq(x >> 32, -2147483648, "INT_MIN >> 32")
  x = INT_MIN; check_eq(x >> 33, -1073741824, "INT_MIN >> 33")
  x = INT_MIN; check_eq(x >> 63,          -1, "INT_MIN >> 63")
  x = INT_MIN; check_eq(x >> 64,          -1, "INT_MIN >> 64")
  x = INT_MIN; check_eq(x >> 65,          -1, "INT_MIN >> 65")

  // negative shifts (reverse to <<)
  x = 1; check_eq(x >> -31,  2147483648, "1 >> -31")
  x = 1; check_eq(x >> -33,  8589934592, "1 >> -33")
  x = 1; check_eq(x >> -63,     INT_MIN, "1 >> -63")
  x = 1; check_eq(x >> -64,           0, "1 >> -64")
  x = 1; check_eq(x >> -65,           0, "1 >> -65")

  x = INT_MIN; check_eq(x >> -31,  0, "INT_MIN >> -31")
  x = INT_MIN; check_eq(x >> -32,  0, "INT_MIN >> -32")
  x = INT_MIN; check_eq(x >> -33,  0, "INT_MIN >> -33")
  x = INT_MIN; check_eq(x >> -63,  0, "INT_MIN >> -63")
  x = INT_MIN; check_eq(x >> -64, -1, "INT_MIN >> -64")
  x = INT_MIN; check_eq(x >> -65, -1, "INT_MIN >> -65")
}

// --- >>> with constants ---

println("\n[const expr >>>] basic")
check_eq(8 >>> 0,       8, "8 >>> 0")
check_eq(8 >>> 1,       4, "8 >>> 1")
check_eq(8 >>> 2,       2, "8 >>> 2")
check_eq(8 >>> 3,       1, "8 >>> 3")
check_eq(8 >>> 4,       0, "8 >>> 4")
check_eq(0xFF00 >>> 8, 255, "0xFF00 >>> 8")

println("\n[const expr >>>] zero value")
check_eq(0 >>> 0,  0, "0 >>> 0")
check_eq(0 >>> 1,  0, "0 >>> 1")
check_eq(0 >>> -1, 0, "0 >>> -1")

println("\n[const expr >>>] negative shift")
check_eq(1 >>> -1,     2, "1 >>> -1")
check_eq(1 >>> -2,     4, "1 >>> -2")
check_eq(1 >>> -10, 1024, "1 >>> -10")

println("\n[const expr >>>] overflow")
{
  local x
  x = 1;       check_eq(x >>> BITS,       0, format("1       >>> %d", BITS))
  x = 1;       check_eq(x >>> (BITS + 1), 0, format("1       >>> %d", BITS + 1))
  x = -1;      check_eq(x >>> BITS,       0, format("-1      >>> %d", BITS))
  x = INT_MIN; check_eq(x >>> BITS,       0, format("INT_MIN >>> %d", BITS))
  x = INT_MAX; check_eq(x >>> BITS,       0, format("INT_MAX >>> %d", BITS))
}

println("\n[const expr >>>] negative value")
{
  local x
  x = -1;      check_eq(x >>> 1,       INT_MAX, "-1      >>> 1")
  x = INT_MIN; check_eq(x >>> 1,       1 << (MAXBIT - 1), "INT_MIN >>> 1")
  x = -1;      check_eq(x >>> MAXBIT,  1, format("-1 >>> %d", MAXBIT))
}

println("\n[const expr >>>] extreme")
check_eq(1 >>> -32,  4294967296, "1 >>> -32")   // reverses to 1 << 32, valid in 64-bit
check_eq(1 >>> -100,          0, "1 >>> -100")  // reverses to 1 << 100, overflow
{
  local x
  x = INT_MAX; check_eq(x >>> 0, INT_MAX, "INT_MAX >>> 0")
  x = INT_MIN; check_eq(x >>> 0, INT_MIN, "INT_MIN >>> 0")
}

println("\n[const expr >>>] boundary 31/32/33 and 63/64/65")
{
  local x
  x = -1; check_eq(x >>> 31,  8589934591, "-1 >>> 31")
  x = -1; check_eq(x >>> 32,  4294967295, "-1 >>> 32")
  x = -1; check_eq(x >>> 33,  2147483647, "-1 >>> 33")
  x = -1; check_eq(x >>> 63,           1, "-1 >>> 63")
  x = -1; check_eq(x >>> 64,           0, "-1 >>> 64")
  x = -1; check_eq(x >>> 65,           0, "-1 >>> 65")

  x = INT_MIN; check_eq(x >>> 31,  4294967296, "INT_MIN >>> 31")
  x = INT_MIN; check_eq(x >>> 32,  2147483648, "INT_MIN >>> 32")
  x = INT_MIN; check_eq(x >>> 33,  1073741824, "INT_MIN >>> 33")
  x = INT_MIN; check_eq(x >>> 63,           1, "INT_MIN >>> 63")
  x = INT_MIN; check_eq(x >>> 64,           0, "INT_MIN >>> 64")
  x = INT_MIN; check_eq(x >>> 65,           0, "INT_MIN >>> 65")

  // negative shifts (reverse to <<)
  x = 1; check_eq(x >>> -31,  2147483648, "1 >>> -31")
  x = 1; check_eq(x >>> -33,  8589934592, "1 >>> -33")
  x = 1; check_eq(x >>> -63,     INT_MIN, "1 >>> -63")
  x = 1; check_eq(x >>> -64,           0, "1 >>> -64")
  x = 1; check_eq(x >>> -65,           0, "1 >>> -65")

  x = INT_MIN; check_eq(x >>> -31,  0, "INT_MIN >>> -31")
  x = INT_MIN; check_eq(x >>> -32,  0, "INT_MIN >>> -32")
  x = INT_MIN; check_eq(x >>> -33,  0, "INT_MIN >>> -33")
  x = INT_MIN; check_eq(x >>> -63,  0, "INT_MIN >>> -63")
  x = INT_MIN; check_eq(x >>> -64,  0, "INT_MIN >>> -64")
  x = INT_MIN; check_eq(x >>> -65,  0, "INT_MIN >>> -65")
}

// --- cross with constants ---

println(format("\n[const expr cross] shift by %d", MAXBIT))
{
  local x
  x = 1;       check_eq(x << MAXBIT,   INT_MIN, format("1       << %d", MAXBIT))
  x = INT_MIN; check_eq(x >> MAXBIT,        -1, format("INT_MIN >> %d", MAXBIT))
  x = INT_MIN; check_eq(x >>> MAXBIT,        1, format("INT_MIN >>> %d", MAXBIT))
  x = -1;      check_eq(x >> MAXBIT,        -1, format("-1      >> %d", MAXBIT))
  x = -1;      check_eq(x >>> MAXBIT,        1, format("-1      >>> %d", MAXBIT))
}


// ================================================================
//  SECTION 3: Compile-time CONST assignments (const X = literal op literal)
// ================================================================

println("\n==============================================================")
println("  SECTION 3: Shifts assigned to const (compile-time folding)")
println("==============================================================\n")

// --- << assigned to const ---

println("\n[const <<] basic")
const SHL_1_0    = 1 << 0
const SHL_1_1    = 1 << 1
const SHL_1_2    = 1 << 2
const SHL_1_10   = 1 << 10
const SHL_3_4    = 3 << 4
const SHL_FF_8   = 0xFF << 8
check_eq(SHL_1_0,       1, "const = 1 << 0")
check_eq(SHL_1_1,       2, "const = 1 << 1")
check_eq(SHL_1_2,       4, "const = 1 << 2")
check_eq(SHL_1_10,   1024, "const = 1 << 10")
check_eq(SHL_3_4,      48, "const = 3 << 4")
check_eq(SHL_FF_8,  65280, "const = 0xFF << 8")

println("\n[const <<] zero value")
const SHL_0_0  = 0 << 0
const SHL_0_1  = 0 << 1
const SHL_0_N1 = 0 << -1
check_eq(SHL_0_0,  0, "const = 0 << 0")
check_eq(SHL_0_1,  0, "const = 0 << 1")
check_eq(SHL_0_N1, 0, "const = 0 << -1")

println("\n[const <<] negative shift")
const SHL_8_N1     = 8 << -1
const SHL_8_N2     = 8 << -2
const SHL_8_N3     = 8 << -3
const SHL_1024_N10 = 1024 << -10
const SHL_16_N1    = 16 << -1
check_eq(SHL_8_N1,     4, "const = 8 << -1")
check_eq(SHL_8_N2,     2, "const = 8 << -2")
check_eq(SHL_8_N3,     1, "const = 8 << -3")
check_eq(SHL_1024_N10, 1, "const = 1024 << -10")
check_eq(SHL_16_N1,    8, "const = 16 << -1")

println("\n[const <<] negative value")
const SHL_N1_0  = -1 << 0
const SHL_N1_1  = -1 << 1
check_eq(SHL_N1_0,  -1, "const = -1 << 0")
check_eq(SHL_N1_1,  -2, "const = -1 << 1")

println("\n[const <<] 32-bit range (valid in 64-bit)")
const SHL_1_31    = 1 << 31
const SHL_1_32    = 1 << 32
const SHL_N1_31   = -1 << 31
const SHL_N1_32   = -1 << 32
check_eq(SHL_1_31,   2147483648,  "const = 1 << 31")
check_eq(SHL_1_32,   4294967296,  "const = 1 << 32")
check_eq(SHL_N1_31, -2147483648,  "const = -1 << 31")
check_eq(SHL_N1_32, -4294967296,  "const = -1 << 32")

println("\n[const <<] overflow")
const SHL_1_N32   = 1 << -32
const SHL_1_N33   = 1 << -33
const SHL_1_N100  = 1 << -100
check_eq(SHL_1_N32,  0, "const = 1 << -32")
check_eq(SHL_1_N33,  0, "const = 1 << -33")
check_eq(SHL_1_N100, 0, "const = 1 << -100")

println("\n[const <<] boundary 33/63/64/65")
const SHL_1_33    = 1 << 33
const SHL_1_63    = 1 << 63
const SHL_1_64    = 1 << 64
const SHL_1_65    = 1 << 65
const SHL_N1_33   = -1 << 33
const SHL_N1_63   = -1 << 63
const SHL_N1_64   = -1 << 64
const SHL_N1_65   = -1 << 65
check_eq(SHL_1_33,   8589934592,  "const = 1 << 33")
check_eq(SHL_1_63,   INT_MIN,     "const = 1 << 63")
check_eq(SHL_1_64,   0,           "const = 1 << 64")
check_eq(SHL_1_65,   0,           "const = 1 << 65")
check_eq(SHL_N1_33, -8589934592,  "const = -1 << 33")
check_eq(SHL_N1_63,  INT_MIN,     "const = -1 << 63")
check_eq(SHL_N1_64,  0,           "const = -1 << 64")
check_eq(SHL_N1_65,  0,           "const = -1 << 65")

const SHL_1_N31   = 1 << -31
const SHL_1_N63   = 1 << -63
const SHL_1_N64   = 1 << -64
const SHL_1_N65   = 1 << -65
check_eq(SHL_1_N31,  0, "const = 1 << -31")
check_eq(SHL_1_N63,  0, "const = 1 << -63")
check_eq(SHL_1_N64,  0, "const = 1 << -64")
check_eq(SHL_1_N65,  0, "const = 1 << -65")

// --- >> assigned to const ---

println("\n[const >>] basic")
const SHR_8_0      = 8 >> 0
const SHR_8_1      = 8 >> 1
const SHR_8_2      = 8 >> 2
const SHR_8_3      = 8 >> 3
const SHR_8_4      = 8 >> 4
const SHR_1024_10  = 1024 >> 10
const SHR_FF00_8   = 0xFF00 >> 8
check_eq(SHR_8_0,       8, "const = 8 >> 0")
check_eq(SHR_8_1,       4, "const = 8 >> 1")
check_eq(SHR_8_2,       2, "const = 8 >> 2")
check_eq(SHR_8_3,       1, "const = 8 >> 3")
check_eq(SHR_8_4,       0, "const = 8 >> 4")
check_eq(SHR_1024_10,   1, "const = 1024 >> 10")
check_eq(SHR_FF00_8,  255, "const = 0xFF00 >> 8")

println("\n[const >>] zero value")
const SHR_0_0  = 0 >> 0
const SHR_0_1  = 0 >> 1
const SHR_0_N1 = 0 >> -1
check_eq(SHR_0_0,  0, "const = 0 >> 0")
check_eq(SHR_0_1,  0, "const = 0 >> 1")
check_eq(SHR_0_N1, 0, "const = 0 >> -1")

println("\n[const >>] negative shift")
const SHR_1_N1  = 1 >> -1
const SHR_1_N2  = 1 >> -2
const SHR_1_N10 = 1 >> -10
const SHR_3_N4  = 3 >> -4
check_eq(SHR_1_N1,     2, "const = 1 >> -1")
check_eq(SHR_1_N2,     4, "const = 1 >> -2")
check_eq(SHR_1_N10, 1024, "const = 1 >> -10")
check_eq(SHR_3_N4,    48, "const = 3 >> -4")

println("\n[const >>] negative shift (large)")
const SHR_1_N32   = 1 >> -32   // reverses to 1 << 32, valid in 64-bit
const SHR_1_N100  = 1 >> -100  // reverses to 1 << 100, overflow in 64-bit
check_eq(SHR_1_N32,  4294967296, "const = 1 >> -32")
check_eq(SHR_1_N100,          0, "const = 1 >> -100")

println("\n[const >>] 32-bit range (valid in 64-bit)")
const SHR_BIG_31 = 4294967296 >> 31   // 2^32 >> 31 = 2
const SHR_BIG_32 = 4294967296 >> 32   // 2^32 >> 32 = 1
check_eq(SHR_BIG_31,  2, "const = 4294967296 >> 31")
check_eq(SHR_BIG_32,  1, "const = 4294967296 >> 32")

println("\n[const >>] boundary 33/63/64/65")
const SHR_BIG_33  = 4294967296 >> 33   // 2^32 >> 33 = 0 (shifted past)
const SHR_BIG2_31 = 8589934592 >> 31   // 2^33 >> 31 = 4
const SHR_BIG2_32 = 8589934592 >> 32   // 2^33 >> 32 = 2
const SHR_BIG2_33 = 8589934592 >> 33   // 2^33 >> 33 = 1
check_eq(SHR_BIG_33,   0, "const = 4294967296 >> 33")
check_eq(SHR_BIG2_31,  4, "const = 8589934592 >> 31")
check_eq(SHR_BIG2_32,  2, "const = 8589934592 >> 32")
check_eq(SHR_BIG2_33,  1, "const = 8589934592 >> 33")

const SHR_N1_31   = -1 >> 31
const SHR_N1_32   = -1 >> 32
const SHR_N1_33   = -1 >> 33
const SHR_N1_63   = -1 >> 63
const SHR_N1_64   = -1 >> 64
const SHR_N1_65   = -1 >> 65
check_eq(SHR_N1_31,          -1, "const = -1 >> 31")
check_eq(SHR_N1_32,          -1, "const = -1 >> 32")
check_eq(SHR_N1_33,          -1, "const = -1 >> 33")
check_eq(SHR_N1_63,          -1, "const = -1 >> 63")
check_eq(SHR_N1_64,          -1, "const = -1 >> 64")
check_eq(SHR_N1_65,          -1, "const = -1 >> 65")

const SHR_1_N31   = 1 >> -31
const SHR_1_N33   = 1 >> -33
const SHR_1_N63   = 1 >> -63
const SHR_1_N64   = 1 >> -64
const SHR_1_N65   = 1 >> -65
check_eq(SHR_1_N31,  2147483648, "const = 1 >> -31")
check_eq(SHR_1_N33,  8589934592, "const = 1 >> -33")
check_eq(SHR_1_N63,     INT_MIN, "const = 1 >> -63")
check_eq(SHR_1_N64,           0, "const = 1 >> -64")
check_eq(SHR_1_N65,           0, "const = 1 >> -65")

// --- >>> assigned to const ---

println("\n[const >>>] basic")
const USHR_8_0     = 8 >>> 0
const USHR_8_1     = 8 >>> 1
const USHR_8_2     = 8 >>> 2
const USHR_8_3     = 8 >>> 3
const USHR_8_4     = 8 >>> 4
const USHR_FF00_8  = 0xFF00 >>> 8
check_eq(USHR_8_0,       8, "const = 8 >>> 0")
check_eq(USHR_8_1,       4, "const = 8 >>> 1")
check_eq(USHR_8_2,       2, "const = 8 >>> 2")
check_eq(USHR_8_3,       1, "const = 8 >>> 3")
check_eq(USHR_8_4,       0, "const = 8 >>> 4")
check_eq(USHR_FF00_8,  255, "const = 0xFF00 >>> 8")

println("\n[const >>>] zero value")
const USHR_0_0  = 0 >>> 0
const USHR_0_1  = 0 >>> 1
const USHR_0_N1 = 0 >>> -1
check_eq(USHR_0_0,  0, "const = 0 >>> 0")
check_eq(USHR_0_1,  0, "const = 0 >>> 1")
check_eq(USHR_0_N1, 0, "const = 0 >>> -1")

println("\n[const >>>] negative shift")
const USHR_1_N1  = 1 >>> -1
const USHR_1_N2  = 1 >>> -2
const USHR_1_N10 = 1 >>> -10
check_eq(USHR_1_N1,     2, "const = 1 >>> -1")
check_eq(USHR_1_N2,     4, "const = 1 >>> -2")
check_eq(USHR_1_N10, 1024, "const = 1 >>> -10")

println("\n[const >>>] negative shift (large)")
const USHR_1_N32   = 1 >>> -32   // reverses to 1 << 32, valid in 64-bit
const USHR_1_N100  = 1 >>> -100  // reverses to 1 << 100, overflow in 64-bit
check_eq(USHR_1_N32,  4294967296, "const = 1 >>> -32")
check_eq(USHR_1_N100,          0, "const = 1 >>> -100")

println("\n[const >>>] 32-bit range (valid in 64-bit)")
const USHR_BIG_31 = 4294967296 >>> 31   // 2^32 >>> 31 = 2
const USHR_BIG_32 = 4294967296 >>> 32   // 2^32 >>> 32 = 1
const USHR_N1_1   = -1 >>> 1
check_eq(USHR_BIG_31, 2, "const = 4294967296 >>> 31")
check_eq(USHR_BIG_32, 1, "const = 4294967296 >>> 32")
check_eq(USHR_N1_1, INT_MAX, "const = -1 >>> 1")

println("\n[const >>>] boundary 33/63/64/65")
const USHR_BIG_33  = 4294967296 >>> 33   // 2^32 >>> 33 = 0
const USHR_BIG2_31 = 8589934592 >>> 31   // 2^33 >>> 31 = 4
const USHR_BIG2_32 = 8589934592 >>> 32   // 2^33 >>> 32 = 2
const USHR_BIG2_33 = 8589934592 >>> 33   // 2^33 >>> 33 = 1
check_eq(USHR_BIG_33,   0, "const = 4294967296 >>> 33")
check_eq(USHR_BIG2_31,  4, "const = 8589934592 >>> 31")
check_eq(USHR_BIG2_32,  2, "const = 8589934592 >>> 32")
check_eq(USHR_BIG2_33,  1, "const = 8589934592 >>> 33")

const USHR_N1_31   = -1 >>> 31
const USHR_N1_32   = -1 >>> 32
const USHR_N1_33   = -1 >>> 33
const USHR_N1_63   = -1 >>> 63
const USHR_N1_64   = -1 >>> 64
const USHR_N1_65   = -1 >>> 65
check_eq(USHR_N1_31,  8589934591, "const = -1 >>> 31")
check_eq(USHR_N1_32,  4294967295, "const = -1 >>> 32")
check_eq(USHR_N1_33,  2147483647, "const = -1 >>> 33")
check_eq(USHR_N1_63,           1, "const = -1 >>> 63")
check_eq(USHR_N1_64,           0, "const = -1 >>> 64")
check_eq(USHR_N1_65,           0, "const = -1 >>> 65")

const USHR_1_N31   = 1 >>> -31
const USHR_1_N33   = 1 >>> -33
const USHR_1_N63   = 1 >>> -63
const USHR_1_N64   = 1 >>> -64
const USHR_1_N65   = 1 >>> -65
check_eq(USHR_1_N31,  2147483648, "const = 1 >>> -31")
check_eq(USHR_1_N33,  8589934592, "const = 1 >>> -33")
check_eq(USHR_1_N63,     INT_MIN, "const = 1 >>> -63")
check_eq(USHR_1_N64,           0, "const = 1 >>> -64")
check_eq(USHR_1_N65,           0, "const = 1 >>> -65")

// --- const cross ---

println("\n[const cross] basic identities")
const CROSS_INV_A = (1 << 10) >> 10
const CROSS_INV_B = (42 << 5) >> 5
check_eq(CROSS_INV_A,  1, "const = (1 << 10) >> 10")
check_eq(CROSS_INV_B, 42, "const = (42 << 5) >> 5")


// ================================================================
//  Summary
// ================================================================

println(format("\n=== Results: %d/%d passed", passed, total))
if (failed > 0)
  print(format(", %d FAILED", failed))
println(" ===")

if (failed > 0)
  throw format("%d test(s) failed", failed)
