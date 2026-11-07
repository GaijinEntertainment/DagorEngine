---
title: Compiling and calling
group: Embedding
order: 104
summary: Compiling a source buffer, and calling the closure it produces.
---

## Compiling

`sq_compile` turns a source buffer into a closure and pushes it:

```cpp
SQRESULT sq_compile(HSQUIRRELVM v, const char *s, SQInteger size,
                    const char *sourcename, SQBool raiseerror,
                    const HSQOBJECT *bindings = nullptr);
```

`sourcename` is the name that appears in a runtime error and a stack trace. Use
the path a reader can open, even when the source came from an archive.
`bindings` supplies compile-time names. A host uses it to make constants and
imported modules visible to the unit being compiled.

Compiling does not run the top-level statements. It can run a pure function:
the compiler calls a pure function with constant arguments to fold the call,
and an error in that call fails the compile; see "Folding outside const" in
[Bindings and constants](page:language/bindings). The closure left on the
stack must be called like any other function. The call runs the top-level
statements of the file.

A syntax error goes to the compiler error handler, which is set once per VM:

```cpp
typedef void (*SQCOMPILERERROR)(HSQUIRRELVM v, SQMessageSeverity severity,
                                const char *desc, const char *source,
                                SQInteger line, SQInteger column,
                                const char *extra_info);

sq_setcompilererrorhandler(v, my_handler);
```

The severity separates an error from an analyzer warning, so one handler covers
both. `extra_info` carries the diagnostic's own context where it has any.
`sqstd_seterrorhandlers` installs a handler that prints all of this. It is
enough for most hosts, and it is the fastest way to stop losing errors silently.

## Calling

A call is assembled on the stack: the closure, then its `this`, then the
arguments. Then call `sq_call` with the total count, including `this`.

```cpp
sq_pushroottable(v);
sq_pushstring(v, "foo", -1);
sq_get(v, -2);              // the closure

sq_pushroottable(v);        // 'this' for the call
sq_pushinteger(v, 1);
sq_pushfloat(v, 2.0);
sq_pushstring(v, "three", -1);

sq_call(v, 4, SQFalse, SQFalse);
sq_pop(v, 2);               // the closure and the root table
```

That is `foo(1, 2.0, "three")`. The count is 4 because `this` is one of the
values. Check this first when a call reports the wrong arity.

`sq_call` pops the arguments and the closure's `this`. It pushes the return
value only when the `retval` argument asks for it. The last argument decides
whether a failure runs the VM error handler on the way out. Pass false when the
caller reports the failure itself, or the error is printed twice.

If the script throws and nothing catches it, `sq_call` fails. Read the thrown
value with `sq_getlasterror`.

## Tail calls

`sq_tailcall` replaces the current native frame with the call instead of nesting
it. A native that dispatches to a script function uses it so that the stack does
not grow.
