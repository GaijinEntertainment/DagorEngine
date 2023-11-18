//expect:w205

let function y() { //-declared-never-used
  return
  let t = { //-declared-never-used
    a = 1
  }
}
