// EXPECTED: no error - class literal assigned to class variable
local x: class = class {
    a = 1
    b = 2
}
local y: class|null = null
return [x, y]
