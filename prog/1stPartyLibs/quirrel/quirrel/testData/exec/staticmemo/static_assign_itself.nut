local x = 333
x = static x
println(x)

local y = {a = 444}
y = static y
println(y.a)

local z = [555, 666]
z = static z
println(z[1])

local w = [777, 888]
w = static w
w = static w
println(w[1])

local a = 111
a = static a
a = static a
println(a)

