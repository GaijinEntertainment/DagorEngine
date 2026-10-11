local clock <const> = os.clock
local format <const> = string.format
local tostring <const> = tostring
local print <const> = print

local function benchmark(name, runs, workload, repeats)
  repeats = repeats or 1
  local best, result
  for _ = 1, runs do
    local start = clock()
    result = workload()
    local elapsed = clock() - start
    if best == nil or elapsed < best then
      best = elapsed
    end
  end
  local tail = result == nil and "" or ", " .. tostring(result)
  print(format("\"%s\", %.8f, %d%s", name, best / repeats, runs * repeats, tail))
end

local function vmBench(name, workload)
  for _ = 1, 5 do
    local start = clock()
    local result = workload()
    print(format("BENCH %s %.3f ms (r=%s)", name, (clock() - start) * 1000, tostring(result)))
  end
end

return { benchmark = benchmark, vmBench = vmBench }
