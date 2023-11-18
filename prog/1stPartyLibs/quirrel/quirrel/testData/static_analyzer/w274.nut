//expect:w274

local tab = []

foreach (v in [1, 2, 3])
  tab.append(@() v)

return tab

