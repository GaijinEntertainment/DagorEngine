// Tutorial 26: Contracts
//
// This tutorial covers:
//   - Contract annotations for constraining generic functions
//   - Built-in contracts: [expect_ref], [expect_dim]
//   - Library contracts: [expect_any_array], [expect_any_numeric], etc.
//   - Combining contracts with && (AND), || (OR), ! (NOT)
//   - concept_assert for custom compile-time checks
//
// Run: daslang.exe tutorials/language/26_contracts.das

options gen2

require daslib/contracts

// === What are contracts? ===
// Contracts are compile-time constraints on generic function arguments.
// When a generic function is called, the contract checks whether each
// argument satisfies the required type constraint. If not, the function
// is not considered a match (overload resolution skips it).
//
// Syntax: [expect_something(arg_name)]
// The arg_name refers to a parameter of the function.

// === Built-in contracts (always available) ===
// These are always available without imports:
//   [expect_ref(arg)]     — argument must be a reference (var)
//   [expect_dim(arg)]     — argument must be a fixed-size array
//   [expect_any_vector(arg)] — argument must be a C++ vector type

// === Library contracts (require daslib/contracts) ===

// [expect_any_array] — matches array<T>, T[N], or das::vector<T>
[expect_any_array(arr)]
def print_first(arr) {
    if (length(arr) > 0) {
        print("  first element: {arr[0]}\n")
    } else {
        print("  empty array\n")
    }
}

// [expect_any_numeric] — matches int, uint, float, double, etc.
[expect_any_numeric(a), expect_any_numeric(b)]
def safe_add(a, b) {
    return a + b
}

// [expect_any_enum] — matches any enumeration type
[expect_any_enum(e)]
def enum_index(e) : int {
    return int(e)
}

enum Color {
    red
    green
    blue
}

// [expect_any_struct] — matches struct types (not classes)
struct Vec2 {
    x : float
    y : float
}

[expect_any_struct(s)]
def struct_info(s) {
    print("  got a struct\n")
}

// === Combining contracts ===

// AND (&&) — both constraints must match
[expect_any_numeric(a) && expect_any_numeric(b)]
def multiply_nums(a, b) {
    return a * b
}

// OR (||) — at least one constraint must match
variant IntOrFloat {
    i : int
    f : float
}

[expect_any_tuple(x) || expect_any_variant(x)]
def is_compound(x) : bool {
    return true
}

def is_compound(x) : bool {    // fallback for non-compound types
    return false
}

// NOT (!) — constraint must NOT match
[!expect_any_numeric(x)]
def not_a_number(x) : string {
    return "not numeric"
}

// Fallback that accepts anything (including numeric)
// When x IS numeric, the contract [!expect_any_numeric] fails,
// so this fallback is selected instead.
def not_a_number(x : int) : string {
    return "is numeric"
}

// === concept_assert — custom compile-time checks ===
// concept_assert(condition, "message")
// Like static_assert, but reports the error at the CALL SITE,
// not inside the generic function. This gives clear error messages.

// [expect_any_array] + concept_assert — combined constraint
def sum_all(arr : array<auto(TT)>) : TT {
    concept_assert(typeinfo is_numeric(type<TT>), "sum_all requires numeric elements")
    var total = arr[0]
    for (i in range(1, length(arr))) {
        total += arr[i]
    }
    return total
}

// === Main ===

[export]
def main {

    // [expect_dim] — see summary for built-in contracts
    // [expect_any_array] — any array type
    print("expect_any_array:\n")
    var nums : array<int>
    nums |> push(1); nums |> push(2); nums |> push(3)
    print_first(nums)

    // [expect_any_numeric] — numeric types only
    print("expect_any_numeric:\n")
    print("  safe_add(3, 4) = {safe_add(3, 4)}\n")
    print("  safe_add(1.5, 2.5) = {safe_add(1.5, 2.5)}\n")
    // safe_add("a", "b")  // would not compile — string is not numeric

    // [expect_any_enum]
    print("expect_any_enum:\n")
    print("  enum_index(Color.green) = {enum_index(Color.green)}\n")

    // [expect_any_struct]
    print("expect_any_struct:\n")
    struct_info(Vec2(x = 1.0, y = 2.0))

    // AND (&&)
    print("AND contract:\n")
    print("  multiply_nums(3, 4) = {multiply_nums(3, 4)}\n")

    // OR (||)
    print("OR contract:\n")
    var t = (1, 2.0)
    print("  tuple is compound? {is_compound(t)}\n")
    var v = IntOrFloat(i = 5)
    print("  variant is compound? {is_compound(v)}\n")
    print("  int is compound? {is_compound(42)}\n")

    // NOT (!)
    print("NOT contract:\n")
    print("  not_a_number(\"hello\") = {not_a_number("hello")}\n")
    print("  not_a_number(42) = {not_a_number(42)}\n")

    // concept_assert
    print("concept_assert:\n")
    var ints : array<int>
    ints |> push(10); ints |> push(20); ints |> push(30)
    print("  sum_all([10,20,30]) = {sum_all(ints)}\n")
    var floats : array<float>
    floats |> push(1.0); floats |> push(2.0); floats |> push(3.0)
    print("  sum_all([1,2,3]) = {sum_all(floats)}\n")
    // var strs : array<string>
    // strs |> push("a")
    // sum_all(strs)  // concept_assert error: "sum_all requires numeric elements"

    // === Summary ===
    // Contracts constrain generic function arguments at compile time.
    //
    // Built-in (always available):
    //   [expect_ref(arg)]     — must be a reference
    //   [expect_dim(arg)]     — must be a fixed-size array
    //
    // From daslib/contracts:
    //   [expect_any_array(a)]     — array<T>, T[N]
    //   [expect_any_numeric(a)]   — int, float, etc.
    //   [expect_any_enum(a)]      — any enum
    //   [expect_any_struct(a)]    — any struct
    //   [expect_any_tuple(a)]     — any tuple
    //   [expect_any_variant(a)]   — any variant
    //   [expect_any_function(a)]  — any function type
    //   [expect_any_lambda(a)]    — any lambda
    //   [expect_any_bitfield(a)]  — any bitfield
    //   [expect_pointer(a)]       — any pointer
    //   [expect_class(a)]         — class pointer
    //
    // Combining:
    //   [a && b]   — both must match
    //   [a || b]   — at least one matches
    //   [!a]       — must NOT match
    //   [a ^^ b]   — exactly one matches (XOR)
    //
    // concept_assert(condition, "msg")
    //   Reports error at caller's line, not inside the generic.

    print("done\n")
}

// output:
// expect_any_array:
//   first element: 1
// expect_any_numeric:
//   safe_add(3, 4) = 7
//   safe_add(1.5, 2.5) = 4
// expect_any_enum:
//   enum_index(Color.green) = 1
// expect_any_struct:
//   got a struct
// AND contract:
//   multiply_nums(3, 4) = 12
// OR contract:
//   tuple is compound? true
//   variant is compound? true
//   int is compound? false
// NOT contract:
//   not_a_number("hello") = not numeric
//   not_a_number(42) = is numeric
// concept_assert:
//   sum_all([10,20,30]) = 60
//   sum_all([1,2,3]) = 6
// done
