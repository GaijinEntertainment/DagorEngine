function isprime(n)
    for i = 2, (n - 1) do
        if (n % i == 0) then
            return false
        end
    end
    return true
end


local function primes(n)
    local count = 0

    for i = 2, n do
        if (isprime(i)) then
            count = count + 1
        end
    end
    return count
end

loadfile("profile.lua")()

io.write(string.format("\"primes loop\", %.8f, 20\n", profile_it(20, function () primes(14000) end)))

