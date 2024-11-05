if (__name__ == "__analysis__")
  return

//expect:w248

local a = ::x?.b
local b = a
if (b) {
    b()
} else {
    a()
}

return a()

//-file:undefined-global