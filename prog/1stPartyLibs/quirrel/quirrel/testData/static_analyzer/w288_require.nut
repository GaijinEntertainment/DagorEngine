let { foo } = require("testData/static_analyzer/foo.nut")
return { bar = @(x) foo(x) }
