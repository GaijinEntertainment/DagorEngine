//expect:w264

local a = "{0}".subst(null)
if (a != "")
  return null

return a + 3
