import "%sqstd/platform.nut" as platform
from "dagor.system" import dgs_get_settings, DBGLEVEL
let sony = require_optional("sony")
let { aliases, is_pc, is_gdk, is_sony, is_mobile, SCE_REGION, platformId } = platform

// platforms have either platformId(xboxOne/xboxScarlett) or alias - xbox.
let winStoreAliases = ["xbox", "xboxOne", "xboxScarlett"]

local ps4RegionName = "no region on this platform"
let naliases = clone aliases
if (is_sony && sony != null) {
   let SONY_REGION_NAMES = {
     [sony.region.SCEE]  = SCE_REGION.SCEE,
     [sony.region.SCEA]  = SCE_REGION.SCEA,
     [sony.region.SCEJ]  = SCE_REGION.SCEJ
   }
   ps4RegionName = SONY_REGION_NAMES[sony.getRegion()]
   naliases.__update({
     [$"{platformId}_{ps4RegionName}"] = true,
     [$"sony_{ps4RegionName}"] = true
   })
}

let isPlatformRelevant = is_pc && is_gdk
  ? @(platforms)
      platforms.len() == 0 || platforms.findvalue(@(p) winStoreAliases.contains(p)) != null
  : @(platforms)
      platforms.len() == 0 || platforms.findvalue(@(p) naliases?[p] ?? (p == platformId)) != null

//to avoid making complex code with watches and stuff, where it is not needed,
//"assumed" isTouchPrimary value is used to adopt parts of UI code to touch-friendly layout
let isTouchPrimary = is_mobile
  || !!(platform.is_pc && DBGLEVEL > 0 && dgs_get_settings()?.debug["touchScreen"])

return freeze(platform.__merge({
  aliases = naliases
  isPlatformRelevant
  ps4RegionName
  isTouchPrimary
}))
