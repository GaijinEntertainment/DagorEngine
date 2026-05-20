options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module result shared public

require daslib/templates
require daslib/templates_boost
require daslib/typemacro_boost
require daslib/option public

//! Monadic ``Result<T, E>`` type — a value (``ok``) or an error (``err``).
//!
//! Functional API for modelling fallible computations where an
//! error carries meaning (unlike a bare ``Option<T>`` that only says "missing").
//! Both the value side ``T`` and the error side ``E`` may be non-copyable
//! (``array<T>``, ``table<K;V>``, lambdas) — combinators clone or move on the
//! non-copyable branch via ``static_if (typeinfo can_copy(...))``.
//!
//! Construct via ``ok(v, type<E>)`` / ``err(e, type<T>)`` (clone), or
//! ``move_ok(v, type<E>)`` / ``move_err(e, type<T>)`` to move out of a mutable
//! source. Compose via ``map`` / ``map_err`` / ``and_then`` / ``or_else``;
//! extract via ``unwrap`` / ``unwrap_or`` / ``??``.

//! Tagged ``ok``/``err`` pair over payload ``T`` and error ``E``.
//!
//! When ``is_ok`` is ``true``, ``value`` is meaningful and ``error`` holds the zero
//! value of ``E``. When ``is_ok`` is ``false``, ``error`` is meaningful and ``value``
//! holds the zero value of ``T``. Prefer the accessor functions below over direct
//! field reads.
[template_structure(T, E)]
struct template Result {
    _is_ok : bool = false
    @safe_when_uninitialized
    _value : T
    @safe_when_uninitialized
    _error : E
}

//! Construct ``Result<T, E>`` carrying successful ``v``. Pass the error type as a witness::
//!
//!     let r = ok(42, type<string>)
//!
//! For non-copyable ``T`` the payload is cloned; if you want to move out of a
//! mutable source, use ``move_ok`` instead.
[template(etype)]
def ok(v : auto(TT); etype : auto(EE)) {
    typedef RT = $Result < TT - const; EE - const >
    var r = default<RT>
    r._is_ok = true
    static_if (typeinfo can_copy(r._value)) {
        r._value = v
    } else {
        r._value <- clone_to_move(v)
    }
    return <- r
}

//! Construct ``Result<T, E>`` by moving the value payload out of a mutable
//! source. After ``move_ok(src, type<E>)`` the source is left empty
//! (``length(src) == 0`` for arrays/tables). Useful for non-copyable
//! payloads where ``ok`` would clone.
[template(etype)]
def move_ok(var v : auto(TT); etype : auto(EE)) {
    typedef RT = $Result < TT - const; EE - const >
    return <- RT(_is_ok = true, _value <- v)
}

//! Construct ``Result<T, E>`` carrying error ``e``. Pass the value type as a witness::
//!
//!     let r = err("boom", type<int>)
//!
//! For non-copyable ``E`` the error is cloned; if you want to move out of a
//! mutable source, use ``move_err`` instead.
[template(ttype)]
def err(e : auto(EE); ttype : auto(TT)) {
    typedef RT = $Result < TT - const; EE - const >
    var r = default<RT>
    r._is_ok = false
    static_if (typeinfo can_copy(r._error)) {
        r._error = e
    } else {
        r._error <- clone_to_move(e)
    }
    return <- r
}

//! Construct ``Result<T, E>`` by moving the error payload out of a mutable
//! source. After ``move_err(src, type<T>)`` the source is left empty
//! (``length(src) == 0`` for arrays/tables). Useful for non-copyable error
//! payloads where ``err`` would clone.
[template(ttype)]
def move_err(var e : auto(EE); ttype : auto(TT)) {
    typedef RT = $Result < TT - const; EE - const >
    return <- RT(_is_ok = false, _error <- e)
}

//! ``true`` iff the result is a successful ``ok``.
def is_ok(r : $Result < auto(TT); auto(EE) >) : bool {
    return r._is_ok
}

//! ``true`` iff the result is a failed ``err``.
def is_err(r : $Result < auto(TT); auto(EE) >) : bool {
    return !r._is_ok
}

//! Apply ``f`` to the ok payload. ``err`` stays ``err`` (same ``E``).
def map(r : $Result < auto(TT); auto(EE) >; f : block<(v : TT) : auto(UU)>) {
    var out = default<$Result<UU -const; EE -const> >
    if (r._is_ok) {
        out._is_ok = true
        static_if (typeinfo can_copy(out._value)) {
            out._value = invoke(f, r._value)
        } else {
            out._value <- invoke(f, r._value)
        }
    } else {
        static_if (typeinfo can_copy(out._error)) {
            out._error = r._error
        } else {
            out._error := r._error
        }
    }
    return <- out
}

//! Apply ``f`` to the err payload. ``ok`` stays ``ok`` (same ``T``).
def map_err(r : $Result < auto(TT); auto(EE) >; f : block<(e : EE) : auto(FF)>) {
    var out = default<$Result<TT -const; FF -const> >
    if (r._is_ok) {
        out._is_ok = true
        static_if (typeinfo can_copy(out._value)) {
            out._value = r._value
        } else {
            out._value := r._value
        }
    } else {
        static_if (typeinfo can_copy(out._error)) {
            out._error = invoke(f, r._error)
        } else {
            out._error <- invoke(f, r._error)
        }
    }
    return <- out
}

//! Monadic bind on the ok side — ``f`` returns the next ``Result``.
def and_then(r : $Result < auto(TT); auto(EE) >; f : block<(v : TT) : $Result<auto(UU); EE> >) {
    if (r._is_ok) {
        return <- invoke(f, r._value)
    }
    var out = default<$Result<UU -const; EE -const> >
    static_if (typeinfo can_copy(out._error)) {
        out._error = r._error
    } else {
        out._error := r._error
    }
    return <- out
}

//! Monadic bind on the err side — ``f`` can recover or re-map the error.
def or_else(r : $Result < auto(TT); auto(EE) >; f : block<(e : EE) : $Result<TT; auto(FF)> >) {
    if (!r._is_ok) {
        return <- invoke(f, r._error)
    }
    var out = default<$Result<TT -const; FF -const> >
    out._is_ok = true
    static_if (typeinfo can_copy(out._value)) {
        out._value = r._value
    } else {
        out._value := r._value
    }
    return <- out
}

//! Extract the ok payload or panic when ``err``.
def unwrap(r : $Result < auto(TT); auto(EE) >) : TT {
    if (!r._is_ok) {
        panic("unwrap called on err")
    }
    static_if (typeinfo can_copy(r._value)) {
        return r._value
    } else {
        return <- clone_to_move(r._value)
    }
}

//! Destructively extract the ok payload, leaving ``r`` in the err state with
//! the err side defaulted. Mirrors ``move_ok`` on the construction side. Use
//! this instead of ``unwrap`` when ``T`` is non-copyable and you no longer need
//! ``r`` — it avoids the deep clone that ``unwrap`` would otherwise perform.
//! Panics when ``err``.
def move_unwrap(var r : $Result < auto(TT); auto(EE) >) : TT {
    if (!r._is_ok) {
        panic("move_unwrap called on err")
    }
    r._is_ok = false
    return <- r._value
}

//! Extract the ok payload or return ``d`` when ``err``.
def unwrap_or(r : $Result < auto(TT); auto(EE) >; d : TT) : TT {
    static_if (typeinfo can_copy(r._value)) {
        return r._is_ok ? r._value : d
    } else {
        return <- clone_to_move(r._is_ok ? r._value : d)
    }
}

//! Extract the ok payload or compute a fallback from the error via ``f``.
def unwrap_or_else(r : $Result < auto(TT); auto(EE) >; f : block<(e : EE) : TT>) : TT {
    static_if (typeinfo can_copy(r._value)) {
        return r._is_ok ? r._value : invoke(f, r._error)
    } else {
        return <- clone_to_move(r._is_ok ? r._value : invoke(f, r._error))
    }
}

//! Extract the ok payload or return ``default<T>`` when ``err``.
def unwrap_or_default(r : $Result < auto(TT); auto(EE) >) : TT {
    static_if (typeinfo can_copy(r._value)) {
        return r._is_ok ? r._value : default<TT>
    } else {
        return <- clone_to_move(r._is_ok ? r._value : default<TT>)
    }
}

//! Extract the err payload or panic when ``ok``.
def unwrap_err(r : $Result < auto(TT); auto(EE) >) : EE {
    if (r._is_ok) {
        panic("unwrap_err called on ok")
    }
    static_if (typeinfo can_copy(r._error)) {
        return r._error
    } else {
        return <- clone_to_move(r._error)
    }
}

//! Destructively extract the err payload, leaving ``r`` in the ok state with
//! the ok side defaulted. Mirrors ``move_err`` on the construction side.
//! Panics when ``ok``.
def move_unwrap_err(var r : $Result < auto(TT); auto(EE) >) : EE {
    if (r._is_ok) {
        panic("move_unwrap_err called on ok")
    }
    r._is_ok = true
    return <- r._error
}

//! Extract the ok payload or panic with ``msg`` when ``err``. ``expect_value``
//! rather than ``expect`` because ``expect`` is a reserved keyword.
def expect_value(r : $Result < auto(TT); auto(EE) >; msg : string) : TT {
    if (!r._is_ok) {
        panic(msg)
    }
    static_if (typeinfo can_copy(r._value)) {
        return r._value
    } else {
        return <- clone_to_move(r._value)
    }
}

//! Extract the err payload or panic with ``msg`` when ``ok``.
def expect_err(r : $Result < auto(TT); auto(EE) >; msg : string) : EE {
    if (r._is_ok) {
        panic(msg)
    }
    static_if (typeinfo can_copy(r._error)) {
        return r._error
    } else {
        return <- clone_to_move(r._error)
    }
}

//! Invoke ``f`` with the ok payload when ``ok``; no-op when ``err``.
def if_ok(r : $Result < auto(TT); auto(EE) >; f : block<(v : TT) : void>) : void {
    if (r._is_ok) {
        invoke(f, r._value)
    }
}

//! Invoke ``f`` with the err payload when ``err``; no-op when ``ok``.
def if_err(r : $Result < auto(TT); auto(EE) >; f : block<(e : EE) : void>) : void {
    if (!r._is_ok) {
        invoke(f, r._error)
    }
}

//! Drop the err side; ``ok`` becomes ``some``, ``err`` becomes ``none``.
def to_option(r : $Result < auto(TT); auto(EE) >) {
    var o = default<$Option<TT -const> >
    if (r._is_ok) {
        o._has_value = true
        static_if (typeinfo can_copy(o._value)) {
            o._value = r._value
        } else {
            o._value := r._value
        }
    }
    return <- o
}

//! Drop the ok side; ``err`` becomes ``some``, ``ok`` becomes ``none``.
def err_to_option(r : $Result < auto(TT); auto(EE) >) {
    var o = default<$Option<EE -const> >
    if (!r._is_ok) {
        o._has_value = true
        static_if (typeinfo can_copy(o._value)) {
            o._value = r._error
        } else {
            o._value := r._error
        }
    }
    return <- o
}

//! Unwrap or default. Mirrors ``T? ?? default`` for value types.
def operator ??(r : $Result < auto(TT); auto(EE) >; d : TT) : TT {
    static_if (typeinfo can_copy(r._value)) {
        return r._is_ok ? r._value : d
    } else {
        return <- clone_to_move(r._is_ok ? r._value : d)
    }
}

//! Structural equality: same tag, and the active payload compares equal.
def operator ==(a, b : $Result < auto(TT); auto(EE) > implicit) : bool {
    if (a._is_ok != b._is_ok) {
        return false
    }
    return a._is_ok ? a._value == b._value : a._error == b._error
}

//! Equality against ok payload: ``ok(v) == v``, ``err(...) != v``.
def operator ==(r : $Result < auto(TT); auto(EE) >; v : TT) : bool {
    return r._is_ok && r._value == v
}

def operator ==(v : auto(TT); r : $Result < TT; auto(EE) >) : bool {
    return r._is_ok && r._value == v
}

def operator !=(r : $Result < auto(TT); auto(EE) >; v : TT) : bool {
    return !(r == v)
}

def operator !=(v : auto(TT); r : $Result < TT; auto(EE) >) : bool {
    return !(r == v)
}
