let { on_module_unload } = require("modules")

on_module_unload(function(is_closing) {
  assert(is_closing)

  for (local i = 0; i < 256; i++)
    on_module_unload(function(_) {})

  println("first")
})

on_module_unload(function(is_closing) {
  assert(is_closing)
  println("second")
})

println("done")
