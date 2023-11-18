//expect:w200

let function fn(mod, wpUnitRank) { //-declared-never-used
  return (mod?.reqRank > wpUnitRank)
}
