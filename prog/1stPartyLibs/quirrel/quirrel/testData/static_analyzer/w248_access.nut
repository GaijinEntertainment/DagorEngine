if (__name__ == "__analysis__")
  return

//-file:undefined-global

local a = ::x?.b
return a.b[6]
