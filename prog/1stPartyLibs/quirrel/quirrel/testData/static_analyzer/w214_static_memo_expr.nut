if (__name__ == "__analysis__")
  return

//expect:w214
local test = ::f

enum REPLAY {
  SKIRMISH = 2
  BATTLE = 3 //-declared-never-used
}

::x <- static(test ? REPLAY.SKIRMISH : REPLAY.SKIRMISH) //-w316

//-file:undefined-global
