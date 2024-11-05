if (__name__ == "__analysis__")
  return

//expect:w270

::handlersManager[::PERSISTENT_DATA_PARAMS].extend([ "curControlsAllowMask", "isCurSceneBgBlurred" ]) // -undefined-global
