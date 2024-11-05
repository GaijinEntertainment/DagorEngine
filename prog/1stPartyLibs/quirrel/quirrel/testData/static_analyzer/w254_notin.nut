if (__name__ == "__analysis__")
  return

//expect:w254

if (!"weapModSlotName" not in ::item)
  return null

//-file:undefined-global