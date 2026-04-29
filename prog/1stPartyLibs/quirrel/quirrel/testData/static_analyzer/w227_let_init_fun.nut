if (__name__ == "__analysis__")
  return

function foo(_p) {}
// This is allowed, no warning here
let setGroup = foo(function setGroup(_crew, _group, _onFinishCb) {
})

setGroup()
