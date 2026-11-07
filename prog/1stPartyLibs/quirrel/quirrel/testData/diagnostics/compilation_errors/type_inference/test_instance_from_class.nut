// EXPECTED: compile-time error - class() produces instance, assigned to string
let MyClass = class {
    x = 0
    constructor() {}
}
local obj: string = MyClass()
return obj
