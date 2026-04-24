if (__name__ == "__analysis__")
  return

//expect:w281
//-file:declared-never-used
//-file:undefined-global


local x = [1, 2]
local z = 555

function fn(arr) {
  return (arr ?? []).extend(x)
//  ::y <- (arr ?? []).extend(x)
}

local tab = {
    watch = (::alertWatched ? ::alertWatched : []).extend(::titleWatched ? ::titleWatched : [])
  }

local getSeasonMainPrizesData = @() (::premiumUnlock.value?.meta.promo ?? [])
  .extend(::basicUnlock.value?.meta.promo ?? [])

function baz(arr) {
  // both are temporaries - no warning needed
  return (x[0] ? [z] : []).extend(x)
}
