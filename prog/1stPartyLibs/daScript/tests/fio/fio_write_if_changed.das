options gen2
options indenting = 4

require dastest/testing_boost
require daslib/defer
require daslib/fio

def private scratch(name : string) : string {
    var terr : string
    let tmp = temp_directory(terr)
    return path_join(empty(tmp) ? "." : tmp, "{name}_{ref_time_ticks()}.txt")
}

[test]
def test_identical_text_is_not_written(t : T?) {
    let path = scratch("fio_wic_same")
    defer() {
        var err : string
        remove(path, err)
    }
    t |> success(fwrite_if_changed(path, "one\n"), "a file that does not exist yet is written")
    let stamp = stat(path).mtime
    t |> success(!fwrite_if_changed(path, "one\n"), "the same text is not written again")
    t |> equal(stat(path).mtime, stamp, "so the timestamp does not move")
    t |> equal(fread(path), "one\n", "and the text is still there")
}

[test]
def test_different_text_is_written(t : T?) {
    let path = scratch("fio_wic_diff")
    defer() {
        var err : string
        remove(path, err)
    }
    fwrite(path, "one\n")
    t |> success(fwrite_if_changed(path, "two\n"), "different text is written")
    t |> equal(fread(path), "two\n", "and it replaces what was there")
}
