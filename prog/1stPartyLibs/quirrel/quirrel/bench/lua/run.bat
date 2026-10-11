@echo off
pushd %~dp0
echo Lua 5.5
for %%w in (nbodies nbodies_arrays particles exp dict darg fib_recursive fib_loop primes f2s s2f queen table-sort spectral-norm tree) do lua.exe %%w.lua
popd
