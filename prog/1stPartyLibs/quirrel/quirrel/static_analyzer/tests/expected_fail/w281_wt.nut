//expect:w281

::premiumUnlock <- require("sq3_sa_test").premiumUnlock
::basicUnlock <- require("sq3_sa_test").basicUnlock

local getSeasonMainPrizesData = @() (::premiumUnlock.value?.meta.promo ?? [])
  .extend(::basicUnlock.value?.meta.promo ?? [])

return getSeasonMainPrizesData
