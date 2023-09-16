//expect:w205
::a <- 5

let function y() { //-declared-never-used
  return
  {
    a = 1
  }
}
