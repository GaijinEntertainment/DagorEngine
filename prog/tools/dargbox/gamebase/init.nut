local {dgs_get_settings, get_primary_screen_info} = require("dagor.system")
local { get_platform_string_id } = require("platform")
local platformId = dgs_get_settings().getStr("platform", get_platform_string_id())


if (["win32", "win64"].contains(platformId)) {
  let info = get_primary_screen_info()
  let {
//    vertDpi,
    pixelsHeight,
//    physHeight,
//    physWidth,
    pixelsWidth
  } = info
  //local scale = vertDpi / (pixelsHeight / (physHeight / 25.4))
  //dlog(info.keys(), physHeight, scale, vertDpi)
  //100% = 96 dpi 0.374453 scale
  //150% = 144 dpi
  //200% = 192 dpi, 0.748906 scale
  /*
  If in windowed mode = fontsize should be limited to up 2x of 100% and/or set by millimeters
  */

  local videoBlk = dgs_get_settings().addBlock("video")
  videoBlk.setStr("mode", "windowed")
  videoBlk.setStr("resolution", $"{pixelsWidth*3/4} x {pixelsHeight*3/4}")
  //dlog(dgs_get_settings())
}
