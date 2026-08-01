particles = {}
for i = 1, 1000 do
	table.insert(particles, i)
end

local function try_catch_loop(fails_count)
  fails_count = fails_count + 1000
  fails = 0
  cnt = 0
  for j = 1, 100 do
    for i = 1, fails_count do
       if not pcall(function ()
               cnt = cnt + particles[i]
               end)
        then
            fails = fails+1
        end
    end
  end
end
loadfile("profile.lua")()
io.write(string.format("try-catch loop: %.8f\n", profile_it(20, function () try_catch_loop(1000) end)))
