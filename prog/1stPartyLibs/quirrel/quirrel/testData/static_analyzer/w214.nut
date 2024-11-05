if (__name__ == "__analysis__")
  return

//expect:w214
local test = ::f

enum REPLAY {
  SKIRMISH = 2
  BATTLE = 3 //-declared-never-used
}

::x <- test ? REPLAY.SKIRMISH : REPLAY.SKIRMISH

//-file:undefined-global
