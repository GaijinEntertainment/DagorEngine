local function fibR(n : number)

    if (n < 2) then return n end
    return (fibR(n-2) + fibR(n-1))
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

print(string.format("\"fibonacci recursive\", %.8f, 20", profile_it(20, function () fibR(31) end)))
