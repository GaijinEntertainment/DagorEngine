--!strict
local function benchmark(name: string, runs: number, workload: () -> ...any, repeats: number?)
  local repeatCount = repeats or 1
  local best = math.huge
  local result: any = nil
  for _ = 1, runs do
    local start = os.clock()
    result = workload()
    local elapsed = os.clock() - start
    if elapsed < best then
      best = elapsed
    end
  end
  local tail = if result == nil then "" else `, {result}`
  print(string.format('"%s", %.8f, %d%s', name, best / repeatCount, runs * repeatCount, tail))
end

local function vmBench(name: string, workload: () -> any)
  for _ = 1, 5 do
    local start = os.clock()
    local result = workload()
    print(string.format("BENCH %s %.3f ms (r=%s)", name, (os.clock() - start) * 1000, tostring(result)))
  end
end

return { benchmark = benchmark, vmBench = vmBench }
