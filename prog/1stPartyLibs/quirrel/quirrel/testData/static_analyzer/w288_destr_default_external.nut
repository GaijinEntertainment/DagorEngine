let mod = require("testData/static_analyzer/module_foo.nut")
let { foo } = mod
let { missing = foo } = mod

if (__name__ == "__analysis__")
  return

let _y = missing(1)
