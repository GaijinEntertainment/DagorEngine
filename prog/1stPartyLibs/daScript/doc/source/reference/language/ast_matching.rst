.. _ast_matching:

============
AST Matching
============

.. index::
    single: AST Matching

AST matching is reverse :ref:`reification <reification>` — instead of building AST from values,
it matches compiled AST against structural patterns and extracts values from it.

The module ``daslib/ast_match`` provides three macros: ``qmatch``, ``qmatch_block``, and ``qmatch_function``.
They use the same tag system as ``qmacro`` (``$e``, ``$v``, ``$i``, ``$t``, ``$c``, ``$f``, ``$b``, ``$a``)
but in reverse — tags extract values from the matched expression instead of substituting values in.

.. code-block:: das

    require daslib/ast_match

--------------
Simple example
--------------

Match a compiled expression and extract parts of it:

.. code-block:: das

    var inscope expr <- qmacro(a + b)
    var left_name : string
    let r = qmatch(expr, $i(left_name) + b)
    assert(r.matched)
    assert(left_name == "a")

``qmatch`` compiles the pattern ``$i(left_name) + b`` into a series of checks:
verify the expression is ``ExprOp2`` with ``op="+"`` and right operand ``ExprVar(name="b")``,
then extract the left operand's identifier name into ``left_name``.

------
Macros
------

qmatch
^^^^^^

``qmatch(expr, pattern)`` matches a single expression against a structural pattern.
Returns a :ref:`QMatchResult <struct-ast_match-QMatchResult>`.

.. code-block:: das

    var inscope expr <- qmacro(x * 2 + 1)
    let r = qmatch(expr, x * 2 + 1)
    assert(r.matched)

The pattern is **not type-inferred** — it stays as raw AST, the same way ``qmacro`` patterns work.
This means identifier names in the pattern refer to AST variable names, not local variables.

qmatch_block
^^^^^^^^^^^^^

``qmatch_block(expr) $ { stmts }`` matches a block's statement list with wildcard support.
Optionally matches block arguments and return type:

.. code-block:: das

    var inscope blk <- qmacro_block() <| $ {
        var x = 1
        print("{x}\n")
        return x
    }
    let r = qmatch_block(blk) $ {
        _wildcard()
        return x
    }
    assert(r.matched)

With argument and return type matching:

.. code-block:: das

    let r = qmatch_block(blk) $(x : int) : int {
        _wildcard()
        return x
    }

Arguments are matched strictly — if the pattern specifies zero arguments, the target must also have
zero arguments. Use ``$a(var)`` to match any remaining arguments (see below). Omitting the return
type matches any return type.

qmatch_function
^^^^^^^^^^^^^^^

``qmatch_function(func) $(args) : RetType { stmts }`` matches a compiled function's
arguments, return type, and body:

.. code-block:: das

    [export]
    def target_add(a, b : int) : int {
        return a + b
    }

    [test]
    def test_add(t : T?) {
        var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
        let r = qmatch_function(func) $(a : int; b : int) : int {
            return a + b
        }
        assert(r.matched)
    }

The function has been through compilation and optimization, so the AST may differ from the source.
Compiler-injected ``fakeContext`` and ``fakeLineInfo`` arguments are filtered automatically.

----
Tags
----

Tags extract values from matched AST positions. They mirror the :ref:`reification <reification>` escape sequences.

$e(var) — expression capture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Captures a matched sub-expression as a cloned ``ExpressionPtr``.

.. code-block:: das

    var inscope cond, then_val, else_val : ExpressionPtr
    let r = qmatch(expr, $e(cond) ? $e(then_val) : $e(else_val))

$v(var) — value extraction
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Extracts a constant value from ``ExprConst*`` nodes.
The variable type determines which constant type is expected:

.. code-block:: das

    var val : int
    let r = qmatch(expr, a + $v(val))
    // r.matched if the right operand is ExprConstInt; val receives the value

Supported types: ``int``, ``float``, ``bool``, ``string``, ``int64``, ``uint``, ``double``,
``uint64``, ``int8``, ``uint8``, ``int16``, ``uint16``.

$i(var) — identifier name
^^^^^^^^^^^^^^^^^^^^^^^^^^

Extracts an identifier name as ``string``:

.. code-block:: das

    var name : string
    let r = qmatch(expr, $i(name) + b)    // captures variable name

Also works on ``let`` declarations and ``for`` loop iterators:

.. code-block:: das

    let r = qmatch_block(blk) $ {
        var $i(var_name) = 5
    }

$t(var) — type capture
^^^^^^^^^^^^^^^^^^^^^^^

Captures a ``TypeDeclPtr`` from type positions:

.. code-block:: das

    var inscope captured_type : TypeDeclPtr
    let r = qmatch(expr, type<$t(captured_type)>)

$c(var) — call name capture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Matches call arguments structurally, captures the call name:

.. code-block:: das

    var call_name : string
    let r = qmatch(expr, $c(call_name)(a, b))

When the call targets a generic function instance, ``$c`` returns the original generic name
(before mangling).

$f(var) — field name capture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Matches the field value, captures the field name:

.. code-block:: das

    var field : string
    let r = qmatch(expr, obj.$f(field))

$a(var) — remaining arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Captures remaining arguments after the explicitly listed ones as ``array<VariablePtr>``.
Works in both ``qmatch_function`` and ``qmatch_block``. Fixed arguments before ``$a``
are matched by name and type as usual; everything after goes into the array.
``$a`` must be the last argument in the pattern.

Capture remaining function arguments:

.. code-block:: das

    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(a : int; $a(rest)) {
        _wildcard()
    }
    // rest contains all arguments after `a`

Capture all arguments (no fixed prefix):

.. code-block:: das

    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $($a(rest)) {
        _wildcard()
    }
    // rest contains every argument

Works the same on block arguments:

.. code-block:: das

    var inscope rest : array<VariablePtr>
    let r = qmatch_block(blk) $(a : int; $a(rest)) {
        return a
    }

If all fixed arguments match but no arguments remain, ``$a`` captures an empty array
and the match succeeds. If there are fewer actual arguments than fixed ones in the pattern,
the match fails.

$b(var) — statement range capture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Passed as an argument to a wildcard (``_wildcard``, ``_wildcard1``, ``_optional``, ``_any``),
captures the matched statements as ``array<ExpressionPtr>``. Each statement is cloned.

Capture trailing statements (everything after ``foo()``):

.. code-block:: das

    var inscope stmts : array<ExpressionPtr>
    let r = qmatch_block(blk) $ {
        foo()
        _wildcard($b(stmts))
    }

Capture leading statements (everything before ``baz()``):

.. code-block:: das

    var inscope stmts : array<ExpressionPtr>
    let r = qmatch_block(blk) $ {
        _wildcard($b(stmts))
        baz()
    }

Capture a middle range between two fixed statements:

.. code-block:: das

    var inscope stmts : array<ExpressionPtr>
    let r = qmatch_block(blk) $ {
        foo()
        _wildcard($b(stmts))
        bar()
    }
    // stmts contains everything between foo() and bar()

When the wildcard matches zero statements, ``$b`` captures an empty array.
The ``$b`` tag is optional — wildcards work without it, they just don't capture.

---------
Wildcards
---------

Wildcards match variable numbers of statements in block and function patterns:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Wildcard
     - Matches
   * - ``_wildcard()``
     - 0 or more statements
   * - ``_wildcard1()``
     - 1 or more statements
   * - ``_optional()``
     - 0 or 1 statement (fails if 2+)
   * - ``_any()``
     - Exactly 1 statement

All wildcards accept an optional ``$b(var)`` argument to capture the matched statements.

.. code-block:: das

    // Match: any prefix, then if(...), then any suffix
    let r = qmatch_block(blk) $ {
        _wildcard()
        if (a < 5) { return b }
        _wildcard()
    }

--------------
Type matching
--------------

Types in patterns are matched structurally — base type, const/ref/pointer flags, struct/enum names,
and child types are all compared recursively.

auto (type wildcard)
^^^^^^^^^^^^^^^^^^^^

Use ``auto`` in any type position to match any type:

.. code-block:: das

    // Matches any array type
    let r = qmatch(expr, type<array<auto>>)

    // Matches any table with string keys
    let r2 = qmatch(expr, type<table<string; auto>>)

-----------------------------------
Constant constructors and folding
-----------------------------------

The compiler folds constant constructor calls like ``range(1, 10)`` into ``ExprConstRange`` values.
``ast_match`` handles this transparently — a pattern ``range(1, 10)`` matches the folded constant.

Tags work inside constant constructors:

.. code-block:: das

    var x_val : float
    let r = qmatch(expr, float4($v(x_val), 2.0, 3.0, 4.0))
    // Matches ExprConstFloat4, captures the x component

Supported constructors: ``range``, ``urange``, ``range64``, ``urange64``,
``int2``, ``int3``, ``int4``, ``uint2``, ``uint3``, ``uint4``,
``float2``, ``float3``, ``float4``.

-----------------------------------
Generic function matching
-----------------------------------

When matching calls to generic function instances, the compiler mangles the call name
(e.g. ``generic_add`12345``). ``ast_match`` resolves the original name via ``fromGeneric``,
so patterns like ``generic_add(a, b)`` match transparently.

``$c(name)`` on a generic call also returns the original (unmangled) name.

-----------
Result type
-----------

.. code-block:: das

    enum QMatchError : int {
        ok
        rtti_mismatch
        field_mismatch
        const_type_mismatch
        type_mismatch
        list_length
        wildcard_not_found
        null_expression
    }

    struct QMatchResult {
        matched : bool
        error   : QMatchError
        expr    : Expression const?
    }

When a match fails, ``error`` indicates why and ``expr`` points to the mismatched AST node
(useful for diagnostics).

.. seealso::

    :ref:`Reification <reification>` for the forward direction — building AST from values using the same tag system,
    :ref:`Macros <macros>` for the compilation lifecycle where AST matching is used,
    :ref:`AST pattern matching module <stdlib_ast_match>` for the full API reference.
