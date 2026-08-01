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

  loadfile("profile.lua")()
  ---
  io.write(string.format("\"string2float\", %.8f, 20\n", profile_it(20, function () update(nums) end)))

