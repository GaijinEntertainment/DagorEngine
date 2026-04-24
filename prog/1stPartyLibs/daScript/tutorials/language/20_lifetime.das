// Tutorial 20: Lifetime and Cleanup
//
// This tutorial covers:
//   - daslang memory model: no implicit cleanup
//   - delete for explicit cleanup
//   - var inscope for deterministic cleanup (RAII-like)
//   - Custom finalizers
//   - finally blocks
//   - When delete is/isn't needed
//
// Run: daslang.exe tutorials/language/20_lifetime.das

options gen2

// === Custom finalizer ===
// Define a 'finalize' function to run cleanup logic when delete is called.

struct Resource {
    name : string
    id : int
}

def finalize(var r : Resource) {
    print("  cleanup: {r.name} (id={r.id})\n")
    // After this, the struct fields are zeroed automatically
}

// === Function that creates a resource ===
def acquire_resource(n : string; i : int) : Resource {
    return Resource(name = n, id = i)
}

// === Function returning a non-copyable type ===
def make_data() : array<int> {
    var result : array<int>
    result |> push(10)
    result |> push(20)
    result |> push(30)
    return <- result
}

class MyClass {
    dummy : int
}

[export]
def main {

    // === Memory model ===

    // daslang does NOT automatically clean up local variables when they
    // go out of scope. There is no implicit destructor call.
    //
    // Containers (arrays, tables) will have their memory reclaimed eventually,
    // but finalizers are NOT called automatically. If you need deterministic
    // cleanup (e.g., closing a file, releasing a lock), you must use one of:
    //   - explicit 'delete'
    //   - 'var inscope' for automatic cleanup at end of scope
    //
    // For simple programs, you often don't need to worry — the process
    // reclaims all memory on exit. But for long-running applications
    // or resource management, use the tools below.

    // === Explicit delete ===

    // 'delete' zeroes out a variable and calls its finalizer.
    // For containers (array, table), it frees all elements.

    print("=== explicit delete ===\n")
    var data <- make_data()
    print("  before delete: {data}\n")
    delete data
    print("  after delete: length={length(data)}\n")

    // === Custom finalizer ===
    // Our Resource struct has a custom finalize function.
    print("=== custom finalizer ===\n")
    var res = acquire_resource("Database", 1)
    delete res
    // The finalize function printed the cleanup message

    // === var inscope ===

    // 'var inscope' automatically calls 'delete' at the end of the scope.
    // This is the recommended way to ensure deterministic cleanup.
    // Think of it like RAII in C++ or 'defer' in Go.

    print("=== var inscope ===\n")
    {
        var inscope r1 = acquire_resource("File", 2)
        var inscope r2 = acquire_resource("Socket", 3)
        print("  using r1 and r2...\n")
        // delete r2 then delete r1 called automatically here (reverse order)
    }
    print("  inscope block ended\n")

    // === finally blocks ===

    // 'finally' runs cleanup code when a block exits,
    // even if an error occurs.

    print("=== finally ===\n")
    var counter = 0
    for (i in range(3)) {
        counter += 1
    } finally {
        print("  loop done, counter={counter}\n")
    }

    // === Heap pointers (new) ===

    // For class instances created with 'new', delete requires 'unsafe':
    //   var p = new MyClass()
    //   // ... use p ...
    //   unsafe { delete p }
    //
    // Or use 'var inscope' for automatic cleanup:
    unsafe { // needs 'unsafe', because deleting classes is unsafe
        var inscope p = new MyClass()
    }
    //   // deleted automatically at end of scope

    // === When to use what ===

    // In everyday code:
    //   - Most code doesn't need delete — memory is reclaimed on exit
    //   - Use 'var inscope' when you need deterministic cleanup
    //     (file handles, network connections, locks, custom finalizers)
    //   - Use 'delete' for immediate explicit cleanup
    //   - Write custom 'finalize' functions for cleanup logic
    //
    // For containers (array, table):
    //   - 'delete' empties and zeroes the container
    //   - Without delete, memory is NOT freed until program ends
    //   - For long-running code, delete or inscope prevents leaks

    print("=== done ===\n")
}

// output:
// === explicit delete ===
//   before delete: [[ 10; 20; 30]]
//   after delete: length=0
// === custom finalizer ===
//   cleanup: Database (id=1)
// === var inscope ===
//   using r1 and r2...
//   cleanup: Socket (id=3)
//   cleanup: File (id=2)
//   inscope block ended
// === finally ===
//   loop done, counter=3
// === done ===
