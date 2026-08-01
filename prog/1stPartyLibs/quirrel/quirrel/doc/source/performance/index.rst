Performance
------------


As Bob Nystrom said: "Even though most benchmarks aren't worth the pixels they're printed on, people seem to like them, so here's a few:"


Benchmarks
===========

.. include::  results.rst

|

**Shorter bars are better**. Each benchmark is run ten or twenty times and the best time is kept.
It only measures the time taken to execute the benchmarked code itself,
not interpreter startup or script compilation.


Notes & Builds
==============

Everything was built for Window 64bit and with clang-cl (if possible).
All benchmark sources, harnesses and prebuilt third-party interpreter binaries
live in the ``bench/`` folder at the repository root; this page only keeps the
committed results. The Quirrel rows are measured on the release csq built from
this repository (the shipped runtime configuration, mimalloc allocator).
LuaJIT is measured without JIT enabled, cause Quirrel, Lua, QuickJS are made for platforms where JIT-compilation is disallowed
(consoles, phones, other hardware of the type).
Also, check https://daslang.io/#performance for more benchmarks of this kind.


If you need fastest interpreter and\or AoT language your choice should be Daslang.
However Quirrel, Lua and even JS are highly dynamic lagnuages, and much simpler to learn and master;
So, we are comparing only them.


**Lua is 5.4.6 built** from sources

This is **LuaJIT2.1.0Beta3** with only one diff (to allow detailed profiling)
Built with LLVM8.0 (clang-cl)

.. code::

  diff --git a/src/lib_os.c b/src/lib_os.c
  index ffbc3fd..145ebd4 100644
  --- a/src/lib_os.c
  +++ b/src/lib_os.c
  @@ -122,11 +122,27 @@ LJLIB_CF(os_exit)
     return 0;  /* Unreachable. */
   }
   
  +#include <windows.h>
  +
  +LARGE_INTEGER profileGetTime () {
  +    LARGE_INTEGER  t0;
  +    QueryPerformanceCounter(&t0);
  +    return t0;
  +}
  +
  +double profileGetTimeSec ( LARGE_INTEGER minT ) {
  +    LARGE_INTEGER freq;
  +    QueryPerformanceFrequency(&freq);
  +    return ((double)minT.QuadPart) / ((double)freq.QuadPart);
  +}
  +
   LJLIB_CF(os_clock)
   {
  -  setnumV(L->top++, ((lua_Number)clock())*(1.0/(lua_Number)CLOCKS_PER_SEC));
  -  return 1;
  +//setnumV(L->top++, ((lua_Number)clock())*(1.0/(lua_Number)CLOCKS_PER_SEC));
  +setnumV(L->top++, (lua_Number)profileGetTimeSec(profileGetTime()));
  +return 1;
   }
  +__declspec(dllexport) int addOne(int a) {return a+1;}
   
   /* ------------------------------------------------------------------------ */
 

VM acceptance benchmark
=======================

``bench/run_vm_bench.py`` is the head-to-head harness used for interpreter
work (run it before and after any VM change): Quirrel release csq vs
Lua 5.4.6 and Luau (-O2) on paired workloads with identical algorithms.
Committed baseline (Quirrel 4.35.1, AMD Ryzen Threadripper 3970X,
Windows 10 x64, 2026-07-18; ms, median of best rep over 5 process runs;
ratio is relative to Quirrel, above 1.0 means Quirrel is slower), raw data
in ``vm_bench_results.json``:

.. code::

             bench               quirrel                 lua54                  luau
               fib                127.00       65.00 ( 1.95x)       56.34 ( 2.25x)
       binarytrees                 36.00       52.00 ( 0.69x)       16.48 ( 2.18x)
              life                150.00       85.00 ( 1.76x)       72.37 ( 2.07x)
            mandel                 83.00       43.00 ( 1.93x)       32.16 ( 2.58x)
           strings                 98.00       88.00 ( 1.11x)       78.62 ( 1.25x)
        desc_churn                137.00      222.00 ( 0.62x)       67.77 ( 2.02x)
       probe_storm                106.00       97.00 ( 1.09x)       65.03 ( 1.63x)
    nullable_probe                118.00       38.00 ( 3.11x)       31.92 ( 3.70x)
     closure_storm                140.00      240.00 ( 0.58x)       34.15 ( 4.10x)
      method_calls                564.00      369.00 ( 1.53x)      226.95 ( 2.49x)

Geomean: Lua 5.4 ~1.25x, Luau interpreter ~2.3x over release Quirrel;
Quirrel is faster on the allocation-churn rows (binarytrees, desc_churn,
closure_storm vs Lua 5.4).

Sources
=======

All benchmark sources live in ``bench/`` at the repository root, one folder per
language (``bench/quirrel``, ``bench/lua``, ``bench/luau``, ``bench/js``), next
to the prebuilt interpreter binaries and the two harnesses:

- ``bench/benchmarks.py`` - the cross-language suite that produces the charts
  above; it writes ``results.json`` / ``results.rst`` into this documentation
  folder.
- ``bench/run_vm_bench.py`` - the VM acceptance harness (Quirrel vs Lua 5.4 /
  Luau, ratio table); run it before and after interpreter changes.

See ``bench/README.md`` for how to run both.