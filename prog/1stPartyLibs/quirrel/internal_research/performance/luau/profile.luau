--!strict
local function vmBench(name: string, workload: () -> any)
  for _ = 1, 5 do
    local start = os.clock()
    local result = workload()
    print(string.format("BENCH %s %.3f ms (r=%s)", name, (os.clock() - start) * 1000, tostring(result)))
  end
end

return { vmBench = vmBench }
