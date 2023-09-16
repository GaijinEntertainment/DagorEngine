//expect:w270

::handlersManager <- require("sq3_sa_test").handlersManager
::PERSISTENT_DATA_PARAMS <- require("sq3_sa_test").PERSISTENT_DATA_PARAMS


::handlersManager[::PERSISTENT_DATA_PARAMS].extend([ "curControlsAllowMask", "isCurSceneBgBlurred" ])
