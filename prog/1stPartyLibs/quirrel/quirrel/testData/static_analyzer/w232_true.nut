//expect:w232
local a = "     "
local s = {
  charAt =  @(i) a[i]
}
local i = 2
::print(s.charAt(i) != ' ' || s.charAt(i) != '\t')
