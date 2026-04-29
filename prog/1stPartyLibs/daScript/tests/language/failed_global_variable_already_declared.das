// duplicate global variable declaration
// verifies compiler rejects declaring the same global variable twice
options gen2
expect 30204    // global_variable_already_declared

let {
    a : int
}

let {
    a : int

}
