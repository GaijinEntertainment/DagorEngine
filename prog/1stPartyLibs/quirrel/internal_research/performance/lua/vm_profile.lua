local clock <const> = os.clock
local format <const> = string.format
local tostring <const> = tostring
local print <const> = print

local function vmBench(name, workload)
  for _ = 1, 5 do
    local start = clock()
    local result = workload()
    print(format("BENCH %s %.3f ms (r=%s)", name, (clock() - start) * 1000, tostring(result)))
  end
end

return { vmBench = vmBench }
