local function expLoop(n)

    local sum = 0
    for i = 0, n do
      sum = sum + math.exp(1./(1.+i))
    end
    return sum
end
loadfile("profile.lua")()

io.write(string.format("\"exp loop\", %.8f, 20\n", profile_it(20, function () expLoop(1000000) end)))
