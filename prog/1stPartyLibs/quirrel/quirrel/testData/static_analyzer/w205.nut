//expect:w205

function x(callback) { //-declared-never-used
  return
    callback()
}
