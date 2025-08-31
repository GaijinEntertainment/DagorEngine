//expect:w200

function fn(mod, wpUnitRank) { //-declared-never-used
  return (static(mod?.reqRank) > wpUnitRank) // -w316
}
