// EXPECTED: no error - class() produces instance, assigned to instance
// Note: calling a class produces an instance, but inferring this
// requires knowing the callee is a class, which may be ~0u (unknown)
// in early implementation. This test documents the desired behavior.
local MyClass = class {
    x = 0
    constructor() {}
}
local obj: instance = MyClass()
local obj2: instance|null = null
return [obj, obj2]
