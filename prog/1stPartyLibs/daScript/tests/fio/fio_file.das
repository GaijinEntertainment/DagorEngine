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
