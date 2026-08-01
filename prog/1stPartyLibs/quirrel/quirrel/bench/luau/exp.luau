local function expLoop(n:number)
    local sum : number = 0
    for i = 0, n do
      sum = sum + math.exp(1./(1.+i))
    end
    return sum
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


print(string.format("\"exp loop\", %.8f, 20", profile_it(20, function () expLoop(1000000) end)))
