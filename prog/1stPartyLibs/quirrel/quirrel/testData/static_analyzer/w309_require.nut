if (__name__ == "__analysis__")
  return

let { foo bar } = require("testData/static_analyzer/module_foo.nut")

return { foo bar }
