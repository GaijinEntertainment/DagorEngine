if (__name__ == "__analysis__")
  return

//-file:declared-never-used


function fn(x) {
    return ::y.cc ?? x ?? null
}

local s = null
local x = ::y ?? s
return x
