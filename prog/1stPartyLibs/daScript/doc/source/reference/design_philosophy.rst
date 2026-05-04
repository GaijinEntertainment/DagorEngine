.. _design_philosophy:

******************
Design Philosophy
******************

.. index::
    single: design philosophy

This document describes the **why** behind daslang — the problems it was built to solve,
the trade-offs it makes deliberately, and the principles that guide its evolution.

++++++++++++++
Origins
++++++++++++++

daslang was born out of concrete problems at `Gaijin Entertainment <https://gaijin.net/>`_.
The studio had Lua (via LuaJIT) and their own fork of Squirrel integrated into a large C++ game engine.
Both languages hit the same walls:

- **Interop overhead.** Calling back and forth between the scripting language and C++ was so expensive
  that it dominated the frame budget, especially in an Entity Component System (ECS) architecture
  where scripts touch many small pieces of host data every frame.
- **Console performance.** Consoles prohibit JIT and have weaker CPUs. No existing scripting language
  interpreter was fast enough to run gameplay code at acceptable frame rates without JIT.
- **The C++ vs. script dilemma.** Every feature brought the same painful question: write it in C++
  (fast, but hard to iterate) or in script (convenient, but slow)? And when scripts inevitably got slow,
  the only fix was rewriting them in C++ — negating the reason they were scripts in the first place.

A prototype interpreter — tree-based rather than bytecode-based — was already outperforming
LuaJIT's interpreter by a wide margin, simply because it did not marshal data.
"Give it decent syntax and I'll integrate it tomorrow" was the challenge from the engineering side.
daslang shipped its first production integration shortly after.

+++++++++++++++++
Audience
+++++++++++++++++

daslang serves three overlapping groups, in order of population size:

1. **Game scripters** — the largest group. They write the bulk of the codebase but rarely use
   advanced language features. For them, the priorities are hot reload, fast compilation (~5 seconds
   for a full production game), clear error messages, and never needing to rewrite working script into C++
   just because it got slow.

2. **Engine / tools programmers** — they integrate daslang into C++ applications, bind native types
   and functions, and build domain-specific modules. For them, the priorities are zero-cost interop,
   C++-compatible data layout, and a powerful compile-time macro system.

3. **Language enthusiasts and standalone users** — a growing group. They want modules, packages,
   and standalone executables. LLVM-based standalone compilation is now mature enough for this audience.
   Platforms like `edenspark.io <https://edenspark.io/>`_ (currently in early beta) aim to serve this community.

+++++++++++++++++++++++++++++++++++
Core Design Principles
+++++++++++++++++++++++++++++++++++

**Iteration speed is king.**
A full production game recompiles in ~5 seconds on decent hardware. Hot reload is built in.
The entire developer experience revolves around the idea that you should never have to wait.

**Explicit, not implicit.**
daslang does not do work "under the hood". There are no hidden type conversions (not even ``int`` to ``float``),
no silent allocations, no auto-casting outside of type inference.
If the code does not say it, it does not happen. ``options log`` prints the full output of the compiler —
total transparency about what your code becomes. Macros can add behavior; the base language does not.

**99% safe, not 100%.**
The goal is to eliminate 99% of the bugs seen in real-world C++ code — use-after-free, null dereferences,
type mismatches, off-by-one errors — without imposing the full cost of formal verification.
This is not Rust-level safety; it is pragmatic safety for game developers who can handle an occasional crash
but should never be chasing undefined behavior.
Safe code is the default. ``unsafe`` blocks exist for the remaining 1% where you need direct memory access.

**No data marshaling.**
daslang's data layout is essentially identical to C++ (minus bitfields).
Calling from C++ into daslang or from daslang into C++ does not copy, convert, or box data.
This is the single most important performance decision in the language — it is what makes daslang viable
as a per-frame ECS scripting language where Lua and C# are not.

**If it gets slow, you can fix it.**
Garbage collection exists but should rarely run per-frame. Manual ``delete`` is available
to reduce GC pressure when needed. Ahead-of-time compilation (AOT) to C++ is always an option.
The philosophy is: the fast path should be fast by default, and when it isn't, the language gives you tools
to make it fast — deterministically, not hopefully.

**The language should reflect the problem domain.**
The compile-time macro system (inspired by FORTRAN + LISP, with HAXE-style reification) exists not to keep
the language core small, but to let libraries reshape the language to match the problem they solve.
Less verbose solutions are better solutions. Pattern matching, LINQ, JSON — all are implemented as macros
that extend the language rather than burdening the core.

+++++++++++++++++++++++++++++++++++++++
Performance Model
+++++++++++++++++++++++++++++++++++++++

daslang has three execution tiers, all planned from day one:

1. **Interpreter** — tree-based, not bytecode. Fast enough for production use even on consoles.
   Recompiles in seconds, enabling hot reload during development.

2. **AOT (Ahead-of-Time compilation to C++)** — produces code with performance comparable to C++11.
   Required for platforms that prohibit JIT (consoles, iOS). Always available, always at least as fast
   as the interpreter.

3. **JIT (LLVM-based)** — produces native machine code at load time. Fastest tier when available.
   Also enables standalone executable generation.

The **hybrid mode** is the practical sweet spot: each function has a semantic hash. When the script changes
and the hash still matches, the AOT-compiled version is used. When the hash changes, the interpreter takes over
until the next AOT build. This means a December 31st hotfix can ship new script without a full rebuild — if the
semantics didn't change, it stays native; if they did, the interpreter covers it at acceptable speed.

++++++++++++++++++++++++++++++++++
Type System
++++++++++++++++++++++++++++++++++

daslang is statically typed — **stricter than C++**. ``a = 0`` means ``a`` is an ``int``, full stop.
Every expression has a type known at compile time.

Static typing was chosen for two reasons:

- **Performance.** Dynamic typing is fundamentally a runtime cost. Static types let the compiler
  produce tight, unboxed code without runtime type checks.
- **Bug prevention.** Restricting the input space eliminates whole classes of errors before the code runs.

**Aggressive type inference** (``auto``) is a deliberate "best of both worlds" choice. You get the safety
of static typing without the annotation burden of C++. Most intermediate types are irrelevant to the programmer —
the compiler infers them, and ``static_if`` with ``typeinfo`` provides compile-time reflection when you need
to branch on types.

++++++++++++++++++++++++++++++++++++++++++++++++++
Syntax: Gen1 and Gen2
++++++++++++++++++++++++++++++++++++++++++++++++++

daslang has two syntax modes:

- **Gen1** — Python-like significant whitespace. The original syntax.
- **Gen2** — C-like curly braces and mandatory parentheses on control flow. The current default.

Gen2 was created entirely from user feedback. Two distinct groups of programmers prefer different styles.
The intent is to eventually transition fully to gen2 (avoiding the maintenance cost of two parsers),
but gen1 may persist indefinitely. Gen2 also has a simpler parser, which is a practical advantage.

Even in gen2, some significant whitespace remains — for example, automatic semicolon insertion at end-of-line —
so gen1's legacy is not entirely gone.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
AI-Driven Syntax Evolution
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Since GitHub Copilot emerged, daslang's syntax has been actively shaped by AI code generation.
Copilot frequently suggests code that borrows conventions from Python, TypeScript, or C++ — commas instead
of semicolons in argument lists, ``->`` instead of ``:`` for return types, and so on. Rather than rejecting
these suggestions, the parser was made flexible enough to accept them.

For example, both of these are valid daslang::

    def foo(a : int; b : float) : bool { return true; }
    def foo(a : int, b : float) -> bool { return true; }

The first form uses daslang's native separators (``;`` between arguments, ``:`` for return type).
The second uses conventions AI models pick up from other languages (``,`` and ``->``).
Both compile to exactly the same thing.

This is a deliberate design choice: if AI writes code that *looks right*, the language should accept it.
The alternative — fighting the AI — wastes the programmer's time, which violates the core principle
that iteration speed is king.

+++++++++++++++++++++++++++++++++++++++++++++++
Embedding and Beyond
+++++++++++++++++++++++++++++++++++++++++++++++

daslang was born as an embedded scripting language and that remains its strongest use case.
But the language is expanding:

- **Modules and packages.** As the community grows, people want to write, share, and discover modules.
  An NPM-like package manager is in development.
- **Standalone executables.** LLVM integration is mature enough to produce native binaries.
  daslang is no longer "just a scripting language" — hence the rename from daScript to daslang.
- **Releases and distribution.** The build has many prerequisites.
  Making daslang easy to download and run is an active priority.

++++++++++++++++++++++++++++++++++++++++++++++++++++
The Rename: daScript → daslang
++++++++++++++++++++++++++++++++++++++++++++++++++++

The original name "daScript" caused people to assume it was a scripting language in the traditional sense —
interpreted, dynamically typed, slow. It is none of those things. The rename to **daslang** signals
that this is a full programming language: statically typed, AOT-compiled, with JIT and standalone binary support.

As the `blog post <https://borisbat.github.io/dascf-blog/2023/12/28/daslang-it-is/>`_ put it:
"It's not a script, so renaming it was a matter of time."

++++++++++++++++++++++++++++++++++++++++++++++++
Competitive Landscape
++++++++++++++++++++++++++++++++++++++++++++++++

daslang occupies a unique position among game-adjacent languages:

- **Lua / LuaJIT** — interop overhead kills performance in ECS-heavy architectures.
- **C# (Mono / IL2CPP)** — has interop, but the interpreter is slow.
  Unity's Burst compiler makes C# viable for hot paths, but it is not open source and has limitations.
- **Zig / Odin** — interesting systems languages, but lack daslang's iteration speed
  (seconds, not minutes, to recompile a production game).
- **daslang** — the combination of C++ data layout, near-zero interop cost, scripting-speed iteration,
  and embeddability is unique. No other language in this space offers all four simultaneously.

+++++++++++++++++++++++++++++
Summary
+++++++++++++++++++++++++++++

daslang is a pragmatic language built by game developers for game developers.
It trades theoretical purity for practical speed — in compilation, in execution, and in the
daily workflow of the people who use it. Every design decision traces back to a real problem
encountered in production game development at scale.
