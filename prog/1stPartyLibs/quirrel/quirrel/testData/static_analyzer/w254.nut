if (__name__ == "__analysis__")
  return

//expect:w254

if (!"weapModSlotName" in ::item)
  return null

//-file:undefined-global
