options gen2
require dastest/testing_boost public
require daslib/strings_boost

[test]
def test_string_operations(t : T?) {
    t |> run("string building") @(t : T?) {
        let s = "hello"
        let w = "world"
        t |> equal("{s} {w}", "hello world")
        t |> equal("{1 + 2}", "3")
    }
    t |> run("string functions") @(t : T?) {
        t |> success("hello world" |> starts_with("hello"))
        t |> success("hello world" |> ends_with("world"))
        t |> equal(length("test"), 4)
    }
    t |> run("string comparison") @(t : T?) {
        t |> success("abc" == "abc")
        t |> success("abc" != "def")
        t |> success("abc" < "abd")
    }
}

def greet(name : string) : string {
    return "Hello, {name}!"
}

[test]
def test_string_functions(t : T?) {
    t |> run("custom string function") @(t : T?) {
        t |> equal(greet("daslang"), "Hello, daslang!")
    }
    t |> run("to_lower and to_upper") @(t : T?) {
        t |> equal(to_lower("HELLO"), "hello")
        t |> equal(to_upper("hello"), "HELLO")
    }
    t |> run("join") @(t : T?) {
        var arr : array<string>
        arr |> push("a")
        arr |> push("b")
        arr |> push("c")
        t |> equal(join(arr, ","), "a,b,c")
    }
}

def test_first_character_empty_throws(t : T?) {
    var threw = false
    try {
        let ch = first_character("")
        t |> failure("should have thrown, got {ch}")
    } recover {
        threw = true
    }
    t |> success(threw, "first_character on empty string should throw")
}

def test_first_character_ds_empty_throws(t : T?) {
    var threw = false
    with_das_string() <| $(var ds) {
        try {
            let ch = first_character(ds)
            t |> failure("should have thrown, got {ch}")
        } recover {
            threw = true
        }
    }
    t |> success(threw, "first_character on empty das_string should throw")
}

[test]
def test_first_character(t : T?) {
    t |> run("first_character basic") @(t : T?) {
        t |> equal(first_character("hello"), 'h')
        t |> equal(first_character("A"), 'A')
        t |> equal(first_character("123"), '1')
    }
    t |> run("first_character empty throws") @(t : T?) {
        test_first_character_empty_throws(t)
    }
    t |> run("first_character das_string") @(t : T?) {
        with_das_string() <| $(var ds) {
            ds := "hello"
            t |> equal(first_character(ds), 'h')
        }
        with_das_string() <| $(var ds) {
            ds := "Z"
            t |> equal(first_character(ds), 'Z')
        }
    }
    t |> run("first_character das_string empty throws") @(t : T?) {
        test_first_character_ds_empty_throws(t)
    }
    t |> run("character_at bounds check") @(t : T?) {
        t |> equal(character_at("hello", 0), 'h')
        t |> equal(character_at("hello", 4), 'o')
    }
}
