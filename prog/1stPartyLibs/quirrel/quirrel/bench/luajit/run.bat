@echo off
pushd %~dp0
echo LuaJIT 2.1, JIT off
for %%w in (nbodies nbodies_arrays particles exp dict darg fib_recursive fib_loop primes f2s s2f queen table-sort spectral-norm tree) do luajit.exe -joff %%w.lua
popd
