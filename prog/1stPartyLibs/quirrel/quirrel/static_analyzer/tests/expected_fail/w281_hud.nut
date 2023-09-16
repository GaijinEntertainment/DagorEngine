//expect:w281

::alertWatched <- require("sq3_sa_test").alertWatched
::titleWatched <- require("sq3_sa_test").titleWatched


local tab = {
  watch = (::alertWatched ? ::alertWatched : []).extend(::titleWatched ? ::titleWatched : [])
}

return tab
