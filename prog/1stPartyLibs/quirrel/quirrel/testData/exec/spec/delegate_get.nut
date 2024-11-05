
let t = {
    rawget = function (k) { return $"HAHA -- {k}" },
    a = "10",
    b = "Foo Foo"
}

let x = t.$rawget("b")
let y = t.rawget("b")

println($"rawget(\"b\") = {y}, $rawget(\"b\") = {x}")

let z = t?.$foo
let nz = t?.foo

println($"t?.$foo = {z}, t?.foo = {nz}")

try {
    let vf = t.$bar
    println("FAIL: '$bar' is not a built-in member of table but successfully accessed")
} catch (e) {
    println($"OK: '$bar' is not a built-in member of table and access raised exectpion: \"{e}\"")
}

#forbid-implicit-default-delegates

println("Forbid implicit default delegates")

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

#allow-implicit-default-delegates

println("Allow implicit default delegates")

try {
    let rg1 = t.rawget
    println("OK: 'rawget' is field in table")
} catch (e) {
    println($"FAIL: 'rawget' is field in table. Exception: \"{e}\"")
}

try {
    let ks = t.keys
    println($"OK: 'keys' is built-in method of table.")
} catch (e) {
    println("FAIL: 'keys' is built-in method of table which are forbiden.")
}

try {
    let rg2 = t.$rawget
    println("OK: 'rawget' is built-in method of table accessed via '$'")
} catch (e) {
    println($"FAIL: 'rawget' is built-in method of table accessed via '$'. Exception: \"{e}\"")
}
