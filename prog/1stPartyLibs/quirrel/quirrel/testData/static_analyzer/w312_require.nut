if (__name__ == "__analysis__")
  return

let baz = require("testData/static_analyzer/module_foo.nut").baz
return baz(1)
