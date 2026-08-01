-- allocation and collection pressure (binary-trees style)
local function mkTree(depth)
  if depth == 0 then
    return { left = false, right = false }
  end
  return { left = mkTree(depth - 1), right = mkTree(depth - 1) }
end

local function checkTree(t)
  if not t.left then return 1 end
  return 1 + checkTree(t.left) + checkTree(t.right)
end

for rep = 1, 5 do
  local t0 = os.clock()
  local sum = 0
  for i = 1, 10 do
    sum = sum + checkTree(mkTree(13))
  end
  print("BENCH binarytrees " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. sum .. ")")
end
