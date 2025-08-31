from "%darg/ui_imports.nut" import *
import "datacache"
from "eventbus" import eventbus_subscribe_onehit

let cursors = require("samples_prog/_cursors.nut")

let movieFileName = Watched(null)
let movieError = Watched(null)

const CACHE_NAME = "video"
datacache.init_cache(CACHE_NAME, {
  mountPath = "gamedatadir"
})

function load_movie(name) {
  eventbus_subscribe_onehit($"datacache.{name}", function(resp) {
    if ("error" in resp) {
      movieError.set(resp.error)
      movieFileName.set(null)
    } else {
      movieError.set(null)
      movieFileName.set(resp.path)
    }
  })
  datacache.request_entry(CACHE_NAME, name)
}

load_movie("http://localhost:8000/gaijin.ogg")

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  children = @() {
    watch = [movieFileName, movieError]
    size = static [sh(60), sh(40)]
    rendObj = ROBJ_MOVIE
    movie = movieFileName.get()
    behavior = movieFileName.get() ? Behaviors.Movie : null

    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = movieError.get() ? {
      rendObj = ROBJ_TEXT
      text = movieError.get()
    } : null
  }
}
