"""Generate improved module documentation with compilable examples.

Writes updated module-*.rst files in handmade/ with:
- Improved descriptions
- Compilable code examples
- Expected output comments
"""
import os

HANDMADE = r"d:\Work\daslang\doc\source\stdlib\handmade"
EXAMPLES_DIR = r"d:\Work\daslang\doc\reflections\examples"

# Module documentation with examples
# Format: module_name -> (description_text, require_line, example_code_or_None)
# description_text replaces everything before the require block
# example_code is appended after the require block

MODULES = {}

def reg(name, desc, require, example=None, after_require=""):
    """Register a module doc."""
    MODULES[name] = (desc, require, example, after_require)


# ============ CORE LANGUAGE MODULES ============

reg("builtin",
    """The BUILTIN module contains core runtime functions available in all daslang programs
without explicit ``require``. It includes:

- Heap and memory management (``heap_bytes_allocated``, ``heap_report``, ``memory_report``)
- Debug output (``print``, ``debug``, ``stackwalk``)
- Panic and error handling (``panic``, ``terminate``, ``assert``)
- Pointer and memory operations (``intptr``, ``malloc``, ``free``)
- Profiling (``profile``)
- Type conversion (``string``)""",
    "require builtin",
    example="""\
    [export]
    def main() {
        print("hello, world!\\n")
        assert(1 + 1 == 2)
        let s = string(42)
        print("string(42) = {s}\\n")
        let name = "daslang"
        print("welcome to {name}\\n")
        var arr : array<int>
        arr |> push(10)
        arr |> push(20)
        print("length = {length(arr)}\\n")
        print("arr[0] = {arr[0]}\\n")
    }
    // output:
    // hello, world!
    // string(42) = 42
    // welcome to daslang
    // length = 2
    // arr[0] = 10
""")


reg("math",
    """The MATH module contains floating point math functions and constants
(trigonometry, exponentials, clamping, interpolation, noise, and vector/matrix operations).
Floating point math in general is not bit-precise: the compiler may optimize
permutations, replace divisions with multiplications, and some functions are
not bit-exact. Use ``double`` precision types when exact results are required.""",
    "require math",
    example="""\
    require math

    [export]
    def main() {
        print("sin(PI/2) = {sin(PI / 2.0)}\\n")
        print("cos(0)    = {cos(0.0)}\\n")
        print("sqrt(16)  = {sqrt(16.0)}\\n")
        print("abs(-5)   = {abs(-5)}\\n")
        print("clamp(15, 0, 10) = {clamp(15, 0, 10)}\\n")
        print("min(3, 7) = {min(3, 7)}\\n")
        print("max(3, 7) = {max(3, 7)}\\n")
        let v = float3(1, 0, 0)
        print("length = {length(v)}\\n")
    }
    // output:
    // sin(PI/2) = 1
    // cos(0)    = 1
    // sqrt(16)  = 4
    // abs(-5)   = 5
    // clamp(15, 0, 10) = 10
    // min(3, 7) = 3
    // max(3, 7) = 7
    // length = 1
""")


reg("strings",
    """The STRINGS module implements string formatting, conversion, searching, and modification
routines. It provides functions for building strings (``build_string``), parsing
(``to_int``, ``to_float``), character classification (``is_alpha``, ``is_number``),
and low-level string manipulation.""",
    "require strings",
)

reg("rtti",
    """The RTTI module exposes runtime type information and program introspection facilities.
It allows querying module structure, type declarations, function signatures, annotations,
and other compile-time metadata at runtime. Used primarily by macro libraries and
code generation tools.""",
    "require daslib/rtti",
)

reg("ast_core",
    """The AST module provides access to the abstract syntax tree representation of daslang programs.
It defines node types for all language constructs (expressions, statements, types, functions,
structures, enumerations, etc.), visitors for tree traversal, and utilities for AST
construction and manipulation. This module is the foundation for writing macros, code
generators, and source-level program transformations.""",
    "require daslib/ast",
)

reg("fio",
    """The FIO module implements file input/output and filesystem operations.
It provides functions for reading and writing files (``fopen``, ``fread``, ``fwrite``),
directory management (``mkdir``, ``dir``), path manipulation (``join_path``,
``basename``, ``dirname``), and file metadata queries (``stat``, ``file_time``).""",
    "require daslib/fio",
    example="""\
    require daslib/fio

    [export]
    def main() {
        let fname = "_test_fio_tmp.txt"
        fopen(fname, "wb") <| $(f) {
            fwrite(f, "hello, daslang!")
        }
        fopen(fname, "rb") <| $(f) {
            let content = fread(f)
            print("{content}\\n")
        }
        remove(fname)
    }
    // output:
    // hello, daslang!
""")


reg("jobque",
    """The JOBQUE module provides low-level job queue and threading primitives.
It includes thread-safe channels for inter-thread communication, lock boxes
for shared data access, job status tracking, and fine-grained thread
management. For higher-level job abstractions, see ``jobque_boost``.""",
    "require jobque",
    example="""\
    require jobque

    [export]
    def main() {
        with_atomic32 <| $(counter) {
            counter |> set(10)
            print("value = {counter |> get}\\n")
            let after_inc = counter |> inc
            print("after inc = {after_inc}\\n")
            let after_dec = counter |> dec
            print("after dec = {after_dec}\\n")
        }
    }
    // output:
    // value = 10
    // after inc = 11
    // after dec = 10
""")


reg("network",
    """The NETWORK module implements networking facilities including HTTP client/server
and low-level socket operations. It provides ``Server`` and ``Client`` classes
with event-driven callbacks for handling connections, requests, and responses.""",
    "require daslib/network",
)

reg("uriparser",
    """The URIPARSER module provides URI parsing and manipulation based on the uriparser library.
It supports parsing URI strings into components (scheme, host, path, query, fragment),
normalization, resolution of relative URIs, and GUID generation.""",
    "require uriparser",
)

# ============ DASLIB MODULES ============

reg("ansi_colors",
    """The ANSI_COLORS module provides ANSI terminal color and style helpers.
Color output is controlled by the ``use_tty_colors`` global variable.
Call ``init_ansi_colors`` to auto-detect color support from command-line flags
and environment variables, or set ``use_tty_colors`` directly.""",
    "require daslib/ansi_colors",
    example="""\
    require daslib/ansi_colors

    [export]
    def main() {
        use_tty_colors = true
        print("{red_str("error")}: something went wrong\\n")
        print("{green_str("ok")}: all good\\n")
        print("{bold_str("important")}: pay attention\\n")
        use_tty_colors = false
        print("{red_str("no color")}: plain text\\n")
    }
""")

reg("algorithm",
    """The ALGORITHM module provides array and collection manipulation algorithms including
sorting, searching, set operations, and element removal.""",
    "require daslib/algorithm",
    example="""\
    require daslib/algorithm

    [export]
    def main() {
        var arr <- [3, 1, 4, 1, 5, 9, 2, 6, 5]
        sort_unique(arr)
        print("sort_unique: {arr}\\n")
        print("has 4: {binary_search(arr, 4)}\\n")
        print("has 7: {binary_search(arr, 7)}\\n")
        print("lower_bound(4): {lower_bound(arr, 4)}\\n")
        reverse(arr)
        print("reversed: {arr}\\n")
    }
    // output:
    // sort_unique: [[ 1; 2; 3; 4; 5; 6; 9]]
    // has 4: true
    // has 7: false
    // lower_bound(4): 3
    // reversed: [[ 9; 6; 5; 4; 3; 2; 1]]
""")

reg("strings_boost",
    """The STRINGS_BOOST module extends string handling with splitting, joining,
padding, character replacement, and edit distance computation.""",
    "require daslib/strings_boost",
    example="""\
    require daslib/strings_boost

    [export]
    def main() {
        let parts = split("one,two,three", ",")
        print("split: {parts}\\n")
        print("join: {join(parts, " | ")}\\n")
        print("[{wide("hello", 10)}]\\n")
        print("distance: {levenshtein_distance("kitten", "sitting")}\\n")
    }
    // output:
    // split: [[ one; two; three]]
    // join: one | two | three
    // [hello     ]
    // distance: 3
""")

reg("functional",
    """The FUNCTIONAL module implements lazy iterator adapters and higher-order
function utilities including ``filter``, ``map``, ``reduce``, ``fold``,
``scan``, ``flatten``, ``flat_map``, ``enumerate``, ``chain``, ``pairwise``,
``iterate``, ``islice``, ``cycle``, ``repeat``, ``sorted``, ``sum``,
``any``, ``all``, ``tap``, ``for_each``, ``find``, ``find_index``, and
``partition``.""",
    "require daslib/functional",
    example="""\
    require daslib/functional

    [export]
    def main() {
        var src <- [iterator for (x in range(6)); x]
        var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })
        for (v in evens) {
            print("{v} ")
        }
        print("\\n")
    }
    // output:
    // 0 2 4
""")

reg("json",
    """The JSON module implements JSON parsing and serialization.
It provides ``read_json`` for parsing JSON text into a ``JsonValue`` tree,
``write_json`` for serializing back to text, and ``JV`` helpers for constructing
JSON values from daslang types.

See also :doc:`json_boost` for automatic struct-to-JSON conversion and the ``%json~`` reader macro.""",
    "require daslib/json",
    example="""\
    require daslib/json

    [export]
    def main() {
        let data = "[1, 2, 3]"
        var error = ""
        var js <- read_json(data, error)
        print("json: {write_json(js)}\\n")
        unsafe {
            delete js
        }
    }
    // output:
    // json: [1,2,3]
""")

reg("json_boost",
    """The JSON_BOOST module extends JSON support with operator overloads for convenient
field access (``?[]``), null-coalescing (``??``), and automatic struct-to-JSON
conversion macros (``from_JsValue``, ``to_JsValue``).

See also :doc:`json` for core JSON parsing and writing.""",
    "require daslib/json_boost",
    example="""\
    require daslib/json_boost

    [export]
    def main() {
        let data = "\\{ \\"name\\": \\"Alice\\", \\"age\\": 30 \\}"
        var error = ""
        var js <- read_json(data, error)
        if (error == "") {
            let name = js?.name ?? "?"
            print("name = {name}\\n")
            let age = js?.age ?? -1
            print("age = {age}\\n")
        }
        unsafe {
            delete js
        }
    }
    // output:
    // name = Alice
    // age = 30
""")

reg("random",
    """The RANDOM module implements pseudo-random number generation using a linear
congruential generator with vectorized state (``int4``). It provides integer,
float, and vector random values, as well as geometric sampling (unit vectors,
points in spheres and disks).""",
    "require daslib/random",
    example="""\
    require daslib/random

    [export]
    def main() {
        var seed = random_seed(12345)
        print("int: {random_int(seed)}\\n")
        print("float: {random_float(seed)}\\n")
        print("float: {random_float(seed)}\\n")
    }
    // output:
    // int: 7584
    // float: 0.5848567
    // float: 0.78722495
""")

reg("base64",
    """The BASE64 module implements Base64 encoding and decoding.
It provides ``base64_encode`` and ``base64_decode`` for converting between binary data
(strings or ``array<uint8>``) and Base64 text representation.""",
    "require daslib/base64",
    example="""\
    require daslib/base64

    [export]
    def main() {
        let encoded = base64_encode("Hello, daslang!")
        print("encoded: {encoded}\\n")
        let decoded = base64_decode(encoded)
        print("decoded: {decoded.text}\\n")
    }
    // output:
    // encoded: SGVsbG8sIGRhU2NyaXB0IQ==
    // decoded: Hello, daslang!
""")

reg("defer",
    """The DEFER module implements the ``defer`` pattern — the ability to schedule cleanup
code to run at scope exit, similar to Go's ``defer``. The deferred block is moved
to the ``finally`` section of the enclosing scope at compile time.""",
    "require daslib/defer",
    example="""\
    require daslib/defer

    [export]
    def main() {
        print("start\\n")
        defer() <| $() {
            print("cleanup runs last\\n")
        }
        print("middle\\n")
    }
    // output:
    // start
    // middle
    // cleanup runs last
""")

reg("enum_trait",
    """The ENUM_TRAIT module provides reflection utilities for enumerations: iterating
over all values, converting between enum values and strings, and building
lookup tables. The ``[string_to_enum]`` annotation generates a string constructor
for the annotated enum type.""",
    "require daslib/enum_trait",
    example="""\
    require daslib/enum_trait

    enum Color {
        red
        green
        blue
    }

    [export]
    def main() {
        print("{Color.green}\\n")
        let c = to_enum(type<Color>, "blue")
        print("{c}\\n")
        let bad = to_enum(type<Color>, "purple", Color.red)
        print("fallback = {bad}\\n")
    }
    // output:
    // green
    // blue
    // fallback = red
""")

reg("safe_addr",
    """The SAFE_ADDR module provides compile-time checked pointer operations.
``safe_addr`` returns a temporary pointer to a variable only if the compiler
can verify the pointer will not outlive its target. This prevents dangling
pointer bugs without runtime overhead.""",
    "require daslib/safe_addr",
)

reg("array_boost",
    """The ARRAY_BOOST module extends array operations with temporary array views
over fixed-size arrays and C++ handled vectors, emptiness checks, sub-array
views, and arithmetic operators on fixed-size arrays.""",
    "require daslib/array_boost",
)

reg("match",
    """The MATCH module implements pattern matching on variants, structs, tuples,
arrays, and scalar values. Supports variable capture, wildcards, guard
expressions, and alternation. ``static_match`` enforces exhaustive matching
at compile time.""",
    "require daslib/match",
    example="""\
    require daslib/match

    enum Color {
        red
        green
        blue
    }

    def describe(c : Color) : string {
        match (c) {
            if (Color.red) { return "red"; }
            if (Color.green) { return "green"; }
            if (_) { return "other"; }
        }
        return "?"
    }

    [export]
    def main() {
        print("{describe(Color.red)}\\n")
        print("{describe(Color.green)}\\n")
        print("{describe(Color.blue)}\\n")
    }
    // output:
    // red
    // green
    // other
""")

reg("linq",
    """The LINQ module provides query-style operations on sequences: filtering
(``where_``), projection (``select``), sorting (``order``, ``order_by``),
deduplication (``distinct``), pagination (``skip``, ``take``), aggregation
(``sum``, ``average``, ``aggregate``), and element access (``first``, ``last``).

See also :doc:`linq_boost` for pipe-syntax macros with underscore shorthand.""",
    "require daslib/linq",
    example="""\
    require daslib/linq

    [export]
    def main() {
        var src <- [iterator for (x in range(10)); x]
        var evens <- where_(src, $(x : int) : bool { return x % 2 == 0; })
        for (v in evens) {
            print("{v} ")
        }
        print("\\n")
    }
    // output:
    // 0 2 4 6 8
""")

reg("linq_boost",
    """The LINQ_BOOST module extends LINQ with pipe-friendly macros using underscore
syntax for inline predicates and selectors. Expressions like
``arr |> _where(_ > 3) |> _select(_ * 2)`` provide concise functional pipelines.

See also :doc:`linq` for the full set of query operations.""",
    "require daslib/linq_boost",
    example="""\
    require daslib/linq
    require daslib/linq_boost

    [export]
    def main() {
        var src <- [iterator for (x in range(10)); x]
        var evens <- _where(src, _ % 2 == 0)
        for (v in evens) {
            print("{v} ")
        }
        print("\\n")
    }
    // output:
    // 0 2 4 6 8
""")

reg("math_boost",
    """The MATH_BOOST module adds geometric types (``AABB``, ``AABR``, ``Ray``),
angle conversion (``degrees``, ``radians``), intersection tests, color space
conversion (``linear_to_SRGB``, ``RGBA_TO_UCOLOR``), and view/projection
matrix construction (``look_at_lh``, ``perspective_rh``).""",
    "require daslib/math_boost",
    example="""\
    require daslib/math_boost

    [export]
    def main() {
        print("degrees(PI) = {degrees(PI)}\\n")
        print("radians(180) = {radians(180.0)}\\n")
        var box = AABB(min = float3(0), max = float3(10))
        print("box = ({box.min}) - ({box.max})\\n")
    }
    // output:
    // degrees(PI) = 180
    // radians(180) = 3.1415927
    // box = (0,0,0) - (10,10,10)
""")

reg("contracts",
    """The CONTRACTS module provides compile-time type constraints for generic function
arguments. Annotations like ``[expect_any_array]``, ``[expect_any_enum]``,
``[expect_any_numeric]``, and ``[expect_any_struct]`` restrict which types
can instantiate a generic parameter, producing clear error messages on mismatch.""",
    "require daslib/contracts",
    example="""\
    require daslib/contracts

    [!expect_dim(a)]
    def process(a) {
        return "scalar"
    }

    [expect_dim(a)]
    def process(a) {
        return "array"
    }

    [export]
    def main() {
        var arr : int[3]
        print("{process(42)}\\n")
        print("{process(arr)}\\n")
    }
    // output:
    // scalar
    // array
""")

reg("static_let",
    """The STATIC_LET module implements the ``static_let`` pattern — local variables
that persist across function calls, similar to C ``static`` variables. The
variable is initialized once on first call and retains its value in subsequent
invocations.""",
    "require daslib/static_let",
    example="""\
    require daslib/static_let

    def counter() : int {
        static_let <| $() {
            var count = 0
        }
        count ++
        return count
    }

    [export]
    def main() {
        print("{counter()}\\n")
        print("{counter()}\\n")
        print("{counter()}\\n")
    }
    // output:
    // 1
    // 2
    // 3
""")

reg("sort_boost",
    """The SORT_BOOST module provides the ``qsort`` macro that uniformly sorts
built-in arrays, dynamic arrays, and C++ handled vectors using the same
syntax. It automatically wraps handled types in ``temp_array`` as needed.""",
    "require daslib/sort_boost",
)

reg("unroll",
    """The UNROLL module implements compile-time loop unrolling. The ``unroll``
macro replaces a ``for`` loop with a constant ``range`` bound by stamping
out each iteration as separate inlined code, eliminating loop overhead.""",
    "require daslib/unroll",
    example="""\
    require daslib/unroll

    [export]
    def main() {
        unroll <| $() {
            for (i in range(4)) {
                print("step {i}\\n")
            }
        }
    }
    // output:
    // step 0
    // step 1
    // step 2
    // step 3
""")

reg("lpipe",
    """The LPIPE module provides the ``lpipe`` macro for passing multiple block
arguments to a single function call. While ``<|`` handles the first block
argument, ``lpipe`` adds subsequent blocks on following lines.""",
    "require daslib/lpipe",
    example="""\
    require daslib/lpipe

    def take2(a, b : block) {
        invoke(a)
        invoke(b)
    }

    [export]
    def main() {
        take2() <| $() {
            print("first\\n")
        }
        lpipe <| $() {
            print("second\\n")
        }
    }
    // output:
    // first
    // second
""")

reg("if_not_null",
    """The IF_NOT_NULL module provides a null-safe call macro. The expression
``ptr |> if_not_null <| call(args)`` expands to a null check followed by
a dereferenced call: ``if (ptr != null) { call(*ptr, args) }``.""",
    "require daslib/if_not_null",
)

reg("stringify",
    """The STRINGIFY module provides the ``%stringify~`` reader macro for embedding
multi-line string literals verbatim. Text between ``%stringify~`` and ``%%``
is captured as-is without requiring escape sequences for quotes, braces,
or other special characters.""",
    "require daslib/stringify",
)

reg("coroutines",
    """The COROUTINES module provides coroutine infrastructure including the
``[coroutine]`` function annotation, ``yield_from`` for delegating to
sub-coroutines, and ``co_await`` for composing asynchronous generators.
Coroutines produce values lazily via ``yield`` and can be iterated with
``for``.""",
    "require daslib/coroutines",
    example="""\
    require daslib/coroutines

    [coroutine]
    def fibonacci() : int {
        var a = 0
        var b = 1
        while (true) {
            yield a
            let next = a + b
            a = b
            b = next
        }
    }

    [export]
    def main() {
        var count = 0
        for (n in fibonacci()) {
            print("{n} ")
            count ++
            if (count >= 10) {
                break
            }
        }
        print("\\n")
    }
    // output:
    // 0 1 1 2 3 5 8 13 21 34
""")

reg("archive",
    """The ARCHIVE module implements general-purpose serialization infrastructure.
It provides the ``Archive`` type and ``serialize`` functions for reading and
writing binary data. Custom types are supported by implementing ``serialize``
for each type.""",
    "require daslib/archive",
    example="""\
    require daslib/archive

    struct Foo {
        a : float
        b : string
    }

    [export]
    def main() {
        var original = Foo(a = 3.14, b = "hello")
        var data <- mem_archive_save(original)
        var loaded : Foo
        data |> mem_archive_load(loaded)
        delete data
        print("a = {loaded.a}, b = {loaded.b}\\n")
    }
    // output:
    // a = 3.14, b = hello
""",
    after_require="""\
To correctly support serialization of the specific type, you need to define and
implement ``serialize`` method for it.
For example this is how DECS implements component serialization: ::

    def public serialize ( var arch:Archive; var src:Component )
        arch |> serialize(src.name)
        arch |> serialize(src.hash)
        arch |> serialize(src.stride)
        arch |> serialize(src.info)
        invoke(src.info.serializer, arch, src.data)
""")

reg("apply",
    """The APPLY module provides the ``apply`` macro for iterating over struct, tuple,
and variant fields at compile time. Each field is visited with its name and
a reference to its value, enabling generic per-field operations like
serialization, printing, and validation.""",
    "require daslib/apply",
    example="""\
    require daslib/apply

    struct Foo {
        a : int
        b : float
        c : string
    }

    [export]
    def main() {
        var foo = Foo(a = 42, b = 3.14, c = "hello")
        apply(foo) <| $(name, field) {
            print("{name} = {field}\\n")
        }
    }
    // output:
    // a = 42
    // b = 3.14
    // c = hello
""")

reg("apply_in_context",
    """The APPLY_IN_CONTEXT module extends apply operations to work across
different execution contexts, enabling cross-context function invocation
with packed arguments.""",
    "require daslib/apply_in_context",
)

reg("ast_block_to_loop",
    """The AST_BLOCK_TO_LOOP module provides an AST transformation macro that
converts block-based iteration patterns into explicit loop constructs.
Used internally by other macro libraries for optimization.""",
    "require daslib/ast_block_to_loop",
)

reg("ast_boost",
    """The AST_BOOST module provides high-level utilities for working with the AST.
It includes helpers for creating expressions, types, and declarations,
quote-based AST construction, and common AST query and transformation
patterns used by macro authors.""",
    "require daslib/ast_boost",
)

reg("ast_used",
    """The AST_USED module implements analysis passes that determine which AST nodes
are actually used in the program. This information is used for dead code
elimination, tree shaking, and optimizing generated output.""",
    "require daslib/ast_used",
)

reg("ast_match",
    """The AST_MATCH module provides AST pattern matching via reverse reification.
It matches expressions and blocks against structural patterns using the same
tag system as qmacro but in reverse: $e clones a matched expression, $v extracts
a constant value, $i extracts an identifier name, $c extracts a call name,
$f extracts a field name, $b captures a statement range, $t captures a type,
and $a captures remaining arguments.""",
    "require daslib/ast_match",
)

reg("async_boost",
    """The ASYNC_BOOST module implements an async/await pattern for daslang using
channels and coroutines. It provides ``async`` for launching concurrent tasks
and ``await`` for waiting on their results, built on top of the job queue
infrastructure.""",
    "require daslib/async_boost",
)

reg("bitfield_boost",
    """The BITFIELD_BOOST module provides utility macros for working with bitfield types
including conversion between bitfield values and strings, and iteration over
set bits.""",
    "require daslib/bitfield_boost",
)

reg("bitfield_trait",
    """The BITFIELD_TRAIT module implements reflection utilities for bitfield types:
converting bitfield values to and from human-readable strings, iterating
over individual set bits, and constructing bitfield values from string names.""",
    "require daslib/bitfield_trait",
)

reg("bool_array",
    """The BOOL_ARRAY module provides a compact boolean array implementation using
bit-packing. Each boolean value uses a single bit instead of a byte,
providing an 8x memory reduction compared to ``array<bool>``.""",
    "require daslib/bool_array",
)

reg("class_boost",
    """The CLASS_BOOST module provides macros for extending class functionality,
including the ``[serialize_as_class]`` annotation for automatic serialization
and common class patterns like abstract method enforcement.""",
    "require daslib/class_boost",
)

reg("constant_expression",
    """The CONSTANT_EXPRESSION module provides the ``[constant_expression]`` function
annotation. Functions marked with this annotation are evaluated at compile
time when all arguments are constants, replacing the call with the computed
result.""",
    "require daslib/constant_expression",
)

reg("consume",
    """The CONSUME module implements the ``consume`` pattern, which moves ownership
of containers and other moveable values while leaving the source in a
default-constructed state. This enables efficient ownership transfer.""",
    "require daslib/consume",
)

reg("cpp_bind",
    """The CPP_BIND module provides utilities for generating daslang bindings
to C++ code. It helps generate module registration code, type annotations,
and function wrappers for exposing C++ APIs to daslang programs.""",
    "require daslib/cpp_bind",
)

reg("cuckoo_hash_table",
    """The CUCKOO_HASH_TABLE module implements a cuckoo hash table data structure.
Cuckoo hashing provides worst-case O(1) lookup time by using multiple hash
functions and displacing existing entries on collision.""",
    "require daslib/cuckoo_hash_table",
)

reg("dap",
    """The DAP module implements the Debug Adapter Protocol (DAP) for integrating
daslang with external debuggers. It provides the message types, serialization,
and communication infrastructure needed for IDE debugging support.""",
    "require daslib/dap",
)

reg("das_source_formatter",
    """The DAS_SOURCE_FORMATTER module implements source code formatting for daslang.
It can parse and re-emit daslang source code with consistent indentation,
spacing, and line breaking rules. Used by editor integrations and code
quality tools.""",
    "require daslib/das_source_formatter",
)

reg("das_source_formatter_fio",
    """The DAS_SOURCE_FORMATTER_FIO module extends the source formatter with file I/O
capabilities, enabling formatting of daslang source files on disk.
It reads, formats, and writes back source files in place or to new locations.""",
    "require daslib/das_source_formatter_fio",
)

reg("debug_eval",
    """The DEBUG_EVAL module provides runtime expression evaluation for debugging
purposes. It can evaluate daslang expressions in the context of a running
program, supporting variable inspection and interactive debugging.""",
    "require daslib/debug_eval",
)

reg("decs",
    """The DECS module implements a Data-oriented Entity Component System.
Entities are identified by integer IDs and store components as typed data.
Systems query and process entities by their component signatures,
enabling cache-friendly batch processing of game objects.""",
    "require daslib/decs",
)

reg("decs_boost",
    """The DECS_BOOST module provides convenience macros and syntactic sugar for
the DECS entity component system, including simplified component registration,
entity creation, and system definition patterns.

See also :doc:`decs` for the core ECS runtime and :doc:`decs_state` for entity state machines.""",
    "require daslib/decs_boost",
    example="""\
    options persistent_heap = true
    require daslib/decs_boost

    [export]
    def main() {
        restart()
        create_entity <| @(eid, cmp) {
            cmp |> set("pos", float3(1, 2, 3))
            cmp |> set("name", "hero")
        }
        commit()
        query <| $(pos : float3; name : string) {
            print("{name} at {pos}\\n")
        }
    }
    // output:
    // hero at 1,2,3
""")

reg("decs_state",
    """The DECS_STATE module extends DECS with state machine support for entities.
It provides state transition management, allowing entities to change behavior
based on their current state.

See also :doc:`decs` for the core ECS runtime and :doc:`decs_boost` for query macros.""",
    "require daslib/decs_state",
)

reg("dynamic_cast_rtti",
    """The DYNAMIC_CAST_RTTI module implements runtime dynamic casting between class
types using RTTI information. It provides safe downcasting with null results
on type mismatch, similar to C++ ``dynamic_cast``.""",
    "require daslib/dynamic_cast_rtti",
)

reg("faker",
    """Random test-data generator.

The ``Faker`` struct produces random values for every built-in type
(integers, floats, vectors, strings, dates, booleans) using
configurable ranges. Used by ``fuzzer`` for fuzz testing.""",
    "require daslib/faker",
)

reg("flat_hash_table",
    """The FLAT_HASH_TABLE module implements a flat (open addressing) hash table.
It stores all entries in a single contiguous array, providing cache-friendly
access patterns and good performance for small to medium-sized tables.""",
    "require daslib/flat_hash_table",
    example="""\
    require daslib/flat_hash_table public

    typedef IntMap = TFlatHashTable<int; string>

    [export]
    def main() {
        var m <- IntMap()
        m[1] = "one"
        m[2] = "two"
        m[3] = "three"
        print("length = {m.data_length}\\n")
        print("m[2] = {m[2]}\\n")
        m.clear()
        print("after clear: {m.data_length}\\n")
    }
    // output:
    // length = 3
    // m[2] = two
    // after clear: 0
""")

reg("fuzzer",
    """The FUZZER module implements fuzz testing infrastructure for daslang programs.
It generates random inputs for functions and verifies they do not crash or
produce unexpected errors, helping discover edge cases and robustness issues.""",
    "require daslib/fuzzer",
)

reg("generic_return",
    """The GENERIC_RETURN module provides the ``[generic_return]`` annotation that
allows generic functions to automatically deduce their return type from
the body. This simplifies writing generic utility functions by eliminating
explicit return type specifications.""",
    "require daslib/generic_return",
)

reg("instance_function",
    """The INSTANCE_FUNCTION module provides the ``[instance_function]`` annotation
for creating bound method-like functions. It captures the ``self`` reference
at call time, enabling object-oriented dispatch patterns in daslang.""",
    "require daslib/instance_function",
)

reg("interfaces",
    """The INTERFACES module implements interface-based polymorphism for daslang.
It provides the ``[interface]`` annotation for defining abstract interfaces
with virtual method tables, supporting multiple implementations and dynamic
dispatch without class inheritance.""",
    "require daslib/interfaces",
    example="""\
    require daslib/interfaces

    [interface]
    class IGreeter {
        def abstract greet(name : string) : string
    }

    class MyGreeter {
        def greet(name : string) : string {
            return "Hello, {name}!"
        }
    }

    [export]
    def main() {
        var obj = new MyGreeter()
        print("{obj->greet("world")}\\n")
        unsafe {
            delete obj
        }
    }
    // output:
    // Hello, world!
""")

reg("is_local",
    """The IS_LOCAL module provides compile-time checks for whether a variable
is locally allocated (on the stack) versus heap-allocated. This enables
writing generic code that optimizes differently based on allocation strategy.""",
    "require daslib/is_local",
)

reg("jobque_boost",
    """The JOBQUE_BOOST module provides high-level job queue abstractions built on
the low-level ``jobque`` primitives. It includes ``with_job``, ``with_job_status``,
and channel-based patterns for simplified concurrent programming.

See also :doc:`jobque` for the low-level job queue primitives.""",
    "require daslib/jobque_boost",
    example="""\
    require daslib/jobque_boost

    [export]
    def main() {
        with_job_status(1) <| $(status) {
            new_thread <| @() {
                print("from thread\\n")
                status |> notify_and_release()
            }
            status |> join()
            print("thread done\\n")
        }
    }
    // output:
    // from thread
    // thread done
""")

reg("linked_list",
    """The LINKED_LIST module implements intrusive linked list data structures.
Elements contain embedded next/prev pointers, avoiding separate node
allocations. Useful for implementing queues, work lists, and other
dynamic collections with O(1) insertion and removal.""",
    "require daslib/linked_list",
)

reg("lint",
    """The LINT module implements static analysis checks for daslang code.
It provides customizable lint rules that detect common mistakes, style
violations, and potential bugs at compile time.

See also :doc:`lint_everything` for applying lint diagnostics to all modules.""",
    "require daslib/lint",
)

reg("macro_boost",
    """The MACRO_BOOST module provides utility macros for macro authors, including
pattern matching on AST nodes, code generation helpers, and common
transformation patterns used when writing compile-time code.""",
    "require daslib/macro_boost",
)

reg("math_bits",
    """The MATH_BITS module provides bit manipulation functions for floating point
numbers, including type punning between integer and float representations,
and efficient integer math operations like ``int_bits_to_float`` and
``float_bits_to_int``.""",
    "require daslib/math_bits",
    example="""\
    require daslib/math_bits

    [export]
    def main() {
        let f = uint_bits_to_float(0x3F800000u)
        print("uint_bits_to_float(0x3F800000) = {f}\\n")
        let back = float_bits_to_uint(1.0)
        print("float_bits_to_uint(1.0) = {back}\\n")
    }
    // output:
    // uint_bits_to_float(0x3F800000) = 1
    // float_bits_to_uint(1.0) = 0x3f800000
""")

reg("profiler",
    """The PROFILER module provides CPU profiling infrastructure for measuring
function execution times. It includes instrumentation-based profiling with
hierarchical call tracking and timing statistics.

See also :doc:`profiler_boost` for cross-context profiler control and scoped timing macros.""",
    "require daslib/profiler",
)

reg("profiler_boost",
    """The PROFILER_BOOST module extends profiling with high-level macros for
scoped timing (``profile_block``), function-level profiling annotations,
and formatted output of profiling results.

See also :doc:`profiler` for the core profiling infrastructure.""",
    "require daslib/profiler_boost",
)

reg("quote",
    """The QUOTE module provides quasiquotation support for AST construction.
It allows building AST nodes using daslang syntax with ``$``-prefixed
splice points for inserting computed values, making macro writing more
readable and less error-prone than manual AST construction.""",
    "require daslib/quote",
)

reg("refactor",
    """The REFACTOR module implements automated code refactoring transformations.
It provides tools for renaming symbols, extracting functions, and other
structural code changes that preserve program semantics.""",
    "require daslib/refactor",
)

reg("regex",
    """The REGEX module implements regular expression matching and searching.
It provides ``regex_compile`` for building patterns, ``regex_match`` for
full-string matching, ``regex_search`` for finding the first match anywhere,
``regex_foreach`` for iterating all matches, ``regex_replace`` for substitution,
``regex_split`` for splitting strings, ``regex_match_all`` for collecting all
match ranges, ``regex_group`` for capturing groups by index, and
``regex_group_by_name`` for named group lookup.

Supported syntax:

- ``.`` — any character
- ``^`` — beginning of string (or offset position)
- ``$`` — end of string
- ``+`` — one or more (greedy)
- ``*`` — zero or more (greedy)
- ``?`` — zero or one (greedy)
- ``+?`` — one or more (lazy)
- ``*?`` — zero or more (lazy)
- ``??`` — zero or one (lazy)
- ``{n}`` — exactly *n* repetitions
- ``{n,}`` — *n* or more (greedy)
- ``{n,m}`` — between *n* and *m* (greedy)
- ``{n}?`` ``{n,}?`` ``{n,m}?`` — counted repetitions (lazy)
- ``(...)`` — capturing group
- ``(?:...)`` — non-capturing group
- ``(?P<name>...)`` — named capturing group
- ``|`` — alternation
- ``[abc]``, ``[a-z]``, ``[^abc]`` — character sets (negated with ``^``)
- ``\\w`` ``\\W`` — word / non-word characters
- ``\\d`` ``\\D`` — digit / non-digit characters
- ``\\s`` ``\\S`` — whitespace / non-whitespace characters
- ``\\b`` ``\\B`` — word boundary / non-boundary assertions
- ``\\t`` ``\\n`` ``\\r`` ``\\f`` ``\\v`` — whitespace escapes
- ``\\xHH`` — hexadecimal character escape
- ``\\.`` ``\\+`` ``\\*`` ``\\(`` ``\\)`` ``\\[`` ``\\]`` ``\\|`` ``\\\\`` ``\\^`` ``\\{`` ``\\}`` — escaped metacharacters

The engine is ASCII-only (256-bit ``CharSet``). Matching is anchored — ``regex_match`` tests from
position 0 (or the given offset) and does NOT search; use ``regex_search`` to find the first
occurrence, or ``regex_foreach`` / ``regex_match_all`` to find all occurrences.

See also :doc:`regex_boost` for compile-time regex construction via the ``%regex~`` reader macro.""",
    "require daslib/regex",
    example="""\
    require daslib/regex
    require strings

    [export]
    def main() {
        var re <- regex_compile("[0-9]+")
        let m = regex_match(re, "123abc")
        print("match length = {m}\\n")
        let text = "age 25, height 180"
        regex_foreach(re, text) <| $(r) {
            print("found: {slice(text, r.x, r.y)}\\n")
            return true
        }
    }
    // output:
    // match length = 3
    // found: 25
    // found: 180
""")

reg("regex_boost",
    """The REGEX_BOOST module extends regular expressions with the ``%regex~`` reader
macro for compile-time regex construction. Inside the reader macro, backslashes are
literal — no double-escaping is needed (e.g. ``%regex~\\d{3}%%`` instead of
``"\\\\d\\{3}"``).

See :doc:`regex` for the full list of supported syntax.""",
    "require daslib/regex_boost",
    example="""\
    require daslib/regex_boost
    require strings

    [export]
    def main() {
        var inscope re <- %regex~\\d+%%
        let m = regex_match(re, "123abc")
        print("match length = {m}\\n")
        let text = "age 25, height 180"
        regex_foreach(re, text) <| $(r) {
            print("found: {slice(text, r.x, r.y)}\\n")
            return true
        }
    }
    // output:
    // match length = 3
    // found: 25
    // found: 180
""")

reg("remove_call_args",
    """The REMOVE_CALL_ARGS module provides AST transformation macros that remove
specific arguments from function calls at compile time. Used for implementing
optional parameter patterns and compile-time argument stripping.""",
    "require daslib/remove_call_args",
)

reg("rst",
    """The RST module implements the documentation generation pipeline for daslang.
It uses RTTI to introspect modules, types, and functions, then produces
reStructuredText output suitable for Sphinx documentation builds.""",
    "require daslib/rst",
)

reg("soa",
    """The SOA (Structure of Arrays) module transforms array-of-structures data layouts
into structure-of-arrays layouts for better cache performance. It provides macros
that generate parallel arrays for each field of a structure, enabling SIMD-friendly
data access patterns.""",
    "require daslib/soa",
)

reg("temp_strings",
    """The TEMP_STRINGS module provides temporary string construction that avoids heap
allocations. Temporary strings are allocated on the stack or in scratch memory
and are valid only within the current scope, offering fast string building
for formatting and output.""",
    "require daslib/temp_strings",
)

reg("templates",
    """The TEMPLATES module implements template instantiation utilities for daslang
code generation. It supports stamping out parameterized code patterns with
type and value substitution.

See also :doc:`templates_boost` for template substitution and code generation.""",
    "require daslib/templates",
)

reg("templates_boost",
    """The TEMPLATES_BOOST module extends template utilities with high-level macros
for common code generation patterns, including template function generation,
type-parameterized struct creation, and compile-time code expansion.

See also :doc:`templates` for ``decltype`` and ``[template]`` annotations.""",
    "require daslib/templates_boost",
)

reg("type_traits",
    """The TYPE_TRAITS module provides compile-time type introspection and manipulation.
It includes type queries (``is_numeric``, ``is_string``, ``is_pointer``),
type transformations, and generic programming utilities for writing
type-aware macros and functions.""",
    "require daslib/type_traits",
)

reg("typemacro_boost",
    """The TYPEMACRO_BOOST module provides infrastructure for defining type macros —
custom compile-time type transformations. Type macros allow introducing new
type syntax that expands into standard daslang types during compilation.""",
    "require daslib/typemacro_boost",
)

reg("uriparser_boost",
    """The URIPARSER_BOOST module extends URI handling with convenience functions
for common operations like building URIs from components, extracting query
parameters, and resolving relative paths.""",
    "require daslib/uriparser_boost",
)

reg("utf8_utils",
    """The UTF8_UTILS module provides Unicode UTF-8 string utilities including
character iteration, codepoint extraction, byte length calculation, and
validation of UTF-8 encoded text.""",
    "require daslib/utf8_utils",
)

reg("validate_code",
    """The VALIDATE_CODE module implements AST validation passes that check for
common code quality issues, unreachable code, missing return statements,
and other semantic errors beyond what the type checker verifies.""",
    "require daslib/validate_code",
)

reg("assert_once",
    """The ASSERT_ONCE module provides the ``assert_once`` macro — an assertion that
triggers only on its first failure. Subsequent failures at the same location
are silently ignored, preventing assertion storms in loops or frequently
called code.""",
    "require daslib/assert_once",
)


def format_module_doc(name, desc, require_line, example=None, after_require=""):
    """Format a module documentation file."""
    lines = []
    lines.append(desc.strip())
    lines.append("")
    lines.append(f'All functions and symbols are in "{name}" module, use require to get access to it. ::')
    lines.append("")
    lines.append(f"    {require_line}")
    lines.append("")

    if after_require:
        lines.append(after_require.strip())
        lines.append("")

    if example:
        lines.append("Example: ::")
        lines.append("")
        # Dedent: remove common leading whitespace from example lines
        example_lines = example.strip().split("\n")
        # Find minimum indent
        min_indent = float('inf')
        for line in example_lines:
            if line.strip():
                indent = len(line) - len(line.lstrip())
                min_indent = min(min_indent, indent)
        if min_indent == float('inf'):
            min_indent = 0
        for line in example_lines:
            if line.strip():
                dedented = line[min_indent:]
                lines.append(f"    {dedented}")
            else:
                lines.append("")
        lines.append("")

    return "\n".join(lines)


def main():
    written = 0
    skipped = 0
    missing = 0

    # Get list of existing module files
    existing = set()
    for f in os.listdir(HANDMADE):
        if f.startswith("module-") and f.endswith(".rst"):
            existing.add(f[len("module-"):-4])

    for name, (desc, require_line, example, after_require) in sorted(MODULES.items()):
        fname = f"module-{name}.rst"
        path = os.path.join(HANDMADE, fname)
        if not os.path.exists(path):
            print(f"MISSING: {fname}")
            missing += 1
            continue

        content = format_module_doc(name, desc, require_line, example, after_require)
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.write(content)
        written += 1
        has_ex = "+ example" if example else ""
        print(f"Updated: {fname} {has_ex}")

    # Report modules we don't have entries for
    covered = set(MODULES.keys())
    uncovered = existing - covered
    if uncovered:
        print(f"\nNot covered ({len(uncovered)}):")
        for name in sorted(uncovered):
            print(f"  module-{name}.rst")

    print(f"\nWritten: {written}, Missing: {missing}, Uncovered: {len(uncovered)}")


if __name__ == "__main__":
    main()
