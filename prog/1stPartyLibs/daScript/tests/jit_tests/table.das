options gen2
require dastest/testing_boost

def make_table {
    return <- {
        1 => "1",
        2 => "2",
        3 => "3"
    }
}

def test_table(var tab : table<int; string>) {
    unsafe {
        return length(tab) == 3 && tab[1] == "1" && tab[2] == "2" && tab[3] == "3"
    }
}

def make_table_jit {
    return <- {
        1 => "1",
        2 => "2",
        3 => "3"
    }
}

def test_table_jit(var tab : table<int; string>) {
    unsafe {
        return length(tab) == 3 && tab[1] == "1" && tab[2] == "2" && tab[3] == "3"
    }
}

def tab_erase(var tab : table<int; string>; key : int) {
    __builtin_table_erase(tab, key)
}

def tab_set_insert(var tab : table<int>; key : int) {
    __builtin_table_set_insert(tab, key)
}

def tab_find(var tab : table<int; string>; key : int) {
    return __builtin_table_find(tab, key)
}

def tab_key_exist(var tab : table<int; string>; key : int) {
    return __builtin_table_key_exists(tab, key)
}

[test]
def test_table(t : T?) {
    t |> run("make_table") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_table_jit))
        var tab <- make_table_jit()
        t |> success(test_table(tab))
    }
    t |> run("[], size") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_table_jit))
        var tab <- make_table()
        t |> success(test_table_jit(tab))
    }
    t |> run("erase") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@tab_erase))
        var tab <- {
            1 => "1",
            2 => "2",
            3 => "3"
        }
        tab_erase(tab, 2)
        t |> success(key_exists(tab, 1))
        t |> success(!key_exists(tab, 2))
        t |> success(key_exists(tab, 3))
    }
    t |> run("set insert") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@tab_set_insert))
        var tab <- { 1, 2, 4 }
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
        t |> success(!key_exists(tab, 3))
        t |> success(key_exists(tab, 4))
        tab_set_insert(tab, 3)
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
        t |> success(key_exists(tab, 3))
        t |> success(key_exists(tab, 4))
    }
    t |> run("find") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@tab_find))
        var tab <- {
            1 => "1",
            2 => "2",
            4 => "4"
        }
        t |> equal("1", *tab_find(tab, 1))
        t |> equal("2", *tab_find(tab, 2))
        t |> equal(null, tab_find(tab, 3))
        t |> equal("4", *tab_find(tab, 4))
    }
    t |> run("key exists") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@tab_key_exist))
        var tab <- {
            1 => "1",
            2 => "2",
            4 => "4"
        }
        t |> success(tab_key_exist(tab, 1))
        t |> success(tab_key_exist(tab, 2))
        t |> success(!tab_key_exist(tab, 3))
        t |> success(tab_key_exist(tab, 4))
    }
}

