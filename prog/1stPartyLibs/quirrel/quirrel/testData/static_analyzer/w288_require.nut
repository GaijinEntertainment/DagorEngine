let { foo } = require("testData/static_analyzer/module_foo.nut")
return { bar = @(x) foo(x) }
