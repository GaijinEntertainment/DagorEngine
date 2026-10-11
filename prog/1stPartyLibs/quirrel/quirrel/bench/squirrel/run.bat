@echo off
pushd %~dp0
echo Squirrel 3.2
for %%w in (nbodies nbodies_arrays particles exp dict darg fib_recursive fib_loop primes f2s s2f queen table-sort spectral-norm tree) do sq3-64.exe %%w.nut
popd
