if (__name__ == "__analysis__")
  return

let [ foo bar ] = require("testData/static_analyzer/foo.nut")

return { foo, bar }
