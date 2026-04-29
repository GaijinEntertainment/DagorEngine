// Tutorial 36: Pointers
//
// This tutorial covers:
//   - Pointer types (T?, void?)
//   - Creating pointers: new, addr(), safe_addr()
//   - Dereferencing: *, deref(), auto-deref for fields
//   - Null safety: null checks, ?., ??, ?[, try/recover
//   - Passing pointers to functions
//   - Heap allocation and deletion
//   - Pointer arithmetic and indexing
//   - intptr() — pointer to integer
//   - reinterpret — raw bit reinterpretation
//
// Run: daslang.exe tutorials/language/36_pointers.das

options gen2

require daslib/safe_addr

struct Point {
    x : float
    y : float
}

// A function that takes a pointer to a struct
def move_point(var p : Point?; dx, dy : float) : Point? {
    if (p != null) {
        p.x += dx           // auto-deref: no -> needed, just use .
        p.y += dy
    }
    return p
}

// A function that takes a pointer to an int and modifies it
def double_value(var p : int?) {
    *p = *p + *p
}

[export]
def main() {

    // === Pointer types ===
    //
    // In daslang, T? is a pointer to T. Pointers can be null.
    //   int?    — pointer to int
    //   Point?  — pointer to Point struct
    //   void?   — untyped (void) pointer
    //
    // Pointers to structs are common — new always returns T?.

    // === Creating pointers with new ===
    //
    // The new operator allocates on the heap and returns a pointer.

    print("=== new ===\n")
    var p = new Point(x = 3.0, y = 4.0)
    print("  p.x = {p.x}, p.y = {p.y}\n")          // auto-deref on field access

    // new with no arguments — fields get default values (zero)
    var q = new Point()
    print("  q.x = {q.x}, q.y = {q.y}\n")

    // Don't forget to clean up heap allocations (or use var inscope)
    unsafe {
        delete p
        delete q
    }

    // === var inscope — automatic cleanup ===
    //
    // var inscope adds a finally block that deletes the pointer
    // when it goes out of scope. No manual delete needed.

    print("=== var inscope ===\n")
    unsafe {
        var inscope pt = new Point(x = 1.0, y = 2.0)
        print("  pt.x = {pt.x}, pt.y = {pt.y}\n")
        // pt is automatically deleted at end of scope
    }

    // === addr() — pointer to existing variable ===
    //
    // addr(x) returns a pointer to variable x.
    // Requires unsafe because the pointer could outlive the variable.

    print("=== addr ===\n")
    var a = 42
    unsafe {
        var pa = addr(a)          // pa is int?
        print("  *pa = {*pa}\n")
        *pa = 100                 // modify through pointer
        print("  a = {a}\n")      // a is now 100
    }

    // === safe_addr() — without unsafe ===
    //
    // safe_addr from daslib/safe_addr returns a temporary pointer (T?#)
    // that is safe to use within the current scope, no unsafe needed.

    print("=== safe_addr ===\n")
    var b = 77
    var pb = safe_addr(b)         // pb is int?# (temporary pointer)
    print("  *pb = {*pb}\n")

    // === Dereferencing ===
    //
    // *p and deref(p) follow the pointer to the value.
    // They panic if the pointer is null.

    print("=== deref ===\n")
    unsafe {
        var c = 5
        var pc = addr(c)
        print("  *pc = {*pc}\n")
        print("  deref(pc) = {deref(pc)}\n")
    }

    // For struct pointers, field access auto-dereferences:
    //   p.x  is the same as  (*p).x
    // No -> operator needed (unlike C/C++).

    // === Null pointers ===
    //
    // Uninitialized pointers are null (zero).
    // Dereferencing null panics.

    print("=== null ===\n")
    var np : int?                   // null by default
    print("  np is null: {np == null}\n")

    // Catch null dereference with try/recover:
    try {
        unsafe {
            print("  {*np}\n")      // panics — np is null
        }
    } recover {
        print("  caught null deref\n")
    }

    // === Safe navigation: ?. and ?? ===
    //
    // ?. returns null if the pointer is null (no panic).
    // ?? provides a default value when the left side is null.

    print("=== safe navigation ===\n")
    var sp = new Point(x = 10.0, y = 20.0)
    var nullp : Point?

    print("  sp?.x = {sp?.x ?? -1.0}\n")        // 10.0
    print("  nullp?.x = {nullp?.x ?? -1.0}\n")   // -1.0 (default)

    // ?. works on chains:
    //   a?.b?.c ?? default

    // === Passing pointers to functions ===

    print("=== pointer args ===\n")
    var pt2 = new Point(x = 1.0, y = 2.0)
    move_point(pt2, 3.0, 4.0)
    print("  after move: pt2.x = {pt2.x}, pt2.y = {pt2.y}\n")

    var val = 21
    unsafe {
        double_value(addr(val))
    }
    print("  after double: val = {val}\n")

    // === Pointer arithmetic (unsafe) ===
    //
    // Pointers can index into contiguous memory and be incremented.
    // All pointer arithmetic requires unsafe.

    print("=== pointer arithmetic ===\n")
    var arr <- [10, 20, 30, 40, 50]
    unsafe {
        var parr = addr(arr[0])
        // Index into pointer
        print("  parr[0] = {parr[0]}\n")
        print("  parr[2] = {parr[2]}\n")
        print("  parr[4] = {parr[4]}\n")

        // Pointer increment
        ++ parr                     // advance by one element
        print("  after ++: *parr = {*parr}\n")   // 20

        // Pointer addition
        parr += 2                   // advance by two more elements
        print("  after +=2: *parr = {*parr}\n")   // 40
    }

    // === void? — untyped pointer ===
    //
    // void? is a raw pointer with no type information.
    // Useful for interfacing with C/C++ APIs.
    // Must reinterpret back to a typed pointer to use.

    print("=== void pointer ===\n")
    var d = 123
    unsafe {
        var pd = addr(d)
        var vp : void? = reinterpret<void?> pd      // erase type
        var pd2 = reinterpret<int?> vp               // restore type
        print("  *pd2 = {*pd2}\n")
    }

    // === intptr() — pointer to integer ===
    //
    // intptr(p) converts a pointer to a uint64 representing its address.
    // Useful for debugging, hashing, or identity comparisons.

    print("=== intptr ===\n")
    var e = 42
    unsafe {
        var pe = addr(e)
        let address = intptr(pe)
        print("  address != 0: {address != uint64(0)}\n")
        print("  same pointer: {intptr(pe) == address}\n")
    }

    // === reinterpret — raw bit cast ===
    //
    // reinterpret<T> re-reads the raw bits as a different type.
    // Types must be the same size. Extremely dangerous.

    print("=== reinterpret ===\n")
    unsafe {
        let float_val = 1.0
        let int_bits = reinterpret<int> float_val
        print("  1.0 as int bits: 0x{int_bits:08x}\n")   // IEEE 754: 0x3f800000

        let back = reinterpret<float> int_bits
        print("  back to float: {back}\n")
    }

    // === Summary ===
    //
    // Type       | Meaning
    // -----------|----------------------------
    // int?       | Pointer to int (nullable)
    // Point?     | Pointer to struct (nullable)
    // void?      | Untyped pointer
    // T?#        | Temporary pointer (safe_addr)
    //
    // Operation       | Requires unsafe?
    // ----------------|------------------
    // new T()         | No
    // *p / deref(p)   | No (panics if null)
    // p.field         | No (auto-deref)
    // p?.field        | No (safe navigation)
    // p ?? default    | No (null coalescing)
    // addr(x)         | Yes
    // delete p        | Yes
    // p[i]            | Yes
    // ++p / p += N    | Yes
    // reinterpret<T>  | Yes
    // safe_addr(x)    | No (returns T?#)

    print("done\n")
}

// output:
// === new ===
//   p.x = 3, p.y = 4
//   q.x = 0, q.y = 0
// === var inscope ===
//   pt.x = 1, pt.y = 2
// === addr ===
//   *pa = 42
//   a = 100
// === safe_addr ===
//   *pb = 77
// === deref ===
//   *pc = 5
//   deref(pc) = 5
// === null ===
//   np is null: true
//   caught null deref
// === safe navigation ===
//   sp?.x = 10
//   nullp?.x = -1
// === pointer args ===
//   after move: pt2.x = 4, pt2.y = 6
//   after double: val = 42
// === pointer arithmetic ===
//   parr[0] = 10
//   parr[2] = 30
//   parr[4] = 50
//   after ++: *parr = 20
//   after +=2: *parr = 40
// === void pointer ===
//   *pd2 = 123
// === intptr ===
//   address != 0: true
//   same pointer: true
// === reinterpret ===
//   1.0 as int bits: 0x3f800000
//   back to float: 1
// done
