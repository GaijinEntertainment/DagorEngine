# src/ast architecture notes - inference

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 5. Overloads of `=`, `<-` and `:=` (`ast_infer_type_op.cpp`, `ast_infer_type_function.cpp`) {#assign-operator-lookup}

An `ExprCopy` or `ExprMove` whose `no_promotion` flag is clear first looks for a user function
named `=` or `<-` whose two parameters take the left type as a mutable reference and the right
type, by the ordinary overload rules (`inferAssignOperator`); a match replaces the node with a
call to it, and the built-in operation runs only when there is none. An `ExprClone` runs the
same lookup for `clone`, the name `operator :=` declares. Each lookup starts with the module
library's cached name probe (`hasFunctionOrGenericNamed`), so a program that declares no such
operator pays no temporary call per copy or move, and it does not run while either operand's
type is still unresolved, void, or an expression type.

The raw forms `!==`, `!<-` and `!:=` set the flag, and a rewrite of a flagged node - the
relaxed-assign move, the in-scope POD move, the `ExprCopy` the built-in clone of a copyable
type lowers to - carries it over. A raw `!:=` still honors a clone the language itself
provides: a C++-bound `clone`, a generic of the builtin module, a generated field-wise clone.
It skips only a user-written `clone` on the pair; when such a function exists for a
non-copyable type it has replaced the generated clone, nothing else can clone the type, and
the raw form is reported as an error instead of re-entering the overload
(`userCloneReplacesBuiltin`).

A copy or move the compiler manufactures carries the flag: the inliner's return store and its
split-ternary arm stores, a `let` the stack allocator relocates into an assignment, a
generator's yield and loop-finally return stores, loop counters and control flags, a
generator-local store whose initializer the compiler already promoted, and the copy into a
lambda or generator capture (`visitMakeStructureField` skips a generated value). A generated
variable's initializer is never promoted, and neither is an initializer that reads a generated
variable - the inliner's result temp, which stands where a promoted call already ran the
operator; the helpers are `[inline]`, so in a function body the init reads that temp after
inlining, while a global or field-default initializer, which no inliner visits, keeps the call:
the lint that refuses an `[inline]` call there exempts a call marked `generated`. The
generated field-wise clone of a struct, tuple or
variant clones each field with `:=`, which reaches the field type's own operators. Code a macro
emits is user code: nothing marks a `qmacro`-built store as compiler-made.

Initialization - a local `let` or `var`, typed or `auto`, a global, a struct or class field
default, a field in a make-struct or `new` - runs `promoteInitToAssign`; an argument default, a
`return` value, and an element of an array, tuple or variant literal are not init sites and copy
with the built-in operation. When an overload exists for the declared type and the
initializer's type, a plain function or a generic, the initializer becomes a call to a builtin
generic (`daslib/builtin.das`): `copy_to_move(init, type<T>)` for `=` and `move_to_move` for
`<-`, the `_ref` variants when the initializer is a variable so a scalar, pointer or string
source reaches the operator by reference, and the const or `var` overload of each by the
source's constness. Each helper is a fresh `T` assigned through the overload and moved out.
Two plain candidates select none, and the built-in operation runs, as an assignment's
`tryOperator` lookup does; among generics the most specialized one wins by the same
`copmareFunctionSpecialization` ordering an assignment uses, and a tie selects none. An
operator marked `[unsafe_operation]` is reported at the initialization site outside an
`unsafe` block, as the call would be at an assignment, because the helper's own `unsafe`
block would otherwise hide it. The lookup drops the declared constness, because an
initialization writes its destination whatever the declaration says. The variable is then
move-initialized only when `T` cannot be copied: a workhorse call result is a constant value,
and a copyable type never move-initializes from a constant. `isPromotedInitCall`
(`ast_generate.cpp`) names the helpers - `clone_to_move` and `clone_string` included - so an
initializer the compiler already promoted is not promoted again on the next pass, and a
generator-local store of such an initializer is raw; it matches names, not the `generated`
mark, because a table read with field defaults is a generated call too and its initializer
still reaches the operator.
