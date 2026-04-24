//-file:potentially-nullable-index
//-file:undefined-global
//-file:declared-never-used

local x, y
function b(...) {}

// `let` binding with external function call
// Should warn - let bindings are immutable and preserved
function test_let(a) {
    let index = x < y
    print(a[index])
}

// Direct use without any function call (baseline)
// Doesn't work now!!!
// Should warn - no function calls to potentially kill the value
function test_local(a) {
    local index = x < y
    print(a[index])
}

// Local variable, local function call, then external function call
// Doesn't currently warn - the implementation is too complex.
// Should warn - local function b() doesn't modify index
function test_local_with_local_and_external_call(a) {
    local index = x < y
    b()
    print(a[index])
}

// Local variable in nested block scope
// Doesn't currently warn - the implementation is too complex
// Should warn - nested scope doesn't affect the fix
function test_nested_block_scope(a) {
    local index = x < y
    {
        print(a[index])
    }
}

// Multiple external function calls
// Doesn't currently warn - the implementation is too complex
// Should warn - external calls don't affect function-local variables
function test_multiple_external_calls(a) {
    local index = x < y
    print("before")
    print("middle")
    print(a[index])
}

// Local variable used inside loop scope
// Doesn't currently warn - the implementation is too complex
// Should warn - loop scope is still within the function
function test_loop_scope(a, arr) {
    local index = x < y
    foreach (_k, v in arr) { //-potentially-nullable
        if (v)
            print(a[index])
    }
}

// Local function that doesn't modify the variable
// Doesn't currently warn - the implementation is too complex
// Should warn - inner() has empty body, doesn't modify index
function test_local_function_no_modification(a) {
    local index = x < y
    function inner() {} //-declared-never-used
    inner()
    print(a[index])
}


// Variable reassigned - should NOT warn
function test_reassigned_variable(a) {
    local index = x < y
    index = 42
    print(a[index])
}

// Conditional assignment - should NOT warn (index could be non-boolean)
function test_conditional_assignment(a, cond) {
    local index = x < y
    if (cond)
        index = 42
    print(a[index])
}

// Local variable, local function call capturing `index` as a free variable
// Should NOT warn - the function can modify the variable
function test_local_with_local_call_with_outer(a) {
    local index = x < y
    function localFunc() {
      index = 42
    }
    localFunc()
    print(a[index])
}

// A variation of above - should NOT warn
function test_local_with_local_call_with_outer_2(a) {
    local index = x < y
    b(function() {
      index = 42
    })
    print(a[index])
}
