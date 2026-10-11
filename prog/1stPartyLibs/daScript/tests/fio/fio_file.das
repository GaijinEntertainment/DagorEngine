options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer
require daslib/strings_boost

require daslib/fio

let TOTAL_FUZZ_FILES = 100

def operator ==(a, b : array<uint8> implicit) {
    if (length(a) != length(b)) {
        return false
    }
    for (A, B in a, b) {
        if (A != B) {
            return false
        }
    }
    return true
}

[test]
def test_empty_array_io(t : T?) {
    t |> run("fwrite/fread of an empty array is a 0-byte no-op, not an OOB addr") @(t : T?) {
        var terr : string
        let tmp = temp_directory(terr)   // portable (no /tmp on Windows); cwd as a last resort
        let fname = path_join(empty(tmp) ? "." : tmp, "fio_empty_io_{ref_time_ticks()}.bin")
        var none : array<int>
        var wrote = false
        fopen(fname, "wb") $(f) {
            if (f != null) {
                t |> equal(fwrite(f, none), 0)
                t |> equal(long_fwrite(f, none), 0l)
                wrote = true
            }
        }
        t |> success(wrote, "opened for write")
        var read_back = false
        fopen(fname, "rb") $(f) {
            if (f != null) {
                t |> equal(fread(f, none), 0)
                t |> equal(long_fread(f, none), 0l)
                read_back = true
            }
        }
        t |> success(read_back, "opened for read")
        t |> success(remove(fname))
        delete none
    }
}

[test]
def test_stat_is_exec(t : T?) {
    t |> run("stat reports the execute bit: a data file has none, an executable has it") @(t : T?) {
        var terr : string
        let tmp = temp_directory(terr)
        let dir = empty(tmp) ? "." : tmp
        let plain = path_join(dir, "fio_exec_probe_{ref_time_ticks()}.txt")
        t |> success(fwrite(plain, "x"), "the probe file is written")
        let st_plain = stat(plain)
        t |> success(st_plain.is_valid && st_plain.is_reg && !st_plain.is_exec, "a data file is not executable")
        if (get_platform_name() == "windows") {
            let exe = path_join(dir, "fio_exec_probe_{ref_time_ticks()}.exe")
            t |> success(fwrite(exe, "x"), "the .exe probe is written")
            t |> success(stat(exe).is_exec, "Windows: the .exe extension is the execute bit")
            t |> success(remove(exe))
        } else {
            var out = ""
            t |> equal(0, run_and_capture(["chmod", "755", plain], out, 10.0), "chmod runs")
            t |> success(stat(plain).is_exec, "chmod 755 sets the bit stat reports")
            t |> equal(0, run_and_capture(["chmod", "610", plain], out, 10.0), "chmod runs")
            t |> success(stat(plain).is_exec, "a group-only execute bit counts too")
            t |> equal(0, run_and_capture(["chmod", "600", plain], out, 10.0), "chmod runs")
            t |> success(!stat(plain).is_exec, "and no execute bit at all reads false")
            t |> success(stat(dir).is_exec && stat(dir).is_dir, "a directory carries the bit, so a program test pairs it with is_reg")
        }
        t |> success(remove(plain))
    }
}

[test]
def test_fopen(t : T?) {
    t |> run("fuzz fopen") @@(t : T?) {
        var fake <- Faker()
        fuzz(TOTAL_FUZZ_FILES) {
            let fname = fake |> any_file_name           // note: this should be any_string, but we are getting strange strange file names (unicode?), and one does not delete
            var stat : FStat
            if (!stat(fname, stat) && !stat.is_valid) {
                var f = fopen(fname, "w")
                if (f != null) {
                    fclose(f)
                    var written = false
                    fopen(fname, "a") $(ff) {
                        fwrite(ff, "test")
                        written = true
                    }
                    t |> success(written)
                    t |> success(remove(fname))
                }
            }
        }
    }

    t |> run("fuzz fread") @@(t : T?) {
        var fake <- Faker()
        fuzz(TOTAL_FUZZ_FILES) {
            let fname = fake |> any_file_name
            var stat : FStat
            if (!stat(fname, stat) && !stat.is_valid) {
                fopen(fname, "wb") $(f) {
                    fwrite(f, "test")
                }
                fopen(fname, "rb") $(f) {
                    t |> equal(fread(f), "test")
                }
                t |> success(remove(fname))
            }
        }
    }

    t |> run("fuzz fmap") @@(t : T?) {
        var fake <- Faker()
        fuzz(TOTAL_FUZZ_FILES) {
            let fname = fake |> any_file_name
            var stat : FStat
            if (!stat(fname, stat) && !stat.is_valid) {
                fopen(fname, "wb") $(f) {
                    fwrite(f, "test")
                }
                fopen(fname, "rb") $(f) {
                    fmap(f) $(arr1) {
                        peek_data("test") $(arr2) {
                            t |> equal(arr1, arr2)
                        }
                    }
                }
                t |> success(remove(fname))
            }
        }
    }
}
