let gs = require("%sqstd/globalState.nut")

return gs.__merge({ nestWatched = gs.hardPersistWatched })