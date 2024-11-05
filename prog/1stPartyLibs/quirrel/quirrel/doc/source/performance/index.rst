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
All code for tests and binaries of interpreters are included in github source folder of documentation
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
 

Sources
=======


Quirrel & Squirrel: :download:`nbodies.nut <tests/quirrel/nbodies.nut>`
:download:`dict.nut <tests/quirrel/dict.nut>` 
:download:`exp.nut <tests/quirrel/exp.nut>` 
:download:`fib_loop.nut <tests/quirrel/fib_loop.nut>` 
:download:`fib_recursive.nut <tests/quirrel/fib_recursive.nut>` 
:download:`nbodies.nut <tests/quirrel/nbodies.nut>` 
:download:`particles.nut <tests/quirrel/particles.nut>` 
:download:`primes.nut <tests/quirrel/primes.nut>` 
:download:`darg.nut <tests/quirrel/darg.nut>`
:download:`f2i.nut <tests/quirrel/f2i.nut>`
:download:`f2s.nut <tests/quirrel/f2s.nut>`
:download:`particles_array.nut <tests/quirrel/particles_array.nut>`
:download:`profile_try_catch.nut <tests/quirrel/profile_try_catch.nut>`
:download:`queen.nut <tests/quirrel/queen.nut>`
:download:`spectral-norm.nut <tests/quirrel/spectral-norm.nut>`
:download:`table-sort.nut <tests/quirrel/table-sort.nut>`

Lua: :download:`dict.lua <tests/lua/dict.lua>`
:download:`exp.lua <tests/lua/exp.lua>`
:download:`fib_loop.lua <tests/lua/fib_loop.lua>`
:download:`fib_recursive.lua <tests/lua/fib_recursive.lua>`
:download:`nbodies.lua <tests/lua/nbodies.lua>`
:download:`particles.lua <tests/lua/particles.lua>`
:download:`primes.lua <tests/lua/primes.lua>`
:download:`darg.lua <tests/lua/darg.lua>`
:download:`f2i.lua <tests/lua/f2i.lua>`
:download:`f2s.lua <tests/lua/f2s.lua>`
:download:`profile_try_catch.lua <tests/lua/profile_try_catch.lua>`
:download:`queen.lua <tests/lua/queen.lua>`
:download:`spectral-norm.lua <tests/lua/spectral-norm.lua>`
:download:`table-sort.lua <tests/lua/table-sort.lua>`

JavaScript:  :download:`dict.js <tests/js/dict.js>`
:download:`exp.js <tests/js/exp.js>`
:download:`fib_loop.js <tests/js/fib_loop.js>`
:download:`fib_recursive.js <tests/js/fib_recursive.js>`
:download:`nbodies.js <tests/js/nbodies.js>`
:download:`particles.js <tests/js/particles.js>`
:download:`primes.js <tests/js/primes.js>`
:download:`darg.js <tests/js/darg.js>`
:download:`f2i.js <tests/js/f2i.js>`
:download:`f2s.js <tests/js/f2s.js>`
:download:`spectral-norm.js <tests/js/spectral-norm.js>`

Luau:
:download:`darg.luau <tests/luau/darg.luau>`
:download:`dict.luau <tests/luau/dict.luau>`
:download:`exp.luau <tests/luau/exp.luau>`
:download:`f2i.luau <tests/luau/f2i.luau>`
:download:`f2s.luau <tests/luau/f2s.luau>`
:download:`fib_loop.luau <tests/luau/fib_loop.luau>`
:download:`fib_recursive.luau <tests/luau/fib_recursive.luau>`
:download:`nbodies.luau <tests/luau/nbodies.luau>`
:download:`particles.luau <tests/luau/particles.luau>`
:download:`primes.luau <tests/luau/primes.luau>`
:download:`queen.luau <tests/luau/queen.luau>`
:download:`spectral-norm.luau <tests/luau/spectral-norm.luau>`
:download:`table-sort.luau <tests/luau/table-sort.luau>`