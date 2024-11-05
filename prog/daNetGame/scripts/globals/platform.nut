let platform = require("%sqstd/platform.nut")
let sony = require_optional("sony")
let {dgs_get_settings, DBGLEVEL} = require("dagor.system")

let {SCE_REGION, platformId, consoleRevision} = platform
local {aliases} = platform

let isPlatformRelevant = @(platforms)
  platforms.len() == 0 || platforms.findvalue(@(p) aliases?[p] ?? (p == platformId)) != null


local ps4RegionName = "no region on this platform"

if (platform.is_sony && sony != null) {
   let SONY_REGION_NAMES = {
     [sony.region.SCEE]  = SCE_REGION.SCEE,
     [sony.region.SCEA]  = SCE_REGION.SCEA,
     [sony.region.SCEJ]  = SCE_REGION.SCEJ
   }
   ps4RegionName = SONY_REGION_NAMES[sony.getRegion()]
   aliases = aliases.__merge({
     [$"{platformId}_{ps4RegionName}"] = true,
     [$"sony_{ps4RegionName}"] = true
   })
}


return platform.__merge({
  aliases,
  isPlatformRelevant,
  ps4RegionName,
  //to avoid making complex code with watches and stuff, where it is not needed,
  //"assumed" isTouchPrimary value is used to adopt parts of UI code to touch-friendly layout
  isTouchPrimary = platform.is_mobile || (platform.is_pc && DBGLEVEL > 0 && dgs_get_settings()?.debug["touchScreen"]),
  consoleRevision
})
