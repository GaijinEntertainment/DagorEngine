// Squirrel module under test: platform (registered by platformMisc.cpp)

import "platform" as platform


// ============================================================
// Section 1: getter functions return strings
// ============================================================
println("--- Section 1: getter functions return strings ---")

let lang = platform.get_locale_lang()
assert(typeof lang == "string", $"get_locale_lang type: {typeof lang}")
assert(lang.len() >= 2, $"get_locale_lang length: {lang}")

let country = platform.get_locale_country()
assert(typeof country == "string", $"get_locale_country type: {typeof country}")

let pid = platform.get_platform_string_id()
assert(typeof pid == "string", $"get_platform_string_id type: {typeof pid}")
assert(pid.len() > 0, "get_platform_string_id non-empty")

let host = platform.get_host_platform()
assert(typeof host == "string", $"get_host_platform type: {typeof host}")

let arch = platform.get_host_arch()
assert(typeof arch == "string", $"get_host_arch type: {typeof arch}")

let defLang = platform.get_default_lang()
assert(typeof defLang == "string", "get_default_lang type")

println("Section 1 PASSED")


// ============================================================
// Section 2: console queries
// ============================================================
println("--- Section 2: console queries ---")

let model = platform.get_console_model()
assert(typeof model == "integer", $"get_console_model type: {typeof model}")
// On PC builds, the console model is UNKNOWN (==0).
assert(model == platform.UNKNOWN, $"PC build console_model UNKNOWN: {model}")

let rev = platform.get_console_model_revision(platform.UNKNOWN)
assert(typeof rev == "string", $"get_console_model_revision type: {typeof rev}")

let gdk = platform.is_gdk_used()
assert(typeof gdk == "bool", "is_gdk_used returns bool")
assert(gdk == false, "GDK not used in csq build")

println("Section 2 PASSED")


// ============================================================
// Section 3: console enum constants are integers
// ============================================================
println("--- Section 3: console enum constants are integers ---")

assert(typeof platform.UNKNOWN == "integer", "UNKNOWN const")
assert(typeof platform.PS4 == "integer", "PS4 const")
assert(typeof platform.PS5 == "integer", "PS5 const")
assert(typeof platform.XBOXONE == "integer", "XBOXONE const")
assert(typeof platform.NINTENDO_SWITCH == "integer", "NINTENDO_SWITCH const")
assert(platform.UNKNOWN != platform.PS4, "constants differ")

println("Section 3 PASSED")


// ============================================================
// Section 4: incorrect input
// ============================================================
println("--- Section 4: incorrect input ---")

local threw = false
try {
  platform.get_console_model_revision("not int")
} catch (e) {
  threw = true
}
assert(threw, "get_console_model_revision with non-int must throw")

println("Section 4 PASSED")


println("ALL TESTS PASSED")
