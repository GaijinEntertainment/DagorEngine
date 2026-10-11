@echo off
pushd %~dp0
for %%o in (-O0 -O1 -O2) do (
  echo Luau %%o
  for %%w in (nbodies nbodies_arrays particles exp dict darg fib_recursive fib_loop primes f2s s2f queen table-sort spectral-norm tree) do luau.exe %%o %%w.luau
)
popd
