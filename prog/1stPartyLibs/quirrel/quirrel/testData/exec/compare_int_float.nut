{ // >
  let a = -1 > -1.0
  let b = 5 > -1.0
  let c = -1 > 5.0
  assert(a == false)
  assert(b == true)
  assert(c == false)

  assert(-1 > -1.0 == false)
  assert(5 > -1.0 == true)
  assert(-1 > 5.0 == false)

  assert((-1 > -1.0 ? 55 : 66) == 66)
  assert((5 > -1.0 ? 55 : 66) == 55)
  assert((-1 > 5.0 ? 55 : 66) == 66)

  if (-1 > -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (5 > -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (-1 > 5.0) {
    assert(false)
  } else {
    assert(true)
  }

  let x = -1
  let y = 5.0

  let a1 = x > x
  let b1 = y > x
  let c1 = x > y
  assert(a1 == false)
  assert(b1 == true)
  assert(c1 == false)

  if (x > x) {
    assert(false)
  } else {
    assert(true)
  }

  if (y > x) {
    assert(true)
  } else {
    assert(false)
  }

  if (x > y) {
    assert(false)
  } else {
    assert(true)
  }
}

{ // <
  let a = -1 < -1.0
  let b = 5 < -1.0
  let c = -1 < 5.0
  assert(a == false)
  assert(b == false)
  assert(c == true)

  assert(-1 < -1.0 == false)
  assert(5 < -1.0 == false)
  assert(-1 < 5.0 == true)

  assert((-1 < -1.0 ? 55 : 66) == 66)
  assert((5 < -1.0 ? 55 : 66) == 66)
  assert((-1 < 5.0 ? 55 : 66) == 55)

  if (-1 < -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (5 < -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (-1 < 5.0) {
    assert(true)
  } else {
    assert(false)
  }

  let x = -1
  let y = 5.0

  let a1 = x < x
  let b1 = y < x
  let c1 = x < y
  assert(a1 == false)
  assert(b1 == false)
  assert(c1 == true)

  if (x < x) {
    assert(false)
  } else {
    assert(true)
  }

  if (y < x) {
    assert(false)
  } else {
    assert(true)
  }

  if (x < y) {
    assert(true)
  } else {
    assert(false)
  }
}

{ // >=
  let a = -1 >= -1.0
  let b = 5 >= -1.0
  let c = -1 >= 5.0
  assert(a == true)
  assert(b == true)
  assert(c == false)

  assert(-1 >= -1.0 == true)
  assert(5 >= -1.0 == true)
  assert(-1 >= 5.0 == false)

  assert((-1 >= -1.0 ? 55 : 66) == 55)
  assert((5 >= -1.0 ? 55 : 66) == 55)
  assert((-1 >= 5.0 ? 55 : 66) == 66)

  if (-1 >= -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (5 >= -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (-1 >= 5.0) {
    assert(false)
  } else {
    assert(true)
  }

  let x = -1
  let y = 5.0

  let a1 = x >= x
  let b1 = y >= x
  let c1 = x >= y
  assert(a1 == true)
  assert(b1 == true)
  assert(c1 == false)

  if (x >= x) {
    assert(true)
  } else {
    assert(false)
  }

  if (y >= x) {
    assert(true)
  } else {
    assert(false)
  }

  if (x >= y) {
    assert(false)
  } else {
    assert(true)
  }
}

{ // <=
  let a = -1 <= -1.0
  let b = 5 <= -1.0
  let c = -1 <= 5.0
  assert(a == true)
  assert(b == false)
  assert(c == true)

  assert(-1 <= -1.0 == true)
  assert(5 <= -1.0 == false)
  assert(-1 <= 5.0 == true)

  assert((-1 <= -1.0 ? 55 : 66) == 55)
  assert((5 <= -1.0 ? 55 : 66) == 66)
  assert((-1 <= 5.0 ? 55 : 66) == 55)

  if (-1 <= -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (5 <= -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (-1 <= 5.0) {
    assert(true)
  } else {
    assert(false)
  }

  let x = -1
  let y = 5.0

  let a1 = x <= x
  let b1 = y <= x
  let c1 = x <= y
  assert(a1 == true)
  assert(b1 == false)
  assert(c1 == true)

  if (x <= x) {
    assert(true)
  } else {
    assert(false)
  }

  if (y <= x) {
    assert(false)
  } else {
    assert(true)
  }

  if (x <= y) {
    assert(true)
  } else {
    assert(false)
  }
}

{ // ==
  let a = -1 == -1.0
  let b = 5 == -1.0
  let c = -1 == 5.0
  assert(a == true)
  assert(b == false)
  assert(c == false)

  assert(-1 == -1.0 == true)
  assert(5 == -1.0 == false)
  assert(-1 == 5.0 == false)

  assert((-1 == -1.0 ? 55 : 66) == 55)
  assert((5 == -1.0 ? 55 : 66) == 66)
  assert((-1 == 5.0 ? 55 : 66) == 66)

  if (-1 == -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (5 == -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (-1 == 5.0) {
    assert(false)
  } else {
    assert(true)
  }

  let x = -1
  let y = 5.0

  let a1 = x == x
  let b1 = y == x
  let c1 = x == y
  assert(a1 == true)
  assert(b1 == false)
  assert(c1 == false)

  if (x == x) {
    assert(true)
  } else {
    assert(false)
  }

  if (y == x) {
    assert(false)
  } else {
    assert(true)
  }

  if (x == y) {
    assert(false)
  } else {
    assert(true)
  }
}

{ // !=
  let a = -1 != -1.0
  let b = 5 != -1.0
  let c = -1 != 5.0
  assert(a == false)
  assert(b == true)
  assert(c == true)

  assert(-1 != -1.0 == false)
  assert(5 != -1.0 == true)
  assert(-1 != 5.0 == true)

  assert((-1 != -1.0 ? 55 : 66) == 66)
  assert((5 != -1.0 ? 55 : 66) == 55)
  assert((-1 != 5.0 ? 55 : 66) == 55)

  if (-1 != -1.0) {
    assert(false)
  } else {
    assert(true)
  }

  if (5 != -1.0) {
    assert(true)
  } else {
    assert(false)
  }

  if (-1 != 5.0) {
    assert(true)
  } else {
    assert(false)
  }

  let x = -1
  let y = 5.0

  let a1 = x != x
  let b1 = y != x
  let c1 = x != y
  assert(a1 == false)
  assert(b1 == true)
  assert(c1 == true)

  if (x != x) {
    assert(false)
  } else {
    assert(true)
  }

  if (y != x) {
    assert(true)
  } else {
    assert(false)
  }

  if (x != y) {
    assert(true)
  } else {
    assert(false)
  }
}

print("ok")
