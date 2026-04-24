// Test: calling a generator method via .call() from a constructor.
//
// Before fix: this crashed with access violation (or assertion
// "SQObjectPtr::SQObjectPtr(SQGenerator*): Assertion '_unVal.pTable' failed")
// because GrowCallStack() invalidated the prevci pointer in Execute(),
// causing it to enter the main instruction loop instead of returning the
// generator object. The VM then hit _OP_YIELD with ci->_generator==NULL,
// reaching the "only generator can be yielded" error path which itself
// crashed constructing SQObjectPtr from a NULL generator pointer.
//
// After fix: the generator is created and returned correctly.

//--- 1. Direct .call() on yielding method from constructor ---
class DirectCall {
    constructor() {
        this.do_yield.call(0)
    }
    function do_yield() { yield }
}
DirectCall()
print("1. Direct .call() from ctor: OK\n")


//--- 2. Indirect .call() via helper method (original class_yield.nut) ---
class IndirectCall {
    constructor() {
        this.call_yield()
    }
    function call_yield() {
        this.do_yield.call(0)
    }
    function do_yield() { yield }
}
IndirectCall()
print("2. Indirect .call() from ctor: OK\n")


//--- 3. .call() on yielding method with value passing ---
class YieldWithValue {
    _result = null
    constructor(v) {
        this._result = v
        let g = this.gen.call(this)
        this._result = resume g
    }
    function gen() { yield this._result + 100 }
}
let obj3 = YieldWithValue(5)
assert(obj3._result == 105)
print("3. .call() with value passing: OK\n")


//--- 4. Inherited yielding method called via .call() from derived ctor ---
class Base4 {
    function do_yield() { yield 42 }
}
class Derived4(Base4) {
    constructor() {
        this.do_yield.call(0)
    }
}
Derived4()
print("4. Inherited yield via .call(): OK\n")


//--- 5. Deep nesting that triggers multiple callstack reallocations ---
// Initial callstack size is 4; each recursion adds a frame.
// The generator .call() from the constructor must survive GrowCallStack().
function deep(n) {
    if (n <= 0) {
        class Inner {
            constructor() { this.gen.call(0) }
            function gen() { yield }
        }
        Inner()
        return 1
    }
    return deep(n - 1) + 1
}
assert(deep(30) == 31)
print("5. Deep nesting + yield .call(): OK\n")


//--- 6. Generator method .call() outside constructor (control case) ---
class Normal6 {
    _v = 0
    constructor(v) { this._v = v }
    function gen() { yield this._v; yield this._v + 1 }
}
let obj6 = Normal6(10)
let g6 = obj6.gen.call(obj6)
assert(resume g6 == 10)
assert(resume g6 == 11)
print("6. Generator .call() outside ctor: OK\n")


//--- 7. Multiple generators created via .call() in constructor ---
class Multi7 {
    _g1 = null
    _g2 = null
    constructor() {
        this._g1 = this.gen1.call(this)
        this._g2 = this.gen2.call(this)
    }
    function gen1() { yield 10; yield 20 }
    function gen2() { yield 30; yield 40 }
}
let obj7 = Multi7()
assert(resume obj7._g1 == 10)
assert(resume obj7._g2 == 30)
assert(resume obj7._g1 == 20)
assert(resume obj7._g2 == 40)
print("7. Multiple generators from ctor: OK\n")


//--- 8. newthread + generator function ---
let gen8 = function() { yield 21 }
let th8 = newthread(gen8)
let r8 = th8.call()
assert(type(r8) == "generator")
assert(resume r8 == 21)
print("8. newthread + generator: OK\n")


print("\nAll tests passed.\n")
