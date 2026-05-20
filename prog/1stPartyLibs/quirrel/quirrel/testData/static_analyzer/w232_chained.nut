function fn(x, y) {
  return (x == 1) && (y == 2) && ((x == 2))
}

function grouped_chains(x, y, z) {
  return ((x == 1) && (y == 2)) && ((x == 2) && (z == 3))
}

return [fn, grouped_chains]
