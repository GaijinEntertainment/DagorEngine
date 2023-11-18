//expect:w281
//-file:declared-never-used
//-file:undefined-global


local x = [1, 2]

let function fn(arr) {
  return (arr ?? []).extend(x)
//  ::y <- (arr ?? []).extend(x)
}

local tab = {
    watch = (::alertWatched ? ::alertWatched : []).extend(::titleWatched ? ::titleWatched : [])
  }

local getSeasonMainPrizesData = @() (::premiumUnlock.value?.meta.promo ?? [])
  .extend(::basicUnlock.value?.meta.promo ?? [])
