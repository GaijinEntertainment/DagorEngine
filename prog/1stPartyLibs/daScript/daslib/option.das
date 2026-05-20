options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module option shared public

require daslib/templates
require daslib/templates_boost
require daslib/typemacro_boost

//! Monadic ``Option<T>`` type — represents a value that may or may not be present.
//!
//! Functional API for modelling optional value. The payload may be any type,
//! including non-copyable ones (``array<T>``, ``table<K;V>``, lambdas):
//! constructors and combinators clone or move on the non-copyable branch via
//! ``static_if (typeinfo can_copy(...))``.
//!
//! Construct via ``some(v)`` (clones the payload), ``move_some(v)`` (moves out
//! of a mutable source), or ``none(type<T>)``.
//!
//! See also ``daslib/result`` for a ``Result<T, E>`` counterpart.

//! Tagged ``some``/``none`` pair over payload type ``T``.
//!
//! When ``_has_value`` is ``false``, ``value`` holds the zero value of ``T`` and
//! must not be read. Accessor functions should be used for reads/writes.
[template_structure(T), safe_when_uninitialized]
struct template Option {
    _has_value : bool = false
    @safe_when_uninitialized
    _value : T
}

//! Construct ``Option<T>`` carrying ``v``. Payload type is inferred from ``v``.
//! For non-copyable ``T`` the payload is cloned; if you want to move out of a
//! mutable source, use ``move_some`` instead.
def some(v : auto(TT)) {
    typedef OT = $Option < TT - const >
    var o = default<OT>
    o._has_value = true
    static_if (typeinfo can_copy(o._value)) {
        o._value = v
    } else {
        o._value <- clone_to_move(v)
    }
    return <- o
}


//! Construct ``Option<T>`` by moving the payload out of a mutable source.
//! After ``move_some(src)`` the source is left empty (``length(src) == 0``
//! for arrays/tables). Useful for non-copyable payloads where ``some`` would
//! clone; for workhorse types (``int``, ``float``, ``string``, …) ``some`` is
//! equivalent and cheaper to read.
def move_some(var v : auto(TT)) {
    typedef OT = $Option < TT - const >
    return <- OT(_has_value = true, _value <- v)
}

//! Construct an empty ``Option<T>``. Pass the payload type as a witness::
//!
//!     let o = none(type<int>)
[template(t)]
def none(t : auto(TT)) {
    return default<$Option<TT -const> >
}

//! ``true`` iff the option carries a value.
def is_some(o : $Option < auto(TT) >) : bool {
    return o._has_value
}

//! ``true`` iff the option is empty.
def is_none(o : $Option < auto(TT) >) : bool {
    return !o._has_value
}

//! Apply ``f`` to the payload, producing ``Option<U>``. ``none`` stays ``none``.
def map(o : $Option < auto(TT) >; f : block<(v : TT) : auto(UU)>) {
    var r = default<$Option<UU -const> >
    if (o._has_value) {
        r._has_value = true
        static_if (typeinfo can_copy(r._value)) {
            r._value = invoke(f, o._value)
        } else {
            r._value <- invoke(f, o._value)
        }
    }
    return <- r
}

//! Monadic bind — ``f`` returns the next ``Option``; chains only when ``some``.
def and_then(o : $Option < auto(TT) >; f : block<(v : TT) : $Option<auto(UU)> >) {
    if (o._has_value) {
        return invoke(f, o._value)
    }
    return default<$Option<UU -const> >
}

//! Keep the payload only if ``p`` holds; otherwise turn ``some`` into ``none``.
def filter(o : $Option < auto(TT) >; p : block<(v : TT) : bool>) {
    if (o._has_value && invoke(p, o._value)) {
        static_if (typeinfo can_copy(o._value)) {
            return o
        } else {
            return <- clone_to_move(o)
        }
    }
    return default<$Option<TT -const> >
}

//! Fallback to ``f()`` when ``none``; ``some`` passes through unchanged.
def or_else(o : $Option < auto(TT) >; f : block<() : $Option<TT> >) {
    if (o._has_value) {
        static_if (typeinfo can_copy(o._value)) {
            return o
        } else {
            return <- clone_to_move(o)
        }
    }
    return invoke(f)
}

//! Fallback to eager ``v`` when ``none``; ``some`` passes through unchanged.
def or_value(o : $Option < auto(TT) >; v : TT) {
    if (o._has_value) {
        static_if (typeinfo can_copy(o._value)) {
            return o
        } else {
            return <- clone_to_move(o)
        }
    }
    return some(v)
}

//! Extract the payload or panic when ``none``.
def unwrap(o : $Option < auto(TT) >) : TT {
    if (!o._has_value) {
        panic("unwrap called on none")
    }
    static_if (typeinfo can_copy(o._value)) {
        return o._value
    } else {
        return <- clone_to_move(o._value)
    }
}

//! Destructively extract the payload, leaving ``o`` empty. Mirrors ``move_some``
//! on the construction side. Use this instead of ``unwrap`` when ``T`` is
//! non-copyable (``array<T>``, ``table<K;V>``, lambdas) and you no longer need
//! ``o`` — it avoids the deep clone that ``unwrap`` would otherwise perform.
//! Panics when ``none``.
def move_unwrap(var o : $Option < auto(TT) >) : TT {
    if (!o._has_value) {
        panic("move_unwrap called on none")
    }
    o._has_value = false
    return <- o._value
}

//! Extract the payload or return ``d`` when ``none``.
def unwrap_or(o : $Option < auto(TT) >; d : TT) : TT {
    static_if (typeinfo can_copy(o._value)) {
        return o._has_value ? o._value : d
    } else {
        return <- clone_to_move(o._has_value ? o._value : d)
    }
}

//! Extract the payload or compute a fallback via ``f`` when ``none``.
def unwrap_or_else(o : $Option < auto(TT) >; f : block<() : TT>) : TT {
    static_if (typeinfo can_copy(o._value)) {
        return o._has_value ? o._value : invoke(f)
    } else {
        return <- clone_to_move(o._has_value ? o._value : invoke(f))
    }
}

//! Extract the payload or return ``default<T>`` when ``none``.
def unwrap_or_default(o : $Option < auto(TT) >) : TT {
    static_if (typeinfo can_copy(o._value)) {
        return o._has_value ? o._value : default<TT>
    } else {
        return <- clone_to_move(o._has_value ? o._value : default<TT>)
    }
}

//! Extract the payload or panic with ``msg`` when ``none``. ``expect_value``
//! rather than ``expect`` because ``expect`` is a reserved keyword in daslang.
def expect_value(o : $Option < auto(TT) >; msg : string) : TT {
    if (!o._has_value) {
        panic(msg)
    }
    static_if (typeinfo can_copy(o._value)) {
        return o._value
    } else {
        return <- clone_to_move(o._value)
    }
}

//! Invoke ``f`` with the payload when ``some``; no-op when ``none``.
def if_some(o : $Option < auto(TT) >; f : block<(v : TT) : void>) : void {
    if (o._has_value) {
        invoke(f, o._value)
    }
}

//! Invoke ``f`` when ``none``; no-op when ``some``.
def if_none(o : $Option < auto(TT) >; f : block<() : void>) : void {
    if (!o._has_value) {
        invoke(f)
    }
}

//! Pair two options; ``some`` iff both are ``some``.
def zip(a : $Option < auto(TT) >; b : $Option < auto(UU) >) {
    var r = default<$Option<tuple<TT -const; UU -const> > >
    if (a._has_value && b._has_value) {
        r._has_value = true
        static_if (typeinfo can_copy(a._value)) {
            r._value._0 = a._value
        } else {
            r._value._0 := a._value
        }
        static_if (typeinfo can_copy(b._value)) {
            r._value._1 = b._value
        } else {
            r._value._1 := b._value
        }
    }
    return <- r
}

//! Unwrap or default. Mirrors ``T? ?? default`` for value types.
def operator ??(o : $Option < auto(TT) >; d : TT) : TT {
    static_if (typeinfo can_copy(o._value)) {
        return o._has_value ? o._value : d
    } else {
        return <- clone_to_move(o._has_value ? o._value : d)
    }
}

//! Structural equality: same tag, and when ``some``, equal payloads.
def operator ==(a, b : $Option < auto(TT) > implicit) : bool {
    if (a._has_value != b._has_value) {
        return false
    }
    return !a._has_value || a._value == b._value
}

//! Equality against payload value: ``some(v) == v``, ``none != v``.
def operator ==(o : $Option < auto(TT) >; v : TT) : bool {
    return o._has_value && o._value == v
}

def operator ==(v : auto(TT); o : $Option < TT >) : bool {
    return o._has_value && o._value == v
}

def operator !=(o : $Option < auto(TT) >; v : TT) : bool {
    return !(o == v)
}

def operator !=(v : auto(TT); o : $Option < TT >) : bool {
    return !(o == v)
}
