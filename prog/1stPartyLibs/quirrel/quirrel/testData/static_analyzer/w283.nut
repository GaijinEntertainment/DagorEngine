//-file:declared-never-used


let function fn(x) {
    return ::y.cc ?? x ?? null
}

local s = null
local x = ::y ?? s
return x
