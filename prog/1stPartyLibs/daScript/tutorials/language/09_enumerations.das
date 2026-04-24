// Tutorial 9: Enumerations and Bitfields
//
// This tutorial covers:
//   - Enum declaration and values
//   - Dot-syntax access: EnumName.Value
//   - Specifying underlying type (int8, uint, etc.)
//   - Iterating enums with each() from daslib/enum_trait
//   - Converting enums to/from strings
//   - Bitfield types and flag operations
//
// Run: daslang.exe tutorials/language/09_enumerations.das

options gen2

require daslib/enum_trait

// === Enum declaration ===
// Values are auto-numbered starting from 0.

enum Color {
    Red         // 0
    Green       // 1
    Blue        // 2
    Yellow      // 3
}

// === Enum with explicit values ===

enum HttpStatus {
    OK = 200
    NotFound = 404
    ServerError = 500
}

// === Enum with underlying type ===
// By default enums use int. You can specify a smaller type.

enum Direction : uint8 {
    North
    South
    East
    West
}

// === Helper function ===

def describeColor(c : Color) : string {
    if (c == Color.Red) {
        return "red"
    } elif (c == Color.Green) {
        return "green"
    } elif (c == Color.Blue) {
        return "blue"
    } else {
        return "yellow"
    }
}

// === Bitfield declaration ===
// Bitfields represent a set of flags packed into an integer.

bitfield FilePermissions {
    read
    write
    execute
    hidden
}

[export]
def main {

    // === Using enums ===
    // Access values with dot syntax: EnumName.Value
    let favorite = Color.Blue
    print("favorite = {favorite}\n")

    // Enums are their own type — not interchangeable with int
    let status = HttpStatus.OK
    print("status = {status}\n")

    // Cast to underlying integer
    print("Blue as int = {int(favorite)}\n")
    print("OK as int = {int(status)}\n")

    // === Comparing enums ===
    if (favorite == Color.Blue) {
        print("favorite is Blue\n")
    }

    print("describeColor(Green) = {describeColor(Color.Green)}\n")

    // === Iterating over all enum values ===
    // Use each() from daslib/enum_trait (not the deprecated each_enum).
    // Pass any value of the enum type to each().

    print("all colors: ")
    for (c in each(Color.Red)) {
        print("{c} ")
    }
    print("\n")

    print("all directions: ")
    for (d in each(Direction.North)) {
        print("{d} ")
    }
    print("\n")

    // === Enum names and count ===
    // typeinfo gives compile-time info about enum types.

    let numColors = typeinfo enum_length(type<Color>)
    print("number of colors = {numColors}\n")

    // === String conversion ===
    // string(enumValue) gives the value name
    let colorStr = string(Color.Green)
    print("string(Color.Green) = {colorStr}\n")

    // to_enum converts a string back to an enum value
    let parsed = to_enum(type<Color>, "Blue")
    print("to_enum(\"Blue\") = {parsed}\n")

    // to_enum with a fallback for invalid strings
    let unknown = to_enum(type<Color>, "Purple", Color.Red)
    print("to_enum(\"Purple\", fallback=Red) = {unknown}\n")

    // === Bitfields ===
    // Bitfields pack multiple boolean flags into a single integer.

    var perms : FilePermissions
    print("empty perms: {perms}\n")

    // Set individual flags with dot syntax
    perms.read = true
    perms.write = true
    print("read+write: {perms}\n")

    // Test individual flags — dot syntax returns bool
    if (perms.read) {
        print("has read permission\n")
    }
    if (!perms.execute) {
        print("no execute permission\n")
    }

    // You can also use |= with EnumName.Value to set flags
    var rwx : FilePermissions
    rwx |= FilePermissions.read
    rwx |= FilePermissions.write
    rwx |= FilePermissions.execute
    print("rwx = {rwx}\n")

    // Clear a flag with dot syntax, or toggle with ^=
    var flags : FilePermissions = rwx
    flags.write = false               // dot syntax to clear
    print("after clear write: {flags}\n")
    flags ^= FilePermissions.execute  // XOR to toggle
    print("after toggle execute: {flags}\n")
    print("flags.read = {flags.read}\n")

    // === bitfield_boost (advanced) ===
    // require daslib/bitfield_boost adds index-based access:
    //   flags[0] — read bit 0
    //   flags[1] = true — set bit 1
    //   each(flags) — iterate over all bits as bool values
}

// output:
// favorite = Blue
// status = OK
// Blue as int = 2
// OK as int = 200
// favorite is Blue
// describeColor(Green) = green
// all colors: Red Green Blue Yellow
// all directions: North South East West
// number of colors = 4
// string(Color.Green) = Green
// to_enum("Blue") = Blue
// to_enum("Purple", fallback=Red) = Red
// empty perms: (0)
// read+write: (read|write)
// has read permission
// no execute permission
// rwx = (read|write|execute)
// after clear write: (read|execute)
// after toggle execute: (read)
// flags.read = true
