let gs = require("%sqstd/globalState.nut")

return freeze(gs.__merge({ nestWatched = gs.hardPersistWatched }))