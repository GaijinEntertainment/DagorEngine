//expect:w200

function fn(mod, wpUnitRank) { //-declared-never-used
  return (mod?.reqRank > wpUnitRank)
}
