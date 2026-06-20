options gen2
// STYLE011: variable declaration followed by immediate assignment
//
// Problem: Declaring a variable and immediately assigning it on the next line
// should be combined into a single declaration with initialization.
//
// Bad pattern:
//   var x : int
//   x = 5
//
// Good pattern:
//   var x = 5

require strings

// bad: var then copy (1 warning)
def bad_decl_then_copy() : int {
    var x : int
    x = 5
    return x
}

// bad: var then clone (1 warning)
def bad_decl_then_clone() : string {
    var s : string
    s := "hello"
    return s
}

// good: combined declaration
def good_combined() : int {
    var x = 5
    return x
}

// good: intervening statement
def good_intervening() : int {
    var x : int
    let y = 10
    x = y
    return x
}

// good: inscope variable
def good_inscope() {
    var inscope arr : array<int>
    arr |> push(1)
}
