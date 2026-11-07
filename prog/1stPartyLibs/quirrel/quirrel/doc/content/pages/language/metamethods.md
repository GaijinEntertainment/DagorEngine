---
title: Metamethods
group: Language
order: 57
summary: The seventeen reserved method names the VM calls, and the contract of each.
---

A metamethod is an ordinary method with a reserved name. The VM calls it when an
operation on the object has no built-in meaning: reading a slot that is not there,
adding two instances, printing one. There are seventeen of them. Each has an
exact contract: when it runs, what it receives, and what the VM does with the
value it returns.

## Where a metamethod can live

The VM looks for a metamethod in the object's **delegate**. For an instance the
delegate is its class, so a method named `_add` in a class body is enough.

A table or a userdata can carry metamethods too, but only the host can set this
up. The delegate is set through `sq_setdelegate`, and the script API has no
equivalent. A table built in script has no delegate, so none of its metamethods
can fire, regardless of slot names. Classes and instances are the carriers a
script can build.

This matters most for `_get`: a plain table reports a missing key, and a `_get`
slot in it is inert data.

## All metamethods

{{metamethods}}

`==` and `!=` are not in that table; see
[Comparison](page:language/metamethods#comparison) below.

## Reading and writing missing slots

`_get` and `_set` run only after the normal lookup has failed. They share one
protocol to report that the key is absent:

- **`throw null`** is a clean miss. The VM swallows it, continues the fallback
  chain, and finally raises its own `the index 'x' (type='string') does not exist`.
- **Any other throw** is a real error. It reaches the caller wrapped as
  `Error in '_get' metamethod: <your value>`.
- **Returning `null`** is not a miss. It is the value `null`, successfully found.

So the return value cannot report an absent key, and a throw cannot report a
present one. This lets a proxy distinguish a missing key from a key whose lookup
failed.

{{example:language/metamethods-proxy}}

## Iterating

`_nexti` is asked for keys, not for values. The VM calls it with `null` on the
first step and with the previously returned key after that. Returning `null`
ends the loop. Each returned key is then read back through the ordinary read
path, so an object with a `_nexti` almost always needs a matching `_get`. A key
that cannot be read raises `_nexti returned an invalid idx`.

{{example:language/metamethods-nexti}}

## Arithmetic

The metamethod comes from the left operand and runs with it as `this`. There is
no reversed form. If the left operand is an integer and the right is your
instance, the integer decides, and the call fails with
`arith op - between 'integer' and 'instance'`.

An operator with no metamethod throws; there is no default fallback. An
instance that defines `_add` but not `_sub` cannot be subtracted.

{{example:language/metamethods-operands}}

## Comparison

`_cmp` returns an integer: negative if `this` sorts before `other`, zero if they
are equal, positive if it sorts after. Any other return value raises
`_cmp must return an integer`. It drives `<`, `<=`, `>`, `>=` and `<=>`, and
[sort](sym:types.Array.sort) uses it when no comparator is given.

Two limits:

- **`==` and `!=` do not use it.** They compare raw identity, so two instances with
  the same contents are not equal, and no metamethod can change that. Compare a
  field, or write a named method.
- **Both sides must be the same type.** An instance against an integer raises
  `comparison between instance and '0' (type='integer')` without a call to `_cmp`.

The VM also short-circuits when both sides are the same object, so a comparison
of an object with itself gives zero without a call.

## Calling and cloning

`_call` makes the object callable. Its first parameter is not the first
argument. It is the `this` of the call site, which the VM passes as it does for
every call. The arguments follow it.

`_cloned` runs on the object that `clone` has already produced, with the
original as its argument. Since `clone` is shallow, this is the place to give
the copy its own nested containers.

{{example:language/metamethods-call-cloned}}

## Slot creation and deletion

`_newslot` intercepts `<-`. On an instance it runs, but it cannot complete the
job. Instance storage is a fixed-size block sized when the instance is built, so
the slot still cannot appear. Use it to route the value somewhere else, such as
a side table. On a table it runs only if the table has a delegate and the key is
new; an assignment over an existing key is a plain write.

`_delslot` is unreachable from ordinary script. The `delete` operator is
forbidden by default, and [rawdelete](sym:types.Table.rawdelete), which the
compiler suggests instead, is raw and skips metamethods. A host that clears that
language feature gets `delete` back. A `_delslot` that runs is then responsible
for the removal itself. The VM does not remove the slot, and the metamethod's
return value becomes the value of the `delete` expression.

## Type name and text

`_typeof` changes `typeof`, not [type](sym:type). `typeof obj` gives the string
the metamethod returns, while `type(obj)` still gives `"instance"`, because
`type` ignores metamethods by design. See
[Values and types](page:language/types) for when to use each of the two.

`_tostring` is used when the object is printed, concatenated into a string, or
passed to `sq_tostring` from C. It must return a string. Without it, printing an
instance gives an address.

## _lock

`_lock` is the only metamethod that belongs to the class and not to its
instances. It runs once, when the class locks: at its first instantiation, when
another class inherits it, or on an explicit
[lock()](sym:types.Class.lock), whichever happens first. `this` is the class
object. The class can still be modified at that moment, so this is the last
chance to add a member with [newmember](sym:types.Class.newmember).

See [Classes and instances](page:language/classes) for what locking prevents
afterwards.
