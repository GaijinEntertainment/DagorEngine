echo off
pushd quirrel
echo "Quirrel latest"
csq.exe --version
csq.exe version.nut
echo "----"
csq.exe fib_loop.nut
csq.exe fib_recursive.nut
csq.exe primes.nut
csq.exe particles.nut
csq.exe dict.nut
csq.exe exp.nut
csq.exe nbodies.nut
rem csq.exe native.nut
rem csq.exe profile_try_catch.nut 
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