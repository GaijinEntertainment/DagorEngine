// Tutorial 47: Data Walking with DapiDataWalker
//
// This tutorial covers:
//   - Subclassing DapiDataWalker to inspect daScript values at runtime
//   - Walking scalar types (int, float, string, bool)
//   - Walking structures with field names via StructInfo/VarInfo
//   - Walking arrays, tables, tuples, and variants
//   - Walking enumerations and bitfields
//   - Building a JSON serializer as a practical example
//   - Filtering traversal with canVisit methods
//   - Mutating data in-place through walker callbacks
//
// Prerequisites: None (basic daScript knowledge only)
//
// Key concepts:
//   - DapiDataWalker is a visitor pattern for daScript data at runtime
//   - Override scalar callbacks (Int, Float, String, ...) to handle values
//   - Override before/after callbacks for containers (structures, arrays, etc.)
//   - All canVisit methods default to true — override to prune traversal
//   - Scalar callbacks receive mutable references — values can be modified
//   - walk_data takes a smart_ptr<DataWalker> adapter, a data pointer, and TypeInfo
//
// Run: daslang.exe tutorials/language/47_data_walker.das

options gen2
options rtti

require daslib/rtti
require daslib/debugger
require daslib/strings_boost


// ============================================================
// Section 1: Minimal walker — scalar types
// ============================================================
//
// A DapiDataWalker subclass overrides only the callbacks you care
// about.  All methods default to no-op, so a minimal walker that
// just prints integers and floats is very small.

class ScalarPrinter : DapiDataWalker {
    def override Int(var value : int&) : void {
        print("  int: {value}\n")
    }
    def override Float(var value : float&) : void {
        print("  float: {value}\n")
    }
    def override String(var value : string&) : void {
        print("  string: \"{value}\"\n")
    }
    def override Bool(var value : bool&) : void {
        print("  bool: {value}\n")
    }
}

def demo_scalar_types() {
    print("\n=== Section 1: Scalar types ===\n")

    // Step 1: create the walker and its adapter
    var walker = new ScalarPrinter()
    var inscope adapter <- make_data_walker(walker)

    // Step 2: walk various scalar values
    // typeinfo rtti_typeinfo(var) returns the TypeInfo for any variable
    var x = 42
    print("Walking int 42:\n")
    unsafe {
        adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
    }

    var f = 3.14
    print("Walking float 3.14:\n")
    unsafe {
        adapter |> walk_data(addr(f), typeinfo rtti_typeinfo(f))
    }

    var s = "hello"
    print("Walking string \"hello\":\n")
    unsafe {
        adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
    }

    var b = true
    print("Walking bool true:\n")
    unsafe {
        adapter |> walk_data(addr(b), typeinfo rtti_typeinfo(b))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 2: Structures
// ============================================================
//
// The walker calls beforeStructure/afterStructure around the whole
// struct, and beforeStructureField/afterStructureField around each
// field.  StructInfo.name gives the struct type name, VarInfo.name
// gives the field name.

struct Vec3 {
    x : float
    y : float
    z : float
}

struct Player {
    name : string
    health : int
    pos : Vec3
}

class StructPrinter : DapiDataWalker {
    indent : int = 0

    def pad() {
        for (_ in range(indent)) {
            print("  ")
        }
    }

    def override beforeStructure(ps : void?; si : StructInfo) : void {
        print("{si.name} \{\n")
        indent++
    }

    def override afterStructure(ps : void?; si : StructInfo) : void {
        indent--
        self->pad()
        print("\}\n")
    }

    def override beforeStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        self->pad()
        print("{vi.name} = ")
    }

    def override afterStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        pass
    }

    def override Int(var value : int&) : void {
        print("{value}\n")
    }

    def override Float(var value : float&) : void {
        print("{value}\n")
    }

    def override String(var value : string&) : void {
        print("\"{value}\"\n")
    }
}

def demo_structures() {
    print("\n=== Section 2: Structures ===\n")

    var walker = new StructPrinter()
    var inscope adapter <- make_data_walker(walker)

    var player = Player(name = "Alice", health = 100, pos = Vec3(x = 1.0, y = 2.5, z = -3.0))
    print("Walking Player:\n")
    unsafe {
        adapter |> walk_data(addr(player), typeinfo rtti_typeinfo(player))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 3: Arrays and tables
// ============================================================
//
// Dynamic arrays trigger beforeArray/afterArray plus the element
// callbacks.  Fixed-size arrays (dim) trigger beforeDim/afterDim.
// Tables trigger beforeTable/afterTable with key/value pairs.

class ContainerPrinter : DapiDataWalker {
    indent : int = 0

    def pad() {
        for (_ in range(indent)) {
            print("  ")
        }
    }

    // --- arrays ---
    def override beforeArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        self->pad()
        print("array[{int(count)}] = [\n")
        indent++
    }

    def override afterArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        indent--
        self->pad()
        print("]\n")
    }

    def override beforeArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        self->pad()
        print("[{int(index)}] = ")
    }

    def override afterArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        print("\n")
    }

    // --- tables ---
    def override beforeTable(pa : DapiTable; ti : TypeInfo) : void {
        self->pad()
        print("table[{int(pa.size)}] = \{\n")
        indent++
    }

    def override afterTable(pa : DapiTable; ti : TypeInfo) : void {
        indent--
        self->pad()
        print("\}\n")
    }

    def override beforeTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        self->pad()
    }

    def override afterTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        print(" => ")
    }

    def override afterTableValue(pa : DapiTable; ti : TypeInfo; pv : void?; kv : TypeInfo; index : uint; last : bool) : void {
        print("\n")
    }

    // --- scalars ---
    def override Int(var value : int&) : void {
        print("{value}")
    }

    def override Float(var value : float&) : void {
        print("{value}")
    }

    def override String(var value : string&) : void {
        print("\"{value}\"")
    }
}

def demo_arrays_and_tables() {
    print("\n=== Section 3: Arrays and tables ===\n")

    var walker = new ContainerPrinter()
    var inscope adapter <- make_data_walker(walker)

    var nums = [10, 20, 30, 40, 50]
    print("Walking array<int>:\n")
    unsafe {
        adapter |> walk_data(addr(nums), typeinfo rtti_typeinfo(nums))
    }

    var scores : table<string; int>
    scores |> insert("Alice", 95)
    scores |> insert("Bob", 87)
    scores |> insert("Charlie", 72)
    print("\nWalking table<string;int>:\n")
    unsafe {
        adapter |> walk_data(addr(scores), typeinfo rtti_typeinfo(scores))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 4: Tuples and variants
// ============================================================
//
// Tuples walk each element by index.  Variants walk only the
// active alternative — you see which variant is active from te
// TypeInfo passed to beforeVariant/afterVariant.

typedef Result = variant<ok : int; err : string>

class TupleVariantPrinter : DapiDataWalker {
    indent : int = 0

    def pad() {
        for (_ in range(indent)) {
            print("  ")
        }
    }

    def override beforeTuple(ps : void?; ti : TypeInfo) : void {
        self->pad()
        print("tuple(\n")
        indent++
    }

    def override afterTuple(ps : void?; ti : TypeInfo) : void {
        indent--
        self->pad()
        print(")\n")
    }

    def override beforeTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        self->pad()
        print("_{idx} = ")
    }

    def override afterTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        print("\n")
    }

    def override beforeVariant(ps : void?; ti : TypeInfo) : void {
        self->pad()
        print("variant = ")
    }

    def override afterVariant(ps : void?; ti : TypeInfo) : void {
        print("\n")
    }

    def override Int(var value : int&) : void {
        print("{value}")
    }

    def override Float(var value : float&) : void {
        print("{value}")
    }

    def override String(var value : string&) : void {
        print("\"{value}\"")
    }
}

def demo_tuples_and_variants() {
    print("\n=== Section 4: Tuples and variants ===\n")

    var walker = new TupleVariantPrinter()
    var inscope adapter <- make_data_walker(walker)

    var t = ("hello", 42, 3.14)
    print("Walking tuple<string;int;float>:\n")
    unsafe {
        adapter |> walk_data(addr(t), typeinfo rtti_typeinfo(t))
    }

    var ok_result = Result(ok = 42)
    print("\nWalking variant (ok=42):\n")
    unsafe {
        adapter |> walk_data(addr(ok_result), typeinfo rtti_typeinfo(ok_result))
    }

    var err_result = Result(err = "not found")
    print("\nWalking variant (err=\"not found\"):\n")
    unsafe {
        adapter |> walk_data(addr(err_result), typeinfo rtti_typeinfo(err_result))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 5: Enumerations and bitfields
// ============================================================
//
// Enumerations trigger WalkEnumeration with an EnumInfo that
// contains value names.  Bitfields trigger the Bitfield callback
// with a TypeInfo that contains field names.

enum Color {
    Red
    Green
    Blue
}

bitfield Permissions {
    readable
    writable
    executable
}

class EnumBitfieldPrinter : DapiDataWalker {
    def override WalkEnumeration(var value : int&; ei : EnumInfo) : void {
        // EnumInfo contains the enum name and value names
        for (i in range(ei.count)) {
            unsafe {
                if (int(ei.fields[i].value) == value) {
                    print("  enum {ei.name}.{ei.fields[i].name} ({value})\n")
                    return
                }
            }
        }
        print("  enum {ei.name} = {value}\n")
    }

    def override Bitfield(var value : uint&; ti : TypeInfo) : void {
        // TypeInfo.argNames contains the bit field names
        print("  bitfield = {value} [")
        var first = true
        for (i in range(ti.argCount)) {
            if ((value & (1u << uint(i))) != 0u) {
                if (!first) {
                    print(", ")
                }
                first = false
                unsafe {
                    print("{ti.argNames[i]}")
                }
            }
        }
        print("]\n")
    }
}

def demo_enums_and_bitfields() {
    print("\n=== Section 5: Enumerations and bitfields ===\n")

    var walker = new EnumBitfieldPrinter()
    var inscope adapter <- make_data_walker(walker)

    var color = Color.Green
    print("Walking enum Color.Green:\n")
    unsafe {
        adapter |> walk_data(addr(color), typeinfo rtti_typeinfo(color))
    }

    var perms : Permissions = Permissions.readable | Permissions.executable
    print("Walking bitfield (readable | executable):\n")
    unsafe {
        adapter |> walk_data(addr(perms), typeinfo rtti_typeinfo(perms))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 6: JSON serializer — putting it all together
// ============================================================
//
// A practical DapiDataWalker that serializes any daScript value
// to a JSON string.  This demonstrates combining all of the
// structure, array, table, tuple, and scalar callbacks into one
// coherent walker.

struct Item {
    name : string
    weight : float
}

struct Inventory {
    owner : string
    gold : int
    items : array<Item>
    flags : table<string; bool>
}

class JsonWalker : DapiDataWalker {
    // The walker writes directly to a StringBuilderWriter for efficiency.
    // The writer is set externally by to_json() via build_string().
    @do_not_delete writer : StringBuilderWriter?
    indent : int = 0
    needComma : array<bool>

    def comma() {
        if (length(needComma) > 0 && needComma[length(needComma) - 1]) {
            *writer |> write(",")
        }
    }

    def nl() {
        *writer |> write("\n")
        for (_ in range(indent)) {
            *writer |> write("  ")
        }
    }

    def pushComma() {
        needComma |> push(false)
    }

    def popComma() {
        needComma |> pop()
    }

    def markComma() {
        if (length(needComma) > 0) {
            needComma[length(needComma) - 1] = true
        }
    }

    // --- structures ---
    def override beforeStructure(ps : void?; si : StructInfo) : void {
        *writer |> write("\{")
        indent++
        pushComma()
    }

    def override afterStructure(ps : void?; si : StructInfo) : void {
        popComma()
        indent--
        nl()
        *writer |> write("\}")
        markComma()
    }

    def override beforeStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        comma()
        nl()
        *writer |> write("\"")
        *writer |> write(vi.name)
        *writer |> write("\": ")
    }

    def override afterStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        markComma()
    }

    // --- arrays ---
    def override beforeArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        *writer |> write("[")
        indent++
        pushComma()
    }

    def override afterArrayData(ps : void?; stride : uint; count : uint; ti : TypeInfo) : void {
        popComma()
        indent--
        nl()
        *writer |> write("]")
        markComma()
    }

    def override beforeArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        comma()
        nl()
    }

    def override afterArrayElement(ps : void?; ti : TypeInfo; pe : void?; index : uint; last : bool) : void {
        markComma()
    }

    // --- tables ---
    def override beforeTable(pa : DapiTable; ti : TypeInfo) : void {
        *writer |> write("\{")
        indent++
        pushComma()
    }

    def override afterTable(pa : DapiTable; ti : TypeInfo) : void {
        popComma()
        indent--
        nl()
        *writer |> write("\}")
        markComma()
    }

    def override beforeTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        comma()
        nl()
    }

    def override afterTableKey(pa : DapiTable; ti : TypeInfo; pk : void?; ki : TypeInfo; index : uint; last : bool) : void {
        *writer |> write(": ")
    }

    def override afterTableValue(pa : DapiTable; ti : TypeInfo; pv : void?; kv : TypeInfo; index : uint; last : bool) : void {
        markComma()
    }

    // --- tuples ---
    def override beforeTuple(ps : void?; ti : TypeInfo) : void {
        *writer |> write("[")
        indent++
        pushComma()
    }

    def override afterTuple(ps : void?; ti : TypeInfo) : void {
        popComma()
        indent--
        nl()
        *writer |> write("]")
        markComma()
    }

    def override beforeTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        comma()
        nl()
    }

    def override afterTupleEntry(ps : void?; ti : TypeInfo; pv : void?; idx : int; last : bool) : void {
        markComma()
    }

    // --- scalars ---
    def override Int(var value : int&) : void {
        *writer |> write(value)
    }

    def override Float(var value : float&) : void {
        *writer |> write(value)
    }

    def override String(var value : string&) : void {
        *writer |> write("\"")
        *writer |> write(value)
        *writer |> write("\"")
    }

    def override Bool(var value : bool&) : void {
        *writer |> write(value ? "true" : "false")
    }

    def override Null(ti : TypeInfo) : void {
        *writer |> write("null")
    }
}

def to_json(var value; tinfo : TypeInfo) : string {
    var walker = new JsonWalker()
    var inscope adapter <- make_data_walker(walker)
    let res = build_string() $(var writer) {
        unsafe {
            walker.writer = addr(writer)
            adapter |> walk_data(addr(value), tinfo)
            walker.writer = null
        }
    }
    unsafe {
        delete walker
    }
    return res
}

def demo_json_serializer() {
    print("\n=== Section 6: JSON serializer ===\n")

    var inv = Inventory(
        owner = "Alice",
        gold = 250,
        items <- [Item(name = "Sword", weight = 3.5), Item(name = "Shield", weight = 5.2), Item(name = "Potion", weight = 0.5)],
        flags <- { "is_merchant" => true, "is_hostile" => false }
    )

    let json = to_json(inv, typeinfo rtti_typeinfo(inv))
    print("{json}\n")
}


// ============================================================
// Section 7: Filtering with canVisit
// ============================================================
//
// All canVisit methods return true by default.  Override them to
// return false and the walker skips that subtree entirely.

struct Secret {
    classified : string
    code : int
}

struct PublicRecord {
    title : string
    secret : Secret
    score : int
}

class FilteringWalker : DapiDataWalker {
    indent : int = 0
    skipStructName : string

    def pad() {
        for (_ in range(indent)) {
            print("  ")
        }
    }

    def override canVisitStructure(ps : void?; si : StructInfo) : bool {
        // Skip structs matching the filter name
        if (si.name == skipStructName) {
            print("<skipped {si.name}>\n")
            return false
        }
        return true
    }

    def override beforeStructure(ps : void?; si : StructInfo) : void {
        print("{si.name} \{\n")
        indent++
    }

    def override afterStructure(ps : void?; si : StructInfo) : void {
        indent--
        self->pad()
        print("\}\n")
    }

    def override beforeStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        self->pad()
        print("{vi.name} = ")
    }

    def override afterStructureField(ps : void?; si : StructInfo; pv : void?; vi : VarInfo; last : bool) : void {
        pass
    }

    def override Int(var value : int&) : void {
        print("{value}\n")
    }

    def override String(var value : string&) : void {
        print("\"{value}\"\n")
    }
}

def demo_filtering() {
    print("\n=== Section 7: Filtering with canVisit ===\n")

    var walker = new FilteringWalker()
    walker.skipStructName = "Secret"
    var inscope adapter <- make_data_walker(walker)

    var record = PublicRecord(
        title = "Performance Review",
        secret = Secret(classified = "top-secret", code = 42),
        score = 95
    )

    print("Walking PublicRecord (Secret fields skipped):\n")
    unsafe {
        adapter |> walk_data(addr(record), typeinfo rtti_typeinfo(record))
    }

    unsafe {
        delete walker
    }
}


// ============================================================
// Section 8: Mutating data in-place
// ============================================================
//
// Scalar callbacks receive `var value : T&` — a mutable reference.
// This means the walker can modify data during traversal.  Here we
// build a "clamper" that restricts all float values to [0..1].

struct Particle {
    x : float
    y : float
    z : float
    alpha : float
}

class FloatClamper : DapiDataWalker {
    lo : float = 0.0
    hi : float = 1.0

    def override Float(var value : float&) : void {
        if (value < lo) {
            value = lo
        }
        if (value > hi) {
            value = hi
        }
    }
}

def demo_mutation() {
    print("\n=== Section 8: Mutating data in-place ===\n")

    var particle = Particle(x = -0.5, y = 0.3, z = 1.7, alpha = 0.8)
    print("Before clamping: x={particle.x} y={particle.y} z={particle.z} alpha={particle.alpha}\n")

    var walker = new FloatClamper()
    var inscope adapter <- make_data_walker(walker)

    unsafe {
        adapter |> walk_data(addr(particle), typeinfo rtti_typeinfo(particle))
    }

    print("After clamping:  x={particle.x} y={particle.y} z={particle.z} alpha={particle.alpha}\n")

    unsafe {
        delete walker
    }
}


// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_scalar_types()
    demo_structures()
    demo_arrays_and_tables()
    demo_tuples_and_variants()
    demo_enums_and_bitfields()
    demo_json_serializer()
    demo_filtering()
    demo_mutation()
}
