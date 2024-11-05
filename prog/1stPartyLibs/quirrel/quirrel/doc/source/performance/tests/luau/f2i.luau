	TOTAL_NUMBERS = 10000
	TOTAL_TIMES = 100

    function mk_float(i)
        return i + (i/TOTAL_NUMBERS)
    end

    function update(nums)
        local summ = 0
        for i = 1, TOTAL_TIMES do
            for i,num in ipairs(nums) do
                summ = summ + tonumber(num)
            end
        end
        return summ
    end

  nums = {}
  for i = 1, TOTAL_NUMBERS do
    table.insert(nums, tostring(mk_float(i)))
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

print(string.format("\"string2float\", %.8f, 20", profile_it(20, function () update(nums) end)))

