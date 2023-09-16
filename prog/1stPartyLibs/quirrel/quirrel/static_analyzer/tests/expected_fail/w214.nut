//expect:w214

let { test } = require("sq3_sa_test")

enum REPLAY {
  SKIRMISH = 2
  BATTLE = 3
}

::x <- test ? REPLAY.SKIRMISH : REPLAY.SKIRMISH