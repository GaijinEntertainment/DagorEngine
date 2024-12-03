let {dgs_get_settings, get_primary_screen_info} = require("dagor.system")
let DataBlock = require("DataBlock")
//let {flatten} = require("%sqstd/underscore.nut")
//let p = @(v) println($"{v}")
//let log = @(...) p(" ".join(flatten(vargv)))


function main(){
  let blk = DataBlock()
  blk.tryLoad("devlauncher_settings.blk") //-disable-warning:-w238

  local savedWindowWidth = blk?.windowWidth ?? -1
  local savedWindowHeight = blk?.windowHeight ?? -1

  if (savedWindowHeight == -1 || savedWindowWidth == -1) {
    local psi = {pixelsHeight = 720, pixelsWidth = 1280}
    try { psi = get_primary_screen_info() } catch(e) {}
    local {pixelsHeight, pixelsWidth} = psi

    // force 16:9 aspect ratio
    let pHgt = pixelsWidth * 9/16
    if (pHgt > pixelsHeight) {
      pixelsWidth = pixelsHeight * 16/9 //for ultrawide screen
    }
    else {
      pixelsHeight = pHgt
    }

    savedWindowHeight = pixelsHeight * 3/4
    savedWindowWidth = pixelsWidth * 3/4
  }

  let videoBlk = dgs_get_settings().addBlock("video")
  videoBlk.setStr("mode", "windowed")//"noborderwindow"
  videoBlk.setStr("resolution", $"{savedWindowWidth} x {savedWindowHeight}")
}

if (__name__=="__main__")
  main()