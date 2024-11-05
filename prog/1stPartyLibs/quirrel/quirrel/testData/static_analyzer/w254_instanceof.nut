if (__name__ == "__analysis__")
  return

//expect:w254
local x = 10
if (x instanceof !"weapModSlotName")
  return null
