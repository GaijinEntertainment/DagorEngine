function isprime(n : number)
    for i = 2, (n - 1) do
        if (n % i == 0) then
            return false
        end
    end
    return true
end


local function primes(n : number)
    local count = 0
    for i = 2, n do
        if (isprime(i)) then
            count = count + 1
        end
    end
    return count
end

function profile_it(profiles, fn)
  local res
  for i = 1, profiles do
    local start = os.clock()
    fn()
    local measured = os.clock() - start
    if i == 1 or res > measured then res = measured end
  end
  return res
end

print(string.format("\"primes loop\", %.8f, 20", profile_it(20, function () primes(14000) end)))

