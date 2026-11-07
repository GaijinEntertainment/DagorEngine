#forbid-implicit-type-methods

let t = {
    rawget = function (k) { return $"HAHA -- {k}" },
    a = "10",
    b = "Foo Foo"
}

println("Forbid implicit type methods")

try {
    let rg1 = t.rawget
    println("OK: 'rawget' is field in table")
} catch (e) {
    println($"FAIL: 'rawget' is field in table. Exception: \"{e}\"")
}

try {
    let ks = t.keys
    println("FAIL: 'keys' is built-in method of table which are forbiden.")
} catch (e) {
    println($"OK: 'keys' is built-in method of table. Exception: \"{e}\"")
}

try {
    let rg2 = t.$rawget
    println("OK: 'rawget' is built-in method of table accessed via '$'")
} catch (e) {
    println($"FAIL: 'rawget' is built-in method of table accessed via '$'. Exception: \"{e}\"")
}

try {
    let nks = t?.keys
    let nks2 = t?.$keys
    println($"OK: Safe access of built-in method 'keys' in both '$' and regular cases succeseded")
} catch (e) {
    println($"FAIL: Safe access of built-in method 'keys' failed. Exception: \"{e}\"")
}
