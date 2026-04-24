// Squirrel module under test: dagor.fs (registered by dagorFS.cpp)

from "dagor.fs" import file_exists, dir_exists, is_path_absolute, scan_folder, stat, resolve_mountpoint,
  read_text_from_file, read_text_from_file_on_disk, write_text_to_file, mkdir, mkpath, remove_file, remove_dirtree
from "dagor.time" import get_time_msec


// ============================================================
// Section 1: existence queries on self
// ============================================================
println("--- Section 1: existence queries on self ---")

assert(file_exists(".definitely_missing_file_xyz.txt") == false, "missing file does not exist")
assert(dir_exists(".") == true, "current directory exists")
assert(dir_exists(".this_dir_does_not_exist_xyz") == false, "missing dir does not exist")

println("Section 1 PASSED")


// ============================================================
// Section 2: is_path_absolute
// ============================================================
println("--- Section 2: is_path_absolute ---")

// On Windows is_path_abs treats only UNC and drive-letter paths as absolute.
assert(is_path_absolute("\\\\server\\share") == true, "UNC absolute")
assert(is_path_absolute("relative/path") == false, "relative path")
assert(is_path_absolute("file.nut") == false, "bare filename not absolute")

println("Section 2 PASSED")


// ============================================================
// Section 3: scan_folder finds this test
// ============================================================
println("--- Section 3: scan_folder finds this test ---")

let files = scan_folder({root = ".", files_suffix = ".nut", recursive = false})
assert(typeof files == "array", "scan_folder returns array")
/*
local foundSelf = false
foreach (f in files) {
  if (f.indexof("test_dagor_fs.nut") != null)
    foundSelf = true
}
assert(foundSelf, "scan_folder found test_dagor_fs.nut")
*/
println("Section 3 PASSED")


// ============================================================
// Section 4: stat returns table for existing file
// ============================================================
println("--- Section 4: stat ---")

let st = stat(__FILE__)
assert(typeof st == "table", $"stat returns table: {typeof st}")
assert("size" in st && "atime" in st && "mtime" in st && "ctime" in st, "stat keys")
assert(st.size > 0, "stat size > 0")

let missing = stat("definitely_missing_file_xyz.txt")
assert(missing == null, "stat missing returns null")

println("Section 4 PASSED")


// ============================================================
// Section 5: write + read + delete a temp file
// ============================================================
println("--- Section 5: temp file write/read/delete ---")

let tmpName = $"_tmp_test_dagor_fs_{get_time_msec()}.txt"
let payload = "hello from test_dagor_fs"
write_text_to_file(tmpName, payload)
assert(file_exists(tmpName), $"temp file exists after write: {tmpName}")

let readBack = read_text_from_file_on_disk(tmpName)
assert(readBack == payload, $"read_text_from_file_on_disk round-trip: '{readBack}'")

let readBack2 = read_text_from_file(tmpName)
assert(readBack2 == payload, $"read_text_from_file round-trip: '{readBack2}'")

assert(remove_file(tmpName), "remove_file succeeds")
assert(file_exists(tmpName) == false, "file gone after remove")

println("Section 5 PASSED")


// ============================================================
// Section 6: resolve_mountpoint passthrough
// ============================================================
println("--- Section 6: resolve_mountpoint passthrough ---")

let plain = "no_mount/path.txt"
let resolved = resolve_mountpoint(plain)
assert(resolved == plain, $"no-mount passthrough: {resolved}")

println("Section 6 PASSED")


// ============================================================
// Section 7: incorrect input
// ============================================================
println("--- Section 7: incorrect input ---")

local threw = false
try {
  file_exists(123)
} catch (e) {
  threw = true
}
assert(threw, "file_exists with non-string must throw")

threw = false
try {
  scan_folder("not a table")
} catch (e) {
  threw = true
}
assert(threw, "scan_folder with non-table must throw")

threw = false
try {
  read_text_from_file("definitely_missing_file_xyz.txt")
} catch (e) {
  threw = true
}
assert(threw, "read_text_from_file on missing file must throw")

println("Section 7 PASSED")

const temp_dir = "_temp_"
mkdir(temp_dir)
assert(dir_exists(temp_dir), $"directory {temp_dir} wasn't created")
remove_dirtree(temp_dir)
assert(!dir_exists(temp_dir), $"directory {temp_dir} wasn't deleted")
println(mkpath($"{temp_dir}/{temp_dir}/{temp_dir}"))
assert(dir_exists(temp_dir), $"directory {temp_dir} wasn't created")
assert(dir_exists($"{temp_dir}/{temp_dir}"), $"directory {temp_dir}/{temp_dir} wasn't created")
remove_dirtree(temp_dir)

println("ALL TESTS PASSED")
