// Reassigning a local that aliased an external module must clear externalValue.
// The empty diag file is the success signal: m.nonexistent must NOT be flagged
// after m = arg, because m no longer points at the require()'d module.
let mod = require("testData/static_analyzer/module_foo.nut")

return function(arg) {
  local m = mod
  m = arg
  return m.nonexistent
}
