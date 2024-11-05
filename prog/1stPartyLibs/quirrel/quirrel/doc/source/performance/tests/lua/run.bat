echo "LuaJIT, no JIT"
luajit.exe -joff fib_loop.lua
luajit.exe -joff fib_recursive.lua
luajit.exe -joff primes.lua
luajit.exe -joff particles.lua
luajit.exe -joff dict.lua 
luajit.exe -joff profile_try_catch.lua 
luajit.exe -joff exp.lua 
luajit.exe -joff nbodies.lua
luajit.exe -joff f2i.lua
luajit.exe -joff f2s.lua
luajit.exe -joff queen.lua
luajit.exe -joff spectral-norm.lua
luajit.exe -joff table-sort.lua
luajit.exe -joff tree.lua

echo "Lua (low res timer)"
lua.exe fib_loop.lua
lua.exe fib_recursive.lua
lua.exe primes.lua
lua.exe particles.lua
lua.exe dict.lua 
lua.exe profile_try_catch.lua 
lua.exe exp.lua 
lua.exe nbodies.lua
lua.exe f2i.lua
lua.exe f2s.lua
lua.exe queen.lua
lua.exe spectral-norm.lua
lua.exe table-sort.lua
lua.exe tree.lua