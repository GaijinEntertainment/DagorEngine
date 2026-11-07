if (__name__ == "__analysis__")
  return

class Squad {
  constructor(_n) {}
}

let Team = class {
  constructor(_n, _name = "") {}
}

local Shared = class { constructor(_a) {} }
local Use = Shared
Shared = class { constructor(_a, _b) {} }
class Plain {}
class Unit(Squad) {}
class Corps(Unit) {}
class Solo(Squad) { constructor() {} }
local Self = class(Team) {}
Self = class(Self) {}
local Base = Team
class Late(Base) {}
Base = Squad
class InPlace(class { constructor(_n) {} }) {}
local Again = class(Squad) {}
do {
  Again = class(Again) {}
} while (false)

let _a = Squad()
let _b = Squad(4)
let _c = Team()
let _d = Team(4)
let _e = Team(4, "red", 1)
let _use = Use(1, 2)
let _f = Plain()
let _g = Unit()
let _h = Unit(4)
let _j = Corps()
let _k = Solo(4)
let _i = Self(4)
let _m = Self()
let _o = Late(4, "red")
let _p = Late()
let _ip = InPlace()
let _r = Again()

local Shift = Squad
for (local i = 0; i < 2; ++i) {
  let Loop = class(Shift) {}
  let Stay = class(Squad) {}
  Shift = class { constructor() {} }
  if (i > 0) {
    Loop()
    Stay()
  }
  if (i == 0)
    Loop(1)
}

local count = 0
class Counter { constructor() { count += 1 } }
class Counted(Counter) {}
let _s = Counted()
let _t = 10 / count

function _aliasCall(src) {
  local q = src
  src = class { constructor(_a) {} }
  let _r = q(1, 2)
}

let _shared = Shared(1)
