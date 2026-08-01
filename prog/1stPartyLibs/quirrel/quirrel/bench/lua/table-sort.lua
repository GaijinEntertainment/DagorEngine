-- Public domain
--
function create_rng(seed)
  local Rm, Rj = {}, 1
  for i=1,17 do Rm[i] = 0 end
  for i=17,1,-1 do
    seed = (seed*9069) % (2^31)
    Rm[i] = seed
  end
  return function()
      local j, m = Rj, Rm
      local h = j - 5
      if h < 1 then h = h + 17 end
      local k = m[h] - m[j]
      if k < 0 then k = k + 2147483647 end
      m[j] = k
      if j < 17 then Rj = j + 1 else Rj = 1 end
      return k
  end
end

rand = create_rng(12345)

n = tonumber(arg and arg[1]) or 100000
t = {}
for i = 1,n do
	t[i] = rand()
end

function cmp(lhs, rhs)
  return lhs > rhs
end

function table.shallow_copy(t)
  local t2 = {}
  for k,v in pairs(t) do
    t2[k] = v
  end
  return t2
end

loadfile("profile.lua")()

io.write(string.format("\"sort\", %.8f, 20\n", profile_it(20, function () table.sort(table.shallow_copy(t), cmp) end)))


