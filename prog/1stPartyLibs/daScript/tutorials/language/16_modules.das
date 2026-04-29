// Tutorial 16: Modules and Program Structure
//
// This tutorial covers:
//   - File layout (options, module, require)
//   - Creating and using a module from a separate file
//   - Public vs private visibility
//   - Module-qualified function calls (::)
//   - Module declaration and modifiers
//   - Common standard library modules
//   - Options declaration
//
// Prerequisites:
//   tutorial_helpers.das (in the same directory) provides:
//     struct Item { name:string; value:int }
//     make_item(), describe(), MAX_ITEMS (public)
//     internal_id() (private — not accessible from here)
//
// Run: daslang.exe tutorials/language/16_modules.das

options gen2

// === Imports ===
// 'require' brings in another module's types and functions.
// A file in the same directory can be required by its module name.
require tutorial_helpers   // our own module (tutorial_helpers.das)
require math               // sin, cos, sqrt, etc.

[export]
def main {

    // === File layout ===

    // Every daslang file follows this layout:
    //   1. options gen2                 // compiler options (optional)
    //   2. module my_name public       // module name (optional)
    //   3. require math                // imports
    //   4. struct, enum, let, var, def // declarations
    //
    // The 'module' line must appear before any declarations.
    // 'options' and 'require' can be in any order, but convention is:
    //   options → module → require → declarations

    // === Using our own module ===

    // tutorial_helpers.das defines:
    //   module tutorial_helpers public
    // So all its public declarations are available here.

    // Create an Item using the imported struct and function
    let sword = make_item("Sword", 150)
    print("item: {describe(sword)}\n")

    // Use the public constant
    print("max items allowed: {MAX_ITEMS}\n")

    // === Qualified calls (::) ===
    // Prefix with module name to be explicit:
    let shield = tutorial_helpers::make_item("Shield", 80)
    print("qualified: {tutorial_helpers::describe(shield)}\n")

    // Useful when two modules export the same function name.
    // Without qualification, the compiler reports an ambiguity error.

    // === Private members are hidden ===
    // tutorial_helpers has a 'private' function: internal_id()
    // Trying to call it from here is a compile error:
    //   internal_id(sword)   // ERROR: can't access private function

    // === Using standard library ===

    let root = math::sqrt(144.0)
    print("math::sqrt(144) = {root}\n")

    // === Module declaration ===

    // The 'module' keyword names the current file:
    //     module my_lib
    //
    // If omitted, the name defaults to the filename.
    //
    // Modifiers:
    //   module my_lib public   -- all declarations default to public
    //   module my_lib private  -- all declarations default to private
    //   module my_lib shared   -- shared across contexts (like a built-in)
    //
    // Modifiers combine:
    //   module my_lib shared public

    // === Visibility ===

    // Each declaration can override the module default:
    //
    //   def public helper(x : int) : int { ... }    // always public
    //   def private internal() { ... }               // always private
    //   struct public MyData { ... }                  // always public
    //
    // Private declarations are invisible to modules that 'require' you.

    // === Re-exporting ===

    // 'require ... public' re-exports a module to your importers:
    //   require math public
    //
    // Anyone who requires YOUR module will also get math's functions.

    // === Common standard library modules ===

    // Built-in modules:
    //   require math              -- math functions
    //   require strings           -- string parsing and formatting
    //   require daslib/fio               -- file I/O
    //   require daslib/rtti              -- runtime type information
    //
    // daslib modules (user-level standard library):
    //   require daslib/json       -- JSON parsing
    //   require daslib/regex      -- regular expressions
    //   require daslib/algorithm  -- sorting, searching
    //   require daslib/strings_boost  -- advanced string utils
    //   require daslib/enum_trait     -- enum utilities

    // === Options ===

    // 'options' sets compiler flags for the file:
    //   options gen2                              // enable gen2 syntax
    //   options no_unused_block_arguments = false  // explicit flag value
    //   options no_aot = true, rtti = true         // multiple on one line
    //
    // A bare option name is shorthand for = true:
    //   options gen2    is equivalent to    options gen2 = true

    // === Special module references ===

    // _::  refers to the current module (for generics)
    // __:: refers to the current module ONLY (strict)
    //
    // These are important for generic functions where the
    // call should resolve in the module that instantiates
    // the generic, not the module that defines it.

    print("done\n")
}

// output:
// item: Sword (worth 150)
// max items allowed: 100
// qualified: Shield (worth 80)
// math::sqrt(144) = 12
// done
