echo off
pushd quirrel
echo "----"
echo "Quirrel latest (7.4.1)"
sq-64.exe fib_recursive.nut
sq-64.exe fib_loop.nut
sq-64.exe primes.nut
sq-64.exe particles.nut
sq-64.exe dict.nut
sq-64.exe exp.nut
sq-64.exe nbodies.nut
rem sq3-latest-64.exe native.nut
rem sq3-latest-64.exe profile_try_catch.nut 

echo "----"
echo "Squirrel3 original (3.1)"
sq3-64.exe fib_recursive.nut
sq3-64.exe fib_loop.nut
sq3-64.exe primes.nut
sq3-64.exe particles.nut
sq3-64.exe dict.nut
sq3-64.exe exp.nut
sq3-64.exe nbodies.nut
popd

pushd lua
echo ""echo "----"
echo "LuaJIT2.1.0Beta -joff"
luajit.exe -joff fib_loop.lua
luajit.exe -joff fib_recursive.lua
luajit.exe -joff primes.lua
luajit.exe -joff particles.lua
luajit.exe -joff dict.lua 
rem luajit.exe -joff profile_try_catch.lua 
luajit.exe -joff exp.lua 
luajit.exe -joff nbodies.lua

echo "----"
echo "Lua 5.4.6 (low res timer)"
lua.exe fib_loop.lua
lua.exe fib_recursive.lua
lua.exe primes.lua
lua.exe particles.lua
lua.exe dict.lua 
rem lua.exe profile_try_catch.lua 
lua.exe exp.lua 
lua.exe nbodies.lua
popd

pushd js
echo "----"
echo "QuickJS"
qjs.exe fib_loop.js
qjs.exe fib_recursive.js
qjs.exe primes.js
qjs.exe particles.js
qjs.exe dict.js
qjs.exe exp.js
qjs.exe nbodies.js
popd