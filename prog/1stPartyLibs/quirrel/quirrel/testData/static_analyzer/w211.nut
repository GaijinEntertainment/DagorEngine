#allow-switch-statement

//expect:w211
local MODE = {
  MODE_1 = 1
  MODE_2 = 2
  MODE_3 = 3
}

local x = 1
switch(x) {
  case MODE.MODE_1: ::print("1"); break;
  case MODE.MODE_2: ::print("2"); break;
  case MODE.MODE_1: ::print("3"); break;
  default:
    ::print("0")
    break;
}