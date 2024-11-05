//expect:w225

function x(y) { //-declared-never-used
  if (y == 1)
    return "y == 1"
  else if (y == 2)
    return "y == 2"
}