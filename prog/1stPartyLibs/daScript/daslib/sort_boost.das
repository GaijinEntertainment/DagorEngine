options gen2
options indenting = 4
options strict_smart_pointers = true

module sort_boost shared private

//! Extended sorting support: partial_sort, nth_element, heap operations, and the
//! ``qsort`` family of dispatcher macros that route to the right ``__builtin_*``
//! variant based on container type.
//!
//! Phase 0 wrappers cover ``array<T>`` directly; for handled vector types use
//! the matching ``q*`` macro (see ``qsort``-style dispatch macros at the bottom
//! of this module), which wraps the input in ``temp_array`` before dispatching.

require daslib/ast_boost
require daslib/array_boost public
require daslib/macro_boost

// ============================================================================
// User-facing wrappers — array<T> only. Each routes to one of the typed,
// any-cblock, or any-ref-cblock __builtin_* variants based on TT.
//
// Empty/length<=1 arrays are no-ops at the C++ level; n bounds (n<=0 for
// partial_sort, n out of range for nth_element) are also clamped C++-side.
// These wrappers focus on routing, not validation.
// ============================================================================

// ---- partial_sort ----

//! Sort the first ``n`` elements of ``a`` in ascending order.
//! Remaining elements are left in unspecified order. O(M log N) average.
def public partial_sort(var a : array<auto(TT)> | #; n : int) {
    if (length(a) <= 1 || n <= 0) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        __builtin_array_lock(a)
        unsafe {
            __builtin_partial_sort(addr(a[0]), length(a), n)
        }
        __builtin_array_unlock(a)
    } static_elif (typeinfo is_vector(type<TT>)) {
        concept_assert(false, "there is no default compare function for vectors")
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_partial_sort_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), n) $(x, y : TT) : bool {
                    return x < y
                }
            } else {
                __builtin_partial_sort_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), n) $(x, y : TT) : bool {
                    return x < y
                }
            }
        }
    }
}

//! Sort the first ``n`` elements of ``a`` using ``cmp(x, y) -> "x < y"``.
def public partial_sort(var a : array<auto(TT)> | #; n : int; cmp : block<(x, y : TT) : bool>) {
    if (length(a) <= 1 || n <= 0) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        unsafe {
            __builtin_partial_sort_cblock_array(a, typeinfo sizeof(a[0]), length(a), n, cmp)
        }
    } static_elif (typeinfo is_vector(type<TT>)) {
        unsafe {
            __builtin_partial_sort_cblock_array(a, typeinfo sizeof(a[0]), length(a), n, cmp)
        }
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_partial_sort_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), n, cmp)
            } else {
                __builtin_partial_sort_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), n, cmp)
            }
        }
    }
}

// ---- nth_element ----

//! Place the ``n``-th smallest element at position ``n``, with all earlier
//! elements <= and all later elements >=. O(N) average on the kth element only.
def public nth_element(var a : array<auto(TT)> | #; n : int) {
    if (length(a) <= 1 || n < 0 || n >= length(a)) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        __builtin_array_lock(a)
        unsafe {
            __builtin_nth_element(addr(a[0]), length(a), n)
        }
        __builtin_array_unlock(a)
    } static_elif (typeinfo is_vector(type<TT>)) {
        concept_assert(false, "there is no default compare function for vectors")
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_nth_element_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), n) $(x, y : TT) : bool {
                    return x < y
                }
            } else {
                __builtin_nth_element_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), n) $(x, y : TT) : bool {
                    return x < y
                }
            }
        }
    }
}

//! Place the ``n``-th element under ``cmp`` ordering at position ``n``.
def public nth_element(var a : array<auto(TT)> | #; n : int; cmp : block<(x, y : TT) : bool>) {
    if (length(a) <= 1 || n < 0 || n >= length(a)) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        unsafe {
            __builtin_nth_element_cblock_array(a, typeinfo sizeof(a[0]), length(a), n, cmp)
        }
    } static_elif (typeinfo is_vector(type<TT>)) {
        unsafe {
            __builtin_nth_element_cblock_array(a, typeinfo sizeof(a[0]), length(a), n, cmp)
        }
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_nth_element_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), n, cmp)
            } else {
                __builtin_nth_element_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), n, cmp)
            }
        }
    }
}

// ---- Heap operations ----
//
// make_heap turns the array into a binary max-heap.
// push_heap assumes the new element was just appended at the end.
// pop_heap moves the heap max to the end; the caller drops the last slot.

//! Restructure ``a`` into a binary max-heap (using ``<``).
def public make_heap(var a : array<auto(TT)> | #) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        __builtin_array_lock(a)
        unsafe {
            __builtin_make_heap(addr(a[0]), length(a))
        }
        __builtin_array_unlock(a)
    } static_elif (typeinfo is_vector(type<TT>)) {
        concept_assert(false, "there is no default compare function for vectors")
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_make_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            } else {
                __builtin_make_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            }
        }
    }
}

//! Restructure ``a`` into a heap ordered by ``cmp(x, y) -> "x precedes y"``.
def public make_heap(var a : array<auto(TT)> | #; cmp : block<(x, y : TT) : bool>) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        unsafe {
            __builtin_make_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } static_elif (typeinfo is_vector(type<TT>)) {
        unsafe {
            __builtin_make_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_make_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            } else {
                __builtin_make_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            }
        }
    }
}

//! Sift up the just-appended last element to maintain the max-heap property.
//! Pre: a[0..length-2] is already a valid heap and a[length-1] is the new element.
def public push_heap(var a : array<auto(TT)> | #) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        __builtin_array_lock(a)
        unsafe {
            __builtin_push_heap(addr(a[0]), length(a))
        }
        __builtin_array_unlock(a)
    } static_elif (typeinfo is_vector(type<TT>)) {
        concept_assert(false, "there is no default compare function for vectors")
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_push_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            } else {
                __builtin_push_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            }
        }
    }
}

//! Sift up the last element using ``cmp``.
def public push_heap(var a : array<auto(TT)> | #; cmp : block<(x, y : TT) : bool>) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        unsafe {
            __builtin_push_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } static_elif (typeinfo is_vector(type<TT>)) {
        unsafe {
            __builtin_push_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_push_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            } else {
                __builtin_push_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            }
        }
    }
}

//! Move the heap max to a[length-1]; the caller is expected to drop the last
//! slot after (pop_back / resize).
def public pop_heap(var a : array<auto(TT)> | #) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        __builtin_array_lock(a)
        unsafe {
            __builtin_pop_heap(addr(a[0]), length(a))
        }
        __builtin_array_unlock(a)
    } static_elif (typeinfo is_vector(type<TT>)) {
        concept_assert(false, "there is no default compare function for vectors")
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_pop_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            } else {
                __builtin_pop_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a)) $(x, y : TT) : bool {
                    return x < y
                }
            }
        }
    }
}

//! Move the max-under-``cmp`` element to a[length-1].
def public pop_heap(var a : array<auto(TT)> | #; cmp : block<(x, y : TT) : bool>) {
    if (length(a) <= 1) return
    static_if (typeinfo is_numeric_comparable(type<TT>)) {
        unsafe {
            __builtin_pop_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } static_elif (typeinfo is_vector(type<TT>)) {
        unsafe {
            __builtin_pop_heap_cblock_array(a, typeinfo sizeof(a[0]), length(a), cmp)
        }
    } else {
        unsafe {
            static_if (typeinfo is_ref_type(type<TT>)) {
                __builtin_pop_heap_array_any_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            } else {
                __builtin_pop_heap_array_any_ref_cblock(a, typeinfo sizeof(a[0]), length(a), cmp)
            }
        }
    }
}

// ============================================================================
// q*-prefixed dispatch macros — same shape as qsort. Each emits a call to the
// matching user-facing wrapper above (which then dispatches to the right
// __builtin_*). For handled vector types, wraps the source in temp_array.
// ============================================================================

[call_macro(name="qsort")]
class QsortMacro : AstCallMacro {
    //! Implements `qsort` macro. It's `qsort(value,block)`.
    //! For the regular array<> or dim it's replaced with `sort(value,block)`.
    //! For the handled types like das`vector its replaced with `sort(temp_array(value),block)`.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "expecting qsort(value, block)")
        if (expr.arguments[0]._type == null || expr.arguments[1]._type == null) return null
        // todo: verify correct block type?
        macro_verify(expr.arguments[1]._type.baseType == Type.tBlock, prog, expr.at, "expecting 2nd argument to be block(<a,b:auto(TT)>:bool))")
        if (expr.arguments[0]._type.baseType == Type.tFixedArray || expr.arguments[0]._type.baseType == Type.tArray) {
            var call = new ExprCall(name := "sort", at = expr.at)
            emplace_new(call.arguments) <| clone_expression(expr.arguments[0])
            emplace_new(call.arguments) <| clone_expression(expr.arguments[1])
            return <- call
        } elif (expr.arguments[0]._type.baseType == Type.tHandle) {
            var call = new ExprCall(name := "sort", at = expr.at)
            var tsa = new ExprCall(name := "temp_array", at = expr.at)
            emplace_new(tsa.arguments) <| clone_expression(expr.arguments[0])
            tsa.genFlags.alwaysSafe = true
            emplace(call.arguments, tsa)
            emplace_new(call.arguments) <| clone_expression(expr.arguments[1])
            return <- call
        } else {
            macro_error(prog, expr.at, "can only qsort [], array, or handled vector")
            return null
        }
    }
}

// Internal helper: build an ``opName(value [, n], block)`` call from the macro's call expression, wrapping ``value`` in temp_array for handled vectors.
def private build_dispatch_call(prog : ProgramPtr; var expr : ExprCallMacro?; macroName, opName : string; takesN : bool) : ExpressionPtr {
    let expectedArgs = takesN ? 3 : 2
    let argShape = takesN ? "(value, n, block)" : "(value, block)"
    macro_verify(expr.arguments |> length == expectedArgs, prog, expr.at, "expecting {macroName}{argShape}")
    for (a in expr.arguments) {
        if (a._type == null) return null
    }
    let blockIndex = takesN ? 2 : 1
    macro_verify(expr.arguments[blockIndex]._type.baseType == Type.tBlock, prog, expr.at, "expecting block argument as last")
    let srcType = expr.arguments[0]._type
    if (srcType.baseType == Type.tArray) {
        var call = new ExprCall(name := opName, at = expr.at)
        for (a in expr.arguments) {
            emplace_new(call.arguments) <| clone_expression(a)
        }
        return <- call
    } elif (srcType.baseType == Type.tHandle) {
        var call = new ExprCall(name := opName, at = expr.at)
        var tsa = new ExprCall(name := "temp_array", at = expr.at)
        emplace_new(tsa.arguments) <| clone_expression(expr.arguments[0])
        tsa.genFlags.alwaysSafe = true
        emplace(call.arguments, tsa)
        for (i in range(1, expr.arguments |> length)) {
            emplace_new(call.arguments) <| clone_expression(expr.arguments[i])
        }
        return <- call
    } else {
        macro_error(prog, expr.at, "{macroName} can only operate on array or handled vector")
        return null
    }
}

[call_macro(name="qpartial_sort")]
class QPartialSortMacro : AstCallMacro {
    //! Implements ``qpartial_sort(value, n, block)`` — dispatches to
    //! ``partial_sort`` for array sources or ``partial_sort(temp_array(value),
    //! …)`` for handled vector types.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        return <- build_dispatch_call(prog, expr, "qpartial_sort", "partial_sort", true)
    }
}

[call_macro(name="qnth_element")]
class QNthElementMacro : AstCallMacro {
    //! Implements ``qnth_element(value, n, block)`` for arrays and handled
    //! vectors. Same dispatch shape as ``qpartial_sort``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        return <- build_dispatch_call(prog, expr, "qnth_element", "nth_element", true)
    }
}

[call_macro(name="qmake_heap")]
class QMakeHeapMacro : AstCallMacro {
    //! Implements ``qmake_heap(value, block)`` for arrays and handled vectors.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        return <- build_dispatch_call(prog, expr, "qmake_heap", "make_heap", false)
    }
}

[call_macro(name="qpush_heap")]
class QPushHeapMacro : AstCallMacro {
    //! Implements ``qpush_heap(value, block)`` for arrays and handled vectors.
    //! Pre: new element already at end.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        return <- build_dispatch_call(prog, expr, "qpush_heap", "push_heap", false)
    }
}

[call_macro(name="qpop_heap")]
class QPopHeapMacro : AstCallMacro {
    //! Implements ``qpop_heap(value, block)`` for arrays and handled vectors.
    //! Moves max to end; caller drops it.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        return <- build_dispatch_call(prog, expr, "qpop_heap", "pop_heap", false)
    }
}
