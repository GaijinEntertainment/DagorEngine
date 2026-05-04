// Tutorial 10: Tables (Dictionaries)
//
// This tutorial covers:
//   - Declaring and initializing tables
//   - Inserting, erasing, and looking up values
//   - Safe access with ?[] and key_exists
//   - Block-based lookup with get()
//   - Iterating over keys and values
//   - Table comprehensions
//   - Key-only tables (sets)
//
// Note: Tables are hash-based — iteration order is NOT deterministic.
//
// Run: daslang.exe tutorials/language/10_tables.das

options gen2

[export]
def main {

    // === Declaring a table ===
    // table<KeyType; ValueType> — note the semicolon between key and value types.
    var ages : table<string; int>

    // === Inserting values ===
    // Use insert() with pipe syntax
    ages |> insert("Alice", 30)
    ages |> insert("Bob", 25)
    ages |> insert("Carol", 35)
    print("table size: {length(ages)}\n")

    // === Safe lookup with ?[] ===
    // ?[] returns the value if the key exists, otherwise uses the fallback after ??
    // This does NOT insert missing keys.
    let aliceAge = ages?["Alice"] ?? -1
    let unknownAge = ages?["Dave"] ?? -1
    print("Alice's age: {aliceAge}\n")
    print("Dave's age (not found): {unknownAge}\n")

    // === Checking if a key exists ===
    print("has Alice: {key_exists(ages, "Alice")}\n")
    print("has Dave: {key_exists(ages, "Dave")}\n")

    // === Block-based lookup with get() ===
    // get() runs a block only if the key is found.
    get(ages, "Bob") $(val) {
        print("Bob is {val} years old\n")
    }

    // === Erasing keys ===
    ages |> erase("Bob")
    print("after erase, has Bob: {key_exists(ages, "Bob")}\n")
    print("size after erase: {length(ages)}\n")

    // === Inline construction ===
    // Build a table directly with { key => value } syntax and commas.
    var scores <- { "math" => 95, "english" => 87, "science" => 92 }

    // Look up individual values (order-independent)
    print("math: {scores?["math"] ?? 0}\n")
    print("english: {scores?["english"] ?? 0}\n")
    print("science: {scores?["science"] ?? 0}\n")

    // === Iterating over a table ===
    // Use keys() and values() iterators with parallel for
    // NOTE: iteration order is not guaranteed for tables.
    var total = 0
    for (subject, score in keys(scores), values(scores)) {
        total += score
    }
    print("total of all scores: {total}\n")

    // === Modifying values ===
    // Use get() with a mutable block to modify in-place
    get(scores, "math") $(var val) {
        val = 100     // update math score
    }
    print("updated math: {scores?["math"] ?? 0}\n")

    // === Table comprehension ===
    // { for (var in source); key => expr } builds a table
    var squareMap <- { for (x in 1..6); x => x * x }
    print("square of 3: {squareMap?[3] ?? -1}\n")
    print("square of 5: {squareMap?[5] ?? -1}\n")

    // === Key-only tables (sets) ===
    // A table with just keys and no values acts like a set.
    var seen : table<string>
    seen |> insert("apple")
    seen |> insert("banana")
    seen |> insert("apple")     // duplicate — no effect
    print("seen has apple: {key_exists(seen, "apple")}\n")
    print("seen has cherry: {key_exists(seen, "cherry")}\n")
    print("set size: {length(seen)}\n")

    // === Important safety notes ===
    //
    // 1. tab[key] with the [] operator INSERTS a default value if the key
    //    is missing. This requires 'unsafe'. Prefer ?[] or get() instead.
    //
    // 2. Never do tab[k1] = tab[k2] — the first [] may resize the table,
    //    invalidating the reference from the second [].
    //
    // 3. Tables cannot be copied with = — use <- (move) or := (clone).
}

// output:
// table size: 3
// Alice's age: 30
// Dave's age (not found): -1
// has Alice: true
// has Dave: false
// Bob is 25 years old
// after erase, has Bob: false
// size after erase: 2
// math: 95
// english: 87
// science: 92
// total of all scores: 274
// updated math: 100
// square of 3: 9
// square of 5: 25
// seen has apple: true
// seen has cherry: false
// set size: 2
