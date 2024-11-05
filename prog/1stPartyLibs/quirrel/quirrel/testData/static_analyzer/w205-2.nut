//expect:w205

function y() { //-declared-never-used
  return
  let t = { //-declared-never-used
    a = 1
  }
}
