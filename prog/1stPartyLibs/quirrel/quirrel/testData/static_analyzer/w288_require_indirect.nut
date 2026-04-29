let mod = require("testData/static_analyzer/module_foo.nut")
let foo = mod.foo
return { bar = @(x) foo(x) }
