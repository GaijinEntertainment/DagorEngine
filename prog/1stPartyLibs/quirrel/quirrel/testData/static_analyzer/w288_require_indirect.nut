let mod = require("testData/static_analyzer/foo.nut")
let foo = mod.foo
return { bar = @(x) foo(x) }
