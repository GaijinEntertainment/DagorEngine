local function fibR(n)

    if (n < 2) then return n end
    return (fibR(n-2) + fibR(n-1))
end

loadfile("profile.lua")()

io.write(string.format("\"fibonacci recursive\", %.8f, 20\n", profile_it(20, function () fibR(31) end)))
