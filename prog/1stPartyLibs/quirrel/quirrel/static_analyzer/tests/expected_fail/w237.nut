//expect:w237

let function fn(x) { //-declared-never-used
  if (!x)
    return
  x += 1
}
