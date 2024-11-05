if (__name__ == "__analysis__")
  return

//expect:w249

local a = ::x?.b
return a.b[6]

//-file:undefined-global