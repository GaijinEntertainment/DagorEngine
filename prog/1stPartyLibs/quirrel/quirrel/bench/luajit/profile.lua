local clock = os.clock
local format = string.format
local print = print
local tostring = tostring

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

return { benchmark = benchmark }
