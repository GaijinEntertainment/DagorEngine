---
title: Classes and instances
group: Language
order: 55
summary: Class bodies, instances, inheritance and static members.
---

A class is a value like any other. It can be stored in a variable, passed to
a function, and kept in a table or array. It describes the fields and
methods of every instance made from it.

## Declaring a class

`class Name { ... }` declares fields with a default value and methods,
either as `function name() { ... }` or as a field assigned a function or a
lambda. The method named `constructor` runs for every new instance. In a
class body it can also be written without `function`, as
`constructor(x) { ... }`. The word is not a keyword: elsewhere it is an
ordinary name. Inside a method, a field is always reached through
`this.`. Unlike some other object-oriented languages, a bare name does not
mean `this.name`.

{{example:language/classes-basics}}

## Inheritance and base

A derived class copies every field, static and method from its base first,
then applies its own body over that: `class Tank(Vehicle) { ... }`. `base`
reaches the shadowed implementation from inside an override, most often
`base.constructor(...)` or `base.someMethod(...)`. There is no `super`
keyword. `super` is an unknown variable and fails to compile.

A derived class that declares no `constructor` of its own inherits the
base's, called with the arguments the instantiation call passes.
[`getbase()`](sym:types.Class.getbase) returns the class given in the
parentheses, or `null` for a class with none.

{{example:language/classes-inheritance}}

There is no `extends` keyword: `class Tank extends Vehicle` fails with
`expected '{'`. Use the parenthesized form above.

## Static members

`static name = value`, written inside the class body, is one value shared by
every instance, not a per-instance default. A static is read-only after
declaration. `ClassName.name = value` throws `trying to set 'class'`.
`this.name = value` from a method throws `the index 'name' (type='string')
does not exist`, because a static is not stored per instance. The only way
to add or replace a static after the class exists is
[`newmember`](sym:types.Class.newmember) with its third argument set to
`true`.

## Instantiation

`ClassName(args)` creates an instance, copies the class's field defaults into
it, and runs `constructor` if one is declared. The defaults are copied as
they are, not cloned. If a default is an array or a table and no constructor
replaces it, every instance that keeps that default shares the same array.
Give a mutable default its own value inside `constructor` when each instance
needs an independent one. [`instance()`](sym:types.Class.instance) makes an
instance the same way, without running `constructor`.

Every instance remembers the class it was made from.
[`getclass()`](sym:types.Instance.getclass) returns that class, never a base
class, even for a class made with `class Tank(Vehicle) { ... }`.

## instanceof

`x instanceof ClassName` walks `x`'s class and its bases. It does not throw
when `x` is not an instance: `5 instanceof Squad` is a legal `false`. It
does throw when the right-hand side is not a class value: `squad instanceof
5` throws `cannot apply instanceof between a integer and a instance` (the
message names the right operand's type first, then the left's). An
instance's `instanceof` against the built-in placeholder
[`Instance`](sym:types.Instance) class from the `types` module is always
`false`, regardless of the class that made it. See
[Values and types](page:language/types) for the reason, and for the
`type(x) == "instance"` check that tests whether a value is any instance.

## Locking

A class locks permanently the first time it gains an instance, becomes the
base of another class, or has [`lock()`](sym:types.Class.lock) called on it
directly, whichever happens first. After the lock, a new plain field cannot
be added: `ClassName.newField <- value` throws `trying to modify a class
that has already been instantiated, inherited or is locked manually`. The
lock does not stop a method from being added or replaced the same way, since
`<-` with a function value bypasses the lock check. It also does not stop
[`newmember`](sym:types.Class.newmember) from adding a new **static**, when
its third argument asks for one.

## Metamethods

A class can define any of the seventeen metamethods the VM recognizes
(`_add _sub _mul _div _unm _modulo`, `_set _get _newslot _delslot`, `_typeof
_cmp _nexti _call _cloned _tostring`, and `_lock`) as an ordinary method
with that name. [`getmetamethod`](sym:types.Class.getmetamethod) looks one
up by name without calling it. All of them run with an instance as `this`
except `_lock`. `_lock` runs once, at the moment the class itself locks, with
the class object as `this`. It is the only metamethod that never fires on an
instance.

[Metamethods](page:language/metamethods) gives the contract of each one:
when the VM calls it, what it receives, and what it must return or throw.

{{example:language/classes-metamethods}}

## Edge cases

- A class body is not table syntax. A quoted-string key (`"key": value`),
  the bare-name shorthand (`{ name }` for `name = name`) and a
  [spread](page:language/containers#spread) (`...src`) all parse in a
  [table](page:language/containers) literal but fail to compile in a class
  body, which requires `identifier = value` for every field.
- `clone` works on an instance (and runs `_cloned` if the class defines it)
  but not on the class object itself: `clone SomeClass` throws `cloning a
  class`.
- `_newslot` lets a class intercept `<-` on its own instances, which
  normally throws `class instances do not support the new slot operator`.
  The instance still cannot gain the slot. An instance's storage is a
  fixed-size block sized at construction, so even `this.rawset(key, val)`
  called from inside the metamethod throws `the index '<key>'
  (type='<type>') does not exist`. Store the extra data somewhere else, such
  as a side table.
