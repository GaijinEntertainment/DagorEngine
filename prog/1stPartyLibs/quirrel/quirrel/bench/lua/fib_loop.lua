local function fibI(n)
    local last = 0
    local cur = 1
    n = n - 1
    while (n > 0)
    do
        n = n - 1
        local tmp = cur
        cur = last + cur
        last = tmp
    end
    return cur
end
loadfile("profile.lua")()

io.write(string.format("\"fibonacci loop\", %.8f, 20\n", profile_it(20, function () fibI(6511134) end)))

