from "string" import *

print("=== STRING METHODS TESTS ===\n")

let s = "Hello, World!"
println("len: " + s.len())  // 13
println("tostring: '" + s.tostring() + "'")
try println("tointeger (invalid): " + s.tointeger()) catch(e) println(e)
try println("tofloat (invalid): " + s.tofloat()) catch(e) println(e)
println("hash: " + s.hash())

println("tolower: '" + s.tolower() + "'")
println("toupper: '" + s.toupper() + "'")
println("tolower partial: '" + s.tolower(0, 5) + "'")
println("toupper partial: '" + s.toupper(7, 12) + "'")

println("indexof 'World': " + s.indexof("World"))
println("indexof 'o' from 5: " + s.indexof("o", 5))
println("contains 'World': " + s.contains("World"))
println("contains 'world' (case-sensitive): " + s.contains("world"))
println("hasindex 0: " + s.hasindex(0))
println("hasindex 100: " + s.hasindex(100))
println("startswith 'Hello': " + s.startswith("Hello"))
println("endswith 'World!': " + s.endswith("World!"))

println("slice(0,5): '" + s.slice(0, 5) + "'")
println("slice(7): '" + s.slice(7) + "'")
println("replace 'World' -> 'Squirrel': '" + s.replace("World", "Squirrel") + "'")

let parts = s.split(", ")
println("split: [" + "|".join(parts) + "]")
println("join: '" + " ".join(["Hello", "Squirrel", "World"]) + "'")
println("concat: '" + "".concat("a", "b", "c") + "'")

println("strip: '" + "  test  ".strip() + "'")
println("lstrip: '" + "  test  ".lstrip() + "'")
println("rstrip: '" + "  test  ".rstrip() + "'")
println("escape: '" + "\n\t\"\'\\".escape() + "'")

println("subst: '" + "Hello {0}, {1}!".subst("Squirrel", "World") + "'")
println("format: '" + format("Value: %d", 42) + "'")
printf("printf test: %s %d\n", "value", 100)

let chars = "a,b:c;d".split_by_chars(",;:", false)
println("split_by_chars: [" + "|".join(chars) + "]")

let s2 = clone s
println("clone equality: " + (s == s2))

print("\n=== NUMERIC METHODS TESTS ===\n")

let i = 42
println("integer.tostring: '" + i.tostring() + "'")
println("integer.tointeger: " + i.tointeger())
println("integer.tofloat: " + i.tofloat())
println("integer.tochar: '" + i.tochar() + "'")  // '*'
println("integer.clone: " + clone(i))

let f = 3.14
println("float.tostring: '" + f.tostring() + "'")
println("float.tointeger: " + f.tointeger())
println("float.tofloat: " + f.tofloat())
println("float.clone: " + clone(f))

{
let b = true
println("bool.tostring: '" + b.tostring() + "'")
println("bool.tointeger: " + b.tointeger())
println("bool.tofloat: " + b.tofloat())
println("bool.tochar: '" + b.tochar() + "'")  // '\x01'
println("bool.clone: " + clone(b))
}

print("\n=== CLOSURE METHODS TESTS ===\n")

local free_var = "captured"
local closure = @(x) x + " " + free_var

println("closure.call: '" + closure.call(this, "test") + "'")
println("closure.pcall: " + closure.pcall(this, "safe").tostring())

local err_closure = function err_closure() { throw "error" }
local res = "X"
try res = err_closure.pcall(this) catch (e) println(e)
println("closure.pcall error: " + (res.len() == 2 ? res[1] : "success"))

local finfo = closure.getfuncinfos()
println("closure.getfuncinfos keys: " + finfo.rawin("name") + ", " + finfo.rawin("nclosures"))

local fvinfo = closure.getfreevar(0)
println("closure.getfreevar name: '" + fvinfo.name + "', value: '" + fvinfo.value + "'")

local env = { prefix = "env:" }
local bound = closure.bindenv(env)

println("closure.clone type: " + typeof(clone(closure)))

print("\n=== CLASS & INSTANCE METHODS TESTS ===\n")

class TestClass {
    value = null
    constructor(x) {
        this.value = x
    }
    function method() {
        return this.value * 2
    }
}

println("class.rawget 'method' exists: " + TestClass.rawin("method"))
TestClass.rawset("dynamic", 100)
println("class.rawget dynamic: " + TestClass.rawget("dynamic"))
println("class.getbase: " + (TestClass.getbase() == null ? "null" : "not null"))
println("class.getfuncinfos: " + typeof(TestClass.getfuncinfos()))
println("class.hasindex 'value': " + TestClass.hasindex("value"))
println("class.is_frozen initially: " + TestClass.is_frozen())
TestClass.lock()
println("class.is_frozen after freeze: " + TestClass.is_frozen())

local inst = TestClass(21)

println("instance.rawget 'value': " + inst.rawget("value"))
inst.rawset("value", 42)
println("instance.rawget after set: " + inst.rawget("value"))
println("instance.rawin 'method': " + inst.rawin("method"))
println("instance.getfuncinfos type: " + typeof(inst.getfuncinfos()))
println("instance.hasindex 0: " + inst.hasindex(0))  // false для объекта
println("instance.is_frozen: " + inst.is_frozen())

local inst2 = clone(inst)
inst2.value = 100
println("clone isolation (original still 42): " + (inst.value == 42))

{
local t = {[1] = "123", [2] = "asd"}
t.swap(1, 2)
println("swap result: t[1]=" + t[1] + ", t[2]=" + t[2])
}

local meta = inst.getmetamethod("_add")
println("getmetamethod 'add': " + (meta == null ? "null" : "exists"))

print("\n=== ALL TESTS COMPLETED ===\n")
