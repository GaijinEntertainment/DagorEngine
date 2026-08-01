local function fibI(n:number)
    local last : number = 0
    local cur : number = 1
    n = n - 1
    while (n > 0)
    do
        n = n - 1
        local tmp : number = cur
        cur = last + cur
        last = tmp
    end
    return cur
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

print(string.format("\"fibonacci loop\", %.8f, 20", profile_it(20, function () fibI(6511134) end)))

