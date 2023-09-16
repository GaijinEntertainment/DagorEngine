let {dgs_get_settings, DBGLEVEL, get_arg_value_by_name} = require("dagor.system")
let platform = require("platform")
let {get_platform_string_id, get_console_model, get_console_model_revision} = platform

let systemPlatformId = get_platform_string_id()
let settingsPlaformId = dgs_get_settings().getStr("platform", systemPlatformId)
let platformId = DBGLEVEL > 0 ? (get_arg_value_by_name("platform") ?? settingsPlaformId) : settingsPlaformId
let oneOf = @(...) vargv.contains(platformId)
let consoleModel = get_console_model()
let isModel = @(model) consoleModel == model
let consoleRevision = get_console_model_revision(consoleModel)

let is_pc = oneOf("win32", "win64", "macosx", "linux64")
let is_sony = oneOf("ps4", "ps5")
let is_xbox = oneOf("xboxOne", "xboxScarlett")
let is_nswitch = oneOf("nswitch")
let is_mobile = oneOf("iOS", "android")
let is_android = platformId == "android"
let is_console = is_sony || is_xbox || is_nswitch
let isXboxOne = platformId == "xboxOne"
let isXboxScarlett = platformId == "xboxScarlett"
let isXbox = isXboxOne || isXboxScarlett

let isPS4 = platformId == "ps4"
let isPS5 = platformId == "ps5"
let isSony = is_sony

let isPC = is_pc

let aliases = {
  pc = is_pc
  xbox = is_xbox
  sony = is_sony
  console = is_console
  mobile = is_mobile
  google = is_android
}
let platformAlias = is_sony ? "sony"
  : is_xbox ? "xbox"
  : is_mobile ? "mobile"
  : is_pc ? "pc"
  : is_android ? "android"
  : platformId

enum SCE_REGION {
  SCEE = "scee"
  SCEA = "scea"
  SCEJ = "scej"
}

return {
  platformId
  consoleRevision
  platformAlias
  is_pc
  is_windows = oneOf("win32", "win64")
  is_win32 = oneOf("win32")
  is_win64 = oneOf("win64")
  is_ps4 = oneOf("ps4")
  is_ps5 = oneOf("ps5")
  is_sony
  is_ios = oneOf("iOS")
  is_mobile
  is_android
  is_xbox
  is_xboxone = oneOf("xboxOne")
  is_xbox_scarlett = oneOf("xboxScarlett")
  is_nswitch
  is_xboxone_simple = isModel(platform.XBOXONE)
  is_xboxone_s = isModel(platform.XBOXONE_S)
  is_xboxone_X = isModel(platform.XBOXONE_X)
  is_xbox_lockhart = isModel(platform.XBOX_LOCKHART)
  is_xbox_anaconda = isModel(platform.XBOX_ANACONDA)
  is_ps4_simple = oneOf("ps4") && isModel(platform.PS4)
  is_ps4_pro = oneOf("ps4") && isModel(platform.PS4_PRO)
  is_console
  aliases
  SCE_REGION

  isXboxOne
  isXboxScarlett
  isXbox
  isPS4
  isPS5
  isSony
  isPC
}