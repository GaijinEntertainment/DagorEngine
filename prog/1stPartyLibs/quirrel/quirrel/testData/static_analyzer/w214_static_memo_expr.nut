if (__name__ == "__analysis__")
  return

//-file:undefined-global

local test = ::f

enum REPLAY {
  SKIRMISH = 2
  BATTLE = 3 //-declared-never-used
}

::x <- static(test ? REPLAY.SKIRMISH : REPLAY.SKIRMISH) //-w316
