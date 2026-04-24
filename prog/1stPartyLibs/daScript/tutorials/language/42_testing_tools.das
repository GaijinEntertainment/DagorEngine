// Tutorial 42: Testing Tools (faker + fuzzer)
//
// This tutorial covers:
//   - Faker: configurable random test-data generation
//   - Generating random primitives: int, float, bool, string
//   - Generating random vectors: int2, float3, etc.
//   - Generating random dates and file names
//   - Fuzzer: running a block thousands of times to find crashes
//   - fuzz vs fuzz_debug: silent vs verbose crash detection
//   - Property-based testing pattern with faker + fuzzer
//
// Prerequisites: Tutorial 27 (Testing), Tutorial 38 (Random)
//
// Run: daslang.exe tutorials/language/42_testing_tools.das

options gen2
options persistent_heap

require daslib/faker
require daslib/fuzzer
require strings

// ============================================================
// Section 1: Faker basics
// ============================================================
// A Faker struct produces random values for every built-in type.
// It wraps an infinite random uint iterator internally.

def demo_faker_basics() {
    print("=== faker basics ===\n")

    var fake <- Faker()

    // Random integers
    print("  random_int:    {fake |> random_int}\n")
    print("  random_uint:   {fake |> random_uint}\n")
    print("  random_int8:   {fake |> random_int8}\n")

    // Random floats (note: raw bits — can be NaN, Inf, etc.)
    print("  random_float:  {fake |> random_float}\n")
    print("  random_double: {fake |> random_double}\n")

    // Random bool (derived from random_uint)
    let b = (fake |> random_uint) % 2u == 0u
    print("  random_bool:   {b}\n")

    delete fake
}

// ============================================================
// Section 2: Random strings and file names
// ============================================================
// Faker can generate random strings matching regex-like patterns,
// long byte strings, and file-name-safe strings.

def demo_faker_strings() {
    print("\n=== faker strings ===\n")

    var fake <- Faker()

    // any_string: random characters up to re_gen limit
    let s = fake |> any_string()
    print("  any_string length: {length(s)}\n")

    // any_file_name: alphanumeric + underscore + dot
    print("  any_file_name: {fake |> any_file_name}\n")

    // long_string: up to fake.max_long_string bytes (default 4096)
    let ls = fake |> long_string()
    print("  long_string length: {length(ls)}\n")

    delete fake
}

// ============================================================
// Section 3: Random vectors
// ============================================================
// Faker generates random int2/3/4, uint2/3/4, float2/3/4 vectors.

def demo_faker_vectors() {
    print("\n=== faker vectors ===\n")

    var fake <- Faker()

    print("  random_int2:   {fake |> random_int2}\n")
    print("  random_int3:   {fake |> random_int3}\n")
    print("  random_float2: {fake |> random_float2}\n")
    print("  random_float3: {fake |> random_float3}\n")

    delete fake
}

// ============================================================
// Section 4: Random dates
// ============================================================
// Faker generates random dates within a configurable year range.

def demo_faker_dates() {
    print("\n=== faker dates ===\n")

    var fake <- Faker()

    // date: "DayOfWeek, MonthName DD, YYYY"
    print("  date: {fake |> date}\n")
    print("  date: {fake |> date}\n")

    // day: "DayOfWeek"
    print("  day:  {fake |> day}\n")

    delete fake
}

// ============================================================
// Section 5: Customizing Faker
// ============================================================
// Faker has configurable fields: min_year, total_years, max_long_string.
// You can also provide your own RNG iterator.

def demo_faker_custom() {
    print("\n=== faker custom ===\n")

    var fake <- Faker()
    // Restrict year range for dates
    fake.min_year = 2020u
    fake.total_years = 5u
    print("  date (2020-2025): {fake |> date}\n")

    // Limit long string length
    fake.max_long_string = 32u
    let ls = fake |> long_string()
    print("  short long_string length: {length(ls)} (max 32)\n")

    delete fake
}

// ============================================================
// Section 6: Fuzzer — fuzz()
// ============================================================
// fuzz(count, block) runs a block `count` times, catching any panics.
// Use it to test that your code handles arbitrary input gracefully.

def safe_divide(a, b : int) : int {
    if (b == 0) {
        return 0
    }
    return a / b
}

def demo_fuzz_basic() {
    print("\n=== fuzz basic ===\n")

    var fake <- Faker()

    // Run 100 iterations with random data
    var errors = 0
    fuzz(100) {
        let a = fake |> random_int
        let b = fake |> random_int
        let result = safe_divide(a, b)
        // If b != 0, verify the result makes sense
        if (b != 0 && result * b != a) {
            // Integer division truncation is normal, not an error
            pass
        }
    }
    print("  fuzz completed 100 iterations\n")

    delete fake
}

// ============================================================
// Section 7: fuzz_debug — verbose crash detection
// ============================================================
// fuzz_debug is identical to fuzz but does NOT catch panics.
// Replace fuzz with fuzz_debug when you need to see the actual
// error message and stack trace.

def demo_fuzz_debug() {
    print("\n=== fuzz_debug ===\n")

    var fake <- Faker()

    // fuzz_debug: panics are NOT swallowed
    fuzz_debug(50) {
        let a = fake |> random_int
        let b = fake |> random_int
        // safe_divide handles b==0, so this won't panic
        safe_divide(a, b)
    }
    print("  fuzz_debug completed 50 iterations (no panics)\n")

    delete fake
}

// ============================================================
// Section 8: Property-based testing pattern
// ============================================================
// Combine faker + fuzzer for property-based testing:
// generate random inputs and verify invariants hold.

def clamp_value(x, lo, hi : int) : int {
    if (x < lo) {
        return lo
    }
    if (x > hi) {
        return hi
    }
    return x
}

def demo_property_testing() {
    print("\n=== property-based testing ===\n")

    var fake <- Faker()
    var violations = 0

    fuzz(1000) {
        let x = fake |> random_int
        let lo = -100
        let hi = 100
        let result = clamp_value(x, lo, hi)

        // Property: result is always in [lo, hi]
        if (result < lo || result > hi) {
            violations ++
        }
    }
    print("  clamp property violations: {violations}/1000\n")

    delete fake
}

// ============================================================
// Section 9: Testing string operations with faker
// ============================================================

def reverse_string(s : string) : string {
    return build_string() <| $(var w) {
        var i = length(s) - 1
        while (i >= 0) {
            w |> write(slice(s, i, i + 1))
            i --
        }
    }
}

def demo_string_testing() {
    print("\n=== string fuzz testing ===\n")

    var fake <- Faker()
    var failures = 0

    fuzz(100) {
        let s = fake |> any_string()
        let reversed = reverse_string(s)

        // Property: reversing twice gives back original
        let double_rev = reverse_string(reversed)
        if (double_rev != s) {
            failures ++
        }

        // Property: reverse has same length
        if (length(reversed) != length(s)) {
            failures ++
        }
    }
    print("  string reverse failures: {failures}/100\n")

    delete fake
}

// === Summary ===
//
// +----------------------------+----------------------------------------------+
// | Function                   | Description                                  |
// +----------------------------+----------------------------------------------+
// | Faker()                    | Create faker with default RNG                |
// | faker |> random_int        | Random int (full range)                      |
// | faker |> random_uint       | Random uint (full range)                     |
// | faker |> random_float      | Random float (raw bits — may be NaN/Inf)     |
// | (faker |> random_uint) % 2u | Random bool (manual)                         |
// | faker |> any_string()      | Random string                                |
// | faker |> any_file_name     | Random file-safe name                        |
// | faker |> long_string()     | Random byte string (up to max_long_string)   |
// | faker |> date              | Random "DayOfWeek, Month DD, YYYY"           |
// | faker |> day               | Random day of week                           |
// | fuzz(count) <| block       | Run block count times, catch panics          |
// | fuzz_debug(count) <| block | Run block count times, panics propagate      |
// +----------------------------+----------------------------------------------+

[export]
def main() {
    demo_faker_basics()
    demo_faker_strings()
    demo_faker_vectors()
    demo_faker_dates()
    demo_faker_custom()
    demo_fuzz_basic()
    demo_fuzz_debug()
    demo_property_testing()
    demo_string_testing()
    print("done\n")
}

// Note: Output contains random values — exact numbers will differ per run.
// The structure of the output is deterministic, but values are random.
