options gen2
require dastest/testing_boost public

require daslib/fio

// Non-ANSI filenames across the fio surface. das strings are UTF-8 by convention;
// every fio boundary must convert explicitly (wide CRT / u8path / u8string on Windows)
// instead of letting MSVC interpret the bytes in the ANSI codepage. Each test builds its
// own fixture under an ASCII-named temp dir and cleans up after itself.

let UTF8_NAME = "тест_目录"           // Cyrillic + CJK, both outside every Western ANSI codepage
let UTF8_FILE = "файл_文件.txt"
let UTF8_EMOJI = "aim_🎯.txt"         // supplementary plane - UTF-16 surrogate pair

def private make_fixture(t : testing::T?) : string {
    var error : string
    let root = create_temp_directory("fio_utf8", error)
    if (empty(root)) {
        t |> failure("create_temp_directory failed: {error}")
    }
    return root
}

def private drop_fixture(t : testing::T?; root : string) {
    return if (empty(root))
    var error : string
    if (!rmdir_rec(root, error)) {
        t |> failure("fixture cleanup failed for {root}: {error}")
    }
}

[test]
def test_mkdir_stat_utf8(t : T?) {
    t |> run("mkdir with a UTF-8 name creates that exact name") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let d = path_join(root, UTF8_NAME)
        t |> success(mkdir(d), "mkdir should succeed")
        // stat goes through the wide path already - it only sees the dir if mkdir
        // created the RIGHT name, not an ANSI mojibake one
        var st : FStat
        t |> success(stat(d, st), "stat should find the UTF-8 name mkdir created")
        drop_fixture(t, root)
    }
    t |> run("mkdir error-variant with a UTF-8 name") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let d = path_join(root, UTF8_NAME)
        var error : string
        t |> success(mkdir(d, error), "mkdir(ec) should succeed: {error}")
        t |> success(fexist(d), "fexist should find the UTF-8 name mkdir(ec) created")
        drop_fixture(t, root)
    }
}

[test]
def test_file_roundtrip_utf8(t : T?) {
    t |> run("fwrite/fread round-trip through a UTF-8 filename") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_FILE)
        let content = "содержимое 内容 ok"
        t |> success(fwrite(f, content), "fwrite should succeed")
        t |> equal(fread(f), content)
        var st : FStat
        t |> success(stat(f, st), "stat should see the file")
        t |> equal(int(st.size), length(content))
        drop_fixture(t, root)
    }
    t |> run("surrogate-pair (emoji) filename round-trips") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_EMOJI)
        t |> success(fwrite(f, "x"), "fwrite should succeed")
        t |> success(fexist(f), "fexist should see the emoji name")
        drop_fixture(t, root)
    }
}

[test]
def test_dir_lists_utf8(t : T?) {
    t |> run("dir() yields the UTF-8 name byte-exact") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_FILE)
        t |> success(fwrite(f, "x"), "fwrite should succeed")
        var found = false
        dir(root) $(filename) {
            if (filename == UTF8_FILE) {
                found = true
            }
        }
        t |> success(found, "listing should contain the exact UTF-8 name")
        drop_fixture(t, root)
    }
    t |> run("dir_rec() yields the UTF-8 relative path, not a skip") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let d = path_join(root, UTF8_NAME)
        t |> success(mkdir(d), "mkdir should succeed")
        t |> success(fwrite(path_join(d, UTF8_FILE), "x"), "fwrite should succeed")
        var names : array<string>
        var error : string
        dir_rec(root, $(filename; is_dir) {
            names |> push(to_generic_path(filename))
        }, error)
        t |> success(empty(error), "dir_rec should not error: {error}")
        t |> success(find_index(names, UTF8_NAME) >= 0, "dir_rec should list the UTF-8 dir")
        t |> success(find_index(names, "{UTF8_NAME}/{UTF8_FILE}") >= 0, "dir_rec should list the nested UTF-8 file")
        drop_fixture(t, root)
    }
}

[test]
def test_path_helpers_utf8(t : T?) {
    t |> run("pure path helpers keep UTF-8 bytes intact") @(t : T?) {
        let p = "каталог/файл_文件.txt"
        t |> equal(extension(p), ".txt")
        t |> equal(stem(p), "файл_文件")
        t |> equal(base_name(p), "файл_文件.txt")
        t |> equal(dir_name(p), "каталог")
        t |> equal(to_generic_path(path_join("каталог", "файл_文件.txt")), p)
        t |> equal(replace_extension(p, ".dat"), "каталог/файл_文件.dat")
        t |> equal(to_generic_path(normalize("каталог/./файл_文件.txt")), p)
        t |> equal(to_generic_path(parent("каталог/подкаталог/файл_文件.txt")), "каталог/подкаталог")
    }
    t |> run("relative() over UTF-8 components") @(t : T?) {
        var error : string
        let rel = relative("база/каталог/файл_文件.txt", "база", error)
        t |> success(empty(error), "relative should not error: {error}")
        t |> equal(to_generic_path(rel), "каталог/файл_文件.txt")
    }
    t |> run("get_full_file_name resolves a UTF-8 name") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_FILE)
        t |> success(fwrite(f, "x"), "fwrite should succeed")
        let full = get_full_file_name(f)
        t |> success(!empty(full), "full name should resolve")
        t |> equal(base_name(full), UTF8_FILE)
        drop_fixture(t, root)
    }
}

[test]
def test_mutations_utf8(t : T?) {
    t |> run("rename UTF-8 to UTF-8, both arities") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let a = path_join(root, UTF8_FILE)
        let b = path_join(root, "новый_新名.txt")
        t |> success(fwrite(a, "x"), "fwrite should succeed")
        t |> success(rename(a, b), "rename should succeed")
        t |> success(!fexist(a) && fexist(b), "rename should move the exact names")
        var error : string
        t |> success(rename(b, a, error), "rename(ec) should succeed: {error}")
        t |> success(fexist(a), "rename(ec) should restore the name")
        drop_fixture(t, root)
    }
    t |> run("copy_file with UTF-8 names") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let a = path_join(root, UTF8_FILE)
        let b = path_join(root, "копия_副本.txt")
        t |> success(fwrite(a, "data"), "fwrite should succeed")
        var error : string
        t |> success(copy_file(a, b, false, error), "copy_file should succeed: {error}")
        t |> equal(fread(b), "data")
        drop_fixture(t, root)
    }
    t |> run("remove and rmdir with UTF-8 names, both arities") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_FILE)
        t |> success(fwrite(f, "x"), "fwrite should succeed")
        t |> success(remove(f), "remove should succeed")
        t |> success(!fexist(f), "removed file should be gone")
        t |> success(fwrite(f, "x"), "fwrite should succeed again")
        var error : string
        t |> success(remove(f, error), "remove(ec) should succeed: {error}")
        t |> success(!fexist(f), "remove(ec) should delete the exact name")
        let d = path_join(root, UTF8_NAME)
        t |> success(mkdir(d), "mkdir should succeed")
        t |> success(rmdir(d), "rmdir should remove the UTF-8 dir")
        t |> success(mkdir(d), "mkdir should succeed again")
        t |> success(rmdir(d, error), "rmdir(ec) should succeed: {error}")
        drop_fixture(t, root)
    }
    t |> run("file_size sees through a UTF-8 name") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let f = path_join(root, UTF8_FILE)
        t |> success(fwrite(f, "12345"), "fwrite should succeed")
        var error : string
        t |> equal(int(file_size(f, error)), 5)
        t |> success(empty(error), "file_size should not error: {error}")
        drop_fixture(t, root)
    }
}

[test]
def test_cwd_utf8(t : T?) {
    t |> run("chdir into a UTF-8 dir and getcwd reports it") @(t : T?) {
        let root = make_fixture(t)
        return if (empty(root))
        let saved = getcwd()
        t |> success(!empty(saved), "getcwd should work")
        let d = path_join(root, UTF8_NAME)
        t |> success(mkdir(d), "mkdir should succeed")
        t |> success(chdir(d), "chdir into the UTF-8 dir should succeed")
        t |> equal(base_name(getcwd()), UTF8_NAME)
        t |> success(chdir(saved), "chdir back should succeed")
        drop_fixture(t, root)
    }
}
