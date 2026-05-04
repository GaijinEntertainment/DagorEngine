// Squirrel module under test: dagor.fs.vrom (registered by dagorFS.cpp register_dagor_fs_vrom_module)

from "dagor.fs.vrom" import scan_vrom_folder, get_vromfs_dump_version


// ============================================================
// Section 1: scan_vrom_folder returns array even when no vroms mounted
// ============================================================
println("--- Section 1: scan_vrom_folder returns array even when no vroms mounted ---")

let result = scan_vrom_folder({root = ".", files_suffix = ".nut", recursive = false})
assert(typeof result == "array", $"scan_vrom_folder type: {typeof result}")
// No vroms mounted -> empty result is the expected safe default.
assert(result.len() == 0, $"empty result without mounted vroms: {result.len()}")

println("Section 1 PASSED")


// ============================================================
// Section 2: get_vromfs_dump_version on missing vrom returns 0
// ============================================================
println("--- Section 2: get_vromfs_dump_version on missing vrom returns 0 ---")

let v = get_vromfs_dump_version("definitely_not_a_vrom.vromfs.bin")
assert(typeof v == "integer", $"get_vromfs_dump_version returns integer: {typeof v}")
assert(v == 0, $"missing vrom version is 0: {v}")

println("Section 2 PASSED")


// ============================================================
// Section 3: incorrect input
// ============================================================
println("--- Section 3: incorrect input ---")

local threw = false
try {
  scan_vrom_folder("not a table")
} catch (e) {
  threw = true
}
assert(threw, "scan_vrom_folder with non-table must throw")

threw = false
try {
  get_vromfs_dump_version(123)
} catch (e) {
  threw = true
}
assert(threw, "get_vromfs_dump_version with non-string must throw")

println("Section 3 PASSED")


println("ALL TESTS PASSED")
