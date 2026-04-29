options gen2

// Companion script for the "Serialization" tutorial.
// This script is compiled, serialized to a binary blob, deserialized,
// and then simulated — demonstrating that compilation can be skipped
// on subsequent runs.

[export]
def greet(name : string) : string {
    return "Hello, {name}!"
}

[export]
def sum_to(n : int) : int {
    var total = 0
    for (i in range(1, n + 1)) {
        total += i
    }
    return total
}

[export]
def test() {
    print("=== Serialization Tutorial ===\n")
    print("{greet("World")}\n")
    print("sum_to(10)  = {sum_to(10)}\n")
    print("sum_to(100) = {sum_to(100)}\n")
}
