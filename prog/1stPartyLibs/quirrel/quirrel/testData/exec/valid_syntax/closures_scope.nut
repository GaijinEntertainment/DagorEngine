// Valid syntax: Closures and scoping

// Basic closure
local outer_val = 10
let closure = function() {
    return outer_val
}
assert(closure() == 10)

// Closure captures by reference
outer_val = 20
assert(closure() == 20)

// Nested closures
function make_multiplier(factor) {
    return function(x) {
        return x * factor
    }
}
let double_fn = make_multiplier(2)
let triple_fn = make_multiplier(3)
assert(double_fn(5) == 10)
assert(triple_fn(5) == 15)

// Closure over loop variable
local fns = []
for (local i = 0; i < 5; i++) {
    let captured_i = i // capture by value
    fns.append(@() captured_i)
}
assert(fns[0]() == 0)
assert(fns[4]() == 4)

// Nested function accessing outer scope
function outer() {
    local x = 100
    function inner() {
        return x + 1
    }
    return inner()
}
assert(outer() == 101)

// Deep nesting
function level1() {
    local a_val = 1
    function level2() {
        local b_val = 2
        function level3() {
            return a_val + b_val
        }
        return level3()
    }
    return level2()
}
assert(level1() == 3)

// Function returning table of closures
function make_stack() {
    local data = []
    return {
        function push(v) { data.append(v) }
        function pop() { return data.pop() }
        function size() { return data.len() }
        function peek() { return data.top() }
    }
}

let stack = make_stack()
stack.push(1)
stack.push(2)
stack.push(3)
assert(stack.size() == 3)
assert(stack.peek() == 3)
assert(stack.pop() == 3)
assert(stack.size() == 2)

// IIFE (Immediately Invoked Function Expression)
let iife_result = (function() {
    return 42
})()
assert(iife_result == 42)

// Lambda IIFE
let lambda_iife = (@() 99)()
assert(lambda_iife == 99)

// Block scope isolation
{
    local scoped = 42
    assert(scoped == 42)
}

// Multiple blocks
{
    local a_block = 1
    {
        local b_block = 2
        assert(a_block == 1)
        assert(b_block == 2)
    }
}

print("closures_scope: OK\n")
