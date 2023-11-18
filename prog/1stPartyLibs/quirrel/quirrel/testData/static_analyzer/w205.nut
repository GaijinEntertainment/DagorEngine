//expect:w205

let function x(callback) { //-declared-never-used
  return
    callback()
}
