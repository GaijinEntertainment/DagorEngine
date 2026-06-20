options gen2

// Fixture for the lint SKIP path: intentional compile errors guarded by an `expect` directive
// (a `failed_`-style test). `lint` must SKIP it (not compile) and return a VALID make_tool_result
// envelope — a raw "SKIP ..." string would be invalid JSON-RPC and hang the client.
expect 30343

def broken() : int {
    return "not an int"
}
