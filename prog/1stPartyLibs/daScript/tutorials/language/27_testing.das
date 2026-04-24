// Tutorial 27: Testing with dastest
//
// This tutorial covers:
//   - The [test] annotation for declaring tests
//   - Assertion functions: equal, strictEqual, success, failure
//   - Sub-tests with run
//   - Running tests with the dastest runner
//
// This file IS a test — run it with:
//   daslang.exe dastest/dastest.das -- --test tutorials/language/27_testing.das

options gen2

require dastest/testing_boost public
require strings

// === Basic test functions ===
// A test function is annotated with [test] and takes a `t : T?` argument.
// T is the test context — use it for assertions and sub-tests.

[test]
def test_arithmetic(t : T?) {
    // equal(t, expected, actual) — non-fatal check
    // If it fails, the test continues running.
    t |> equal(2 + 2, 4)
    t |> equal(3 * 3, 9, "multiplication")
    t |> equal(10 - 3, 7, "subtraction")
}

// === Assertion functions ===
//
// equal(t, a, b, msg="")        — check a == b; non-fatal on failure
// strictEqual(t, a, b, msg="")  — check a == b; FATAL on failure (stops test)
// success(t, value, msg="")     — check value is truthy; non-fatal
// failure(t, msg="")            — always fails; non-fatal
// accept(t, value)              — suppress unused-variable warnings

[test]
def test_assertions(t : T?) {
    // equal — continues on failure
    t |> equal("hello", "hello", "string equality")
    t |> equal(3.14, 3.14, "float equality")

    // strictEqual — would stop the test on failure (fatal)
    t |> strictEqual(42, 42, "strict int check")

    // success — check a condition is truthy
    t |> success(10 > 5, "10 is greater than 5")
    t |> success(length("abc") == 3, "string length")

    // accept — just consumes a value (no assertion, suppresses warnings)
    let x = 123
    t |> accept(x)
}

// === Sub-tests with run ===
// Group related checks into named sub-tests.
// Each sub-test gets its own T? context.
// Use @@(t : T?) { ... } for the callback (no-capture lambda).

[test]
def test_string_operations(t : T?) {
    t |> run("concatenation") @@(t : T?) {
        t |> equal("ab" + "cd", "abcd")
    }
    t |> run("length") @@(t : T?) {
        t |> equal(length("hello"), 5)
        t |> equal(length(""), 0)
    }
    t |> run("slice") @@(t : T?) {
        t |> equal(slice("hello", 1, 3), "el")
    }
}

// === Testing data structures ===

[test]
def test_array_operations(t : T?) {
    var arr : array<int>
    arr |> push(10)
    arr |> push(20)
    arr |> push(30)

    t |> equal(length(arr), 3, "array length")
    t |> equal(arr[0], 10, "first element")
    t |> equal(arr[2], 30, "last element")

    arr |> erase(0)
    t |> equal(length(arr), 2, "after erase")
    t |> equal(arr[0], 20, "new first element")
}

// === Testing with helper functions ===

def factorial(n : int) : int {
    if (n <= 1) {
        return 1
    }
    return n * factorial(n - 1)
}

def is_palindrome(s : string) : bool {
    var result = true
    peek_data(s) $(data) {
        let n = length(data)
        for (i in range(n / 2)) {
            if (data[i] != data[n - 1 - i]) {
                result = false
                return
            }
        }
    }
    return result
}

[test]
def test_factorial(t : T?) {
    t |> run("base cases") @@(t : T?) {
        t |> equal(factorial(0), 1)
        t |> equal(factorial(1), 1)
    }
    t |> run("typical values") @@(t : T?) {
        t |> equal(factorial(5), 120)
        t |> equal(factorial(10), 3628800)
    }
}

[test]
def test_palindrome(t : T?) {
    t |> success(is_palindrome("racecar"), "racecar")
    t |> success(is_palindrome("madam"), "madam")
    t |> success(!is_palindrome("hello"), "hello is not a palindrome")
    t |> success(is_palindrome(""), "empty string")
}

// === Summary ===
//
// Import:
//   require dastest/testing_boost public
//
// Declare a test:
//   [test]
//   def test_name(t : T?) { ... }
//
// Assertions:
//   t |> equal(a, b)         — non-fatal, continues on fail
//   t |> strictEqual(a, b)   — fatal, stops test on fail
//   t |> success(cond)       — check truthy
//   t |> failure("reason")   — always fails
//   t |> accept(value)       — suppress unused warnings
//
// Sub-tests:
//   t |> run("name") @@(t : T?) { ... }
//
// Running:
//   daslang.exe dastest/dastest.das -- --test path/to/test.das
//   daslang.exe dastest/dastest.das -- --test path/to/tests/  (directory)
