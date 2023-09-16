

function fo(debrData) {
    if (typeof debrData?.players != "table")
      return

    let players = {}
    foreach (id, player in debrData.players)
      players[id.tointeger()] <- player
    debrData.players = players
  }


fo({})