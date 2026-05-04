// Test: closure captures from multiple depths
// The lambda captures both 'rootVar' (depth 0) and 'paramVar' (depth 1)
// It should hoist to depth 1 (outer's body), NOT to depth 0 (root)

let rootVar = 1

function outer(paramVar) {
    function inner() {
        // This lambda captures from BOTH depth 0 (rootVar) and depth 1 (paramVar)
        // Must hoist to depth 1, not depth 0
        return @() rootVar + paramVar
    }
    return inner()
}
