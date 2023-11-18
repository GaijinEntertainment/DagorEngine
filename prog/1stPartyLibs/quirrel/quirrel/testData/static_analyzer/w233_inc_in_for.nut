


function foo(_a) {}

local k
local y = 0
for (k = 0; k < 7; k++) {
    if (y & 1) {
      if (k)
        foo(6 - k);
      else
        foo(y + k);
    }

    y = y >> 1
  }