function fo(debrData) {
  if (typeof debrData?.players != "table")
    return  // Early exit if debrData is null or players isn't a table

  let players = {}
  foreach (id, player in debrData.players)
    players[id.tointeger()] <- player
  debrData.players = players
}


fo({})
