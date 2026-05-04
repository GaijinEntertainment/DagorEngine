// Demonstrates SQVM::Call self-recursion via class constructor (1.5)
//
// When a class is called to create an instance, Call() does two things:
//   1. CreateClassInstance() - allocates the instance
//   2. Call(constructor, ...) - calls the constructor on the new instance
//
// sqvm.cpp:2436-2448 (case OT_CLASS):
//   Call(class)                       // entry: closure type is OT_CLASS
//     -> CreateClassInstance(class)   // line 2440 - allocates instance
//     -> Call(constructor, ...)       // line 2445 - re-enters Call() with OT_CLOSURE
//       -> Execute() runs constructor body
//
// If the constructor body creates another instance of the same class,
// Execute() emits _OP_CALL with the class object, which calls Call(class)
// again. Each level adds TWO native Call() frames (class + constructor):
//
//   Call(MyClass)          -- OT_CLASS dispatch, CreateClassInstance
//     Call(constructor)    -- OT_CLOSURE dispatch -> Execute
//       Call(MyClass)      -- from `this.getclass()()` in constructor body
//         Call(constructor)
//           Call(MyClass)
//             ...
//
// No depth limit exists in Call() itself.
//
// RESULT: Unlike _tostring/_get metamethod recursion (which the VM catches
// at ~99 deep with "Native stack overflow"), this path causes a HARD CRASH
// of the process. The Call(OT_CLASS) branch does extra work per level
// (CreateClassInstance + stack manipulation) consuming more native stack
// per recursion step, so the process segfaults before the VM's stack
// overflow probe (in Execute) gets a chance to fire.

// ---- Controlled version: shows the recursion pattern safely ----

local depth = 0
local MAX = 10

class MyClass {
  child = null
  constructor() {
    depth++
    if (depth <= 5)
      println($"  Call(MyClass) -> Call(ctor) depth={depth}")
    // this.getclass() returns the OT_CLASS object
    // calling it triggers Call(class) -> CreateClassInstance -> Call(ctor)
    if (depth < MAX)
      this.child = this.getclass()()
  }
}

println($"=== Controlled run (max {MAX}) ===")
depth = 0
try {
  local obj = MyClass()
  println($"Completed: depth reached {depth}")
} catch(e) {
  println($"Caught after depth={depth}: {e}")
}

// ---- Unguarded version ----

class B {
  child = null
  constructor() {
    this.child = this.getclass()()
  }
}
try {
  local b = B()
} catch(e) {
  println($"Caught: {e}")  // never reached - process is dead
}
