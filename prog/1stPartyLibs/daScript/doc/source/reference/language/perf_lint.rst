.. _perf_lint:

=================================
Performance Lint (``perf_lint``)
=================================

.. index::
    single: perf_lint
    single: Performance Lint

The ``perf_lint`` module detects common performance anti-patterns in daslang code
at compile time. When required, a lint pass runs after compilation and reports
warnings as ``CompilationError::performance_lint`` (error code ``40217``).

-----------
Quick start
-----------

Add ``require daslib/perf_lint`` to any file. The lint runs automatically at compile
time and reports warnings inline::

    options gen2
    require daslib/perf_lint

    def process(data : string) : string {
        var result = ""
        for (i in range(100)) {
            result += "x"           // warning: PERF001
        }
        return result
    }

-------------------
Standalone utility
-------------------

A standalone utility is available for batch-checking files from the command line::

    bin/Release/daslang.exe utils/perf_lint/main.das -- file1.das file2.das [--quiet]

The utility compiles each file (without simulation or execution), runs the lint
visitor, and prints any warnings. Use ``--quiet`` to suppress progress messages.
Exit code is ``1`` if any files failed to compile, ``0`` otherwise.

-----
Rules
-----

PERF001 — string ``+=`` in loop
================================

String concatenation with ``+=`` inside a loop creates O(n\ :sup:`2`) allocations.
Each iteration allocates a new string of increasing length, copying all previous content.

.. code-block:: das

    // Bad — O(n^2)
    var result = ""
    for (i in range(100)) {
        result += "x"                   // PERF001
    }

    // Good — O(n)
    let result = build_string() <| $(var writer) {
        for (i in range(100)) {
            write(writer, "x")
        }
    }

PERF002 — ``character_at`` in loop with loop variable
======================================================

``character_at(s, i)`` is O(n) per call because it internally calls ``strlen``
to validate the index. In a loop iterating over string indices with the loop
variable as the index, this becomes O(n\ :sup:`2`) total.

.. code-block:: das

    // Bad — O(n^2)
    for (i in range(length(s))) {
        let ch = character_at(s, i)     // PERF002
    }

    // Good — O(n) total, O(1) per access
    peek_data(s) <| $(arr) {
        for (i in range(length(arr))) {
            let ch = int(arr[i])
        }
    }

PERF003 — ``character_at`` anywhere
====================================

Informational warning for any use of ``character_at``. Each call does a bounds
check by scanning to the index. For accessing the first character, use
``first_character`` which is O(1). For bulk access in hot paths, consider
``peek_data`` for reads or ``modify_data`` for mutations.

.. code-block:: das

    let ch = character_at(s, 0)         // PERF003 — use first_character(s) instead
    let ch2 = first_character(s)        // O(1), returns 0 for empty string

    // Alternative: peek_data for O(1) indexed access
    peek_data(s) <| $(arr) {
        let ch = int(arr[0])
    }

PERF004 — string interpolation reassignment in loop
=====================================================

``str = "{str}{more}"`` inside a loop has the same O(n\ :sup:`2`) behavior as
``str += "..."``. Each iteration allocates a new string containing all previous
content.

.. code-block:: das

    // Bad — O(n^2)
    var result = ""
    for (i in range(100)) {
        result = "{result}x"            // PERF004
    }

    // Good — O(n)
    let result = build_string() <| $(var writer) {
        for (i in range(100)) {
            write(writer, "x")
        }
    }

PERF005 — ``length(string)`` in while condition
=================================================

``while (i < length(s))`` recomputes ``strlen(s)`` on every iteration. If ``s``
is not modified in the loop body, this is wasted work. Note that ``for`` loops
do **not** have this problem because ``for`` computes its source expression once.

.. code-block:: das

    // Bad — strlen every iteration
    var i = 0
    while (i < length(s)) {             // PERF005
        i ++
    }

    // Good — cached length
    let slen = length(s)
    var i = 0
    while (i < slen) {
        i ++
    }

PERF006 — ``push``/``emplace`` in loop without ``reserve()``
=============================================================

Calling ``push``, ``push_clone``, or ``emplace`` on an array inside a loop without
a preceding ``reserve()`` may trigger repeated reallocations as the array grows.
The rule traces through field access chains (``self.items``, ``data.buffer``, etc.)
to find the root variable, and distinguishes different field paths — ``reserve(t.a, N)``
does not suppress a warning for ``t.b |> push(x)``.

Conditional pushes (inside ``if``/``else``) and loops with ``break``/``continue``
are not flagged — the number of items is unpredictable, so ``reserve`` would be
guesswork.

.. code-block:: das

    // Bad — may realloc each iteration
    var result : array<int>
    for (i in range(1000)) {
        result |> push(i)                       // PERF006
    }

    // Bad — field access, no reserve on this path
    for (i in range(1000)) {
        self.items |> push(i)                   // PERF006
    }

    // Good — pre-allocate
    var result : array<int>
    result |> reserve(1000)
    for (i in range(1000)) {
        result |> push(i)
    }

    // Good — conditional push, no warning
    for (i in range(1000)) {
        if (i > 500) {
            result |> push(i)
        }
    }

    // Good — loop with break, no warning
    for (x in data) {
        if (x == sentinel) {
            break
        }
        result |> push(x)
    }

PERF007 — unnecessary ``string(das_string)`` in comparison
============================================================

``das_string`` supports direct comparison with string literals and other
``das_string`` values via ``==`` and ``!=``. Wrapping in ``string()`` allocates
a new string unnecessarily.

.. code-block:: das

    // Bad — unnecessary allocation
    if (string(name) == "foo") { ... }      // PERF007
    if (string(a) == string(b)) { ... }     // PERF007

    // Good — direct comparison
    if (name == "foo") { ... }
    if (a == b) { ... }

PERF008 — unnecessary ``get_ptr()`` for ``is``/``as``
======================================================

``smart_ptr<Expression>`` and ``smart_ptr<TypeDecl>`` support ``is`` and ``as``
type checks directly. Calling ``get_ptr()`` first to convert to a raw pointer
is unnecessary.

.. code-block:: das

    // Bad — get_ptr is redundant
    if (get_ptr(expr) is ExprVar) { ... }   // PERF008
    var v = get_ptr(expr) as ExprCall       // PERF008

    // Good — direct type check
    if (expr is ExprVar) { ... }
    var v = expr as ExprCall

PERF009 — redundant move-init variable immediately returned
=============================================================

``var x <- expr(); return <- x`` introduces an unnecessary intermediate variable.
The value is moved in and then immediately moved out. Simplify to
``return <- expr()``.

.. code-block:: das

    // Bad — redundant variable
    var inscope result <- make_thing()
    return <- result                        // PERF009

    // Good — direct return
    return <- make_thing()

------------------------------
Suppressing specific warnings
------------------------------

To suppress a warning on a specific line, add a ``// nolint:PERFxxx`` comment
on the same line as the flagged expression::

    let ch = character_at(s, idx) // nolint:PERF003 single indexed access, not a loop

The suppression is exact: ``// nolint:PERF003`` only suppresses PERF003, not other
rules. The comment must appear after ``//`` on the same line that triggers the warning.
An optional explanation after the code is recommended but not required.

----------------
Important notes
----------------

**Lint runs after optimization.** The lint pass runs on the post-optimization AST.
This means patterns in dead code (unused variables, unreachable functions) may not
trigger warnings. In real code where results are used, the patterns are preserved
and detected correctly.

**ExprRef2Value wrapping.** The compiler wraps many value-type reads in
``ExprRef2Value`` nodes. The lint visitor unwraps these transparently — this is
an implementation detail, not something users need to worry about.

**Closures are excluded.** Code inside closures (blocks, lambdas) is not checked
for loop-related patterns, since the closure may be called outside the loop context.

-----
Tests
-----

Tests are in ``utils/perf_lint/tests/``::

    bin/Release/daslang.exe dastest/dastest.das -- --test utils/perf_lint/tests

.. seealso::

    ``daslib/perf_lint.das`` (source),
    ``utils/perf_lint/main.das`` (standalone utility)
