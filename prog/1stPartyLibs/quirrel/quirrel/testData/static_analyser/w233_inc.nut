

local y = 0, x = 0, r3x = 0;

let width = 100

let qrframe = {}

function ismasked(_a, _b) {}

for (y = 0; y < width; y++) {
    r3x = 0;
    for (x = 0; x < width; x++) {
      if (r3x == 3)
        r3x = 0;
      if (!r3x && !ismasked(x, y))
        qrframe[x + y * width] = qrframe[x + y * width] ^ 1;
      r3x++
    }
  }