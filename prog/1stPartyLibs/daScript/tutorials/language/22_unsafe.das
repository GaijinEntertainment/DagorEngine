// Tutorial 22: Unsafe and Pointers
//
// This tutorial covers:
//   - The unsafe block and expression
//   - addr() — getting a pointer to a variable
//   - Dereferencing with * and deref()
//   - Pointer nullability and null checks
//   - reinterpret — raw bit reinterpretation
//   - Pointer arithmetic and indexing
//   - When unsafe is required
//
// Run: daslang.exe tutorials/language/22_unsafe.das

options gen2

[export]
def main {

    // === addr() — get pointer to variable ===
    // addr() creates a pointer (T?) to a local variable.
    // Requires unsafe because the pointer could outlive the variable.

    print("addr:\n")
    var x = 42
    unsafe {
        var p = addr(x)       // p is int?
        print("  *p = {*p}\n")

        // Modify through pointer
        *p = 100
        print("  after *p = 100: x = {x}\n")
    }

    // === Dereferencing ===
    // * dereferences a pointer. Panics if null.
    // deref(p) is the same as *p.

    print("deref:\n")
    unsafe {
        var y = 7
        var p = addr(y)
        print("  *p = {*p}\n")
        print("  deref(p) = {deref(p)}\n")
    }

    // Null pointer dereference panics (can be caught with try/recover):
    try {
        var p : int?    // null pointer
        unsafe {
            print("  {*p}\n")       // panics
        }
    } recover {
        print("  caught null dereference\n")
    }

    // === Null checks ===
    // Pointers are nullable. Check with != null or ??.

    print("null check:\n")
    unsafe {
        var a = 42
        var p = addr(a)
        var q : int?            // null

        if (p != null) {
            print("  p is valid: {*p}\n")
        }
        if (q == null) {
            print("  q is null\n")
        }
    }

    // === Pointer arithmetic ===
    // Pointer indexing with p[i] is unsafe.

    print("pointer indexing:\n")
    var arr <- [10, 20, 30, 40, 50]
    unsafe {
        var p = addr(arr[0])
        print("  p[0] = {p[0]}\n")
        print("  p[2] = {p[2]}\n")
        print("  p[4] = {p[4]}\n")
    }

    // === reinterpret — raw bit cast ===
    // Reinterprets the raw bits of a value as a different type.
    // Extremely dangerous — types don't need any relationship.

    print("reinterpret:\n")
    unsafe {
        let float_val = 1.0
        let int_bits = reinterpret<int> float_val
        print("  1.0 as int bits: 0x{int_bits:08x}\n")   // IEEE 754: 0x3f800000

        let back = reinterpret<float> int_bits
        print("  back to float: {back}\n")
    }

    // === unsafe expression form ===
    // For single expressions, use unsafe() instead of a block.

    print("unsafe expression:\n")
    var z = 99
    let ptr = unsafe(addr(z))
    unsafe {
        print("  unsafe(addr(z)) = {*ptr}\n")
    }

    // === When unsafe is required ===
    //
    // 1.  addr(x)                  — pointer can outlive variable
    // 2.  delete pointer           — other refs may still exist
    // 3.  upcast<Derived> base     — type may not match at runtime
    // 4.  reinterpret<T> value     — raw bit reinterpretation
    // 5.  pointer[index]           — out-of-bounds possible
    // 6.  table[key] (default)     — inserts on access
    //         (disable with: options unsafe_table_lookup = false)
    // 7.  variant .field access    — potential type mismatch
    // 8.  lambda capture by ref    — capture(ref(var))
    // 9.  moving/copying classes   — lifetime issues
    // 10. pointer arithmetic       — can create invalid pointers
    //
    // Note: unsafe is NOT inherited into lambdas or generators.
    // Each lambda/generator needs its own unsafe block.

    // === Summary ===
    // - unsafe { } — block form, unsafe() — expression form
    // - addr(x) creates T? pointer to x
    // - *p / deref(p) — dereference (panics if null)
    // - reinterpret<T> — raw bit cast (dangerous)
    // - Use unsafe sparingly — it disables safety checks
    // - Prefer safe alternatives: ??, ?as, var inscope

    print("done\n")
}

// output:
// addr:
//   *p = 42
//   after *p = 100: x = 100
// deref:
//   *p = 7
//   deref(p) = 7
//   caught null dereference
// null check:
//   p is valid: 42
//   q is null
// pointer indexing:
//   p[0] = 10
//   p[2] = 30
//   p[4] = 50
// reinterpret:
//   1.0 as int bits: 0x3f800000
//   back to float: 1
// unsafe expression:
//   unsafe(addr(z)) = 99
// done
