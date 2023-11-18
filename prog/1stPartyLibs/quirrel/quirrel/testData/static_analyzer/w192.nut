//expect:w192
local test = true
local a = 1, b = 2
if (test == 3)
  a = 6; a = 10


if (test == 5)
  a = 7
else
  a = 6; a = 10


while (a > b)
    a = 0; b = 20;



::print(a + b);