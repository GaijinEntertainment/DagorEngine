//expect:w293

::mkWatched <- require("sq3_sa_test").mkWatched


let isOpened = ::mkWatched(persist, "isOpened", false)
let x = persist("isOpened", 1)

return {isOpened, x}

