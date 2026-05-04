options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct SafeOpFoo {
    a : string
}

def operator [] (tf : SafeOpFoo; cmp : string) : string {
    return "{tf . . a}-{cmp}"
}

def operator ?[] (tf : SafeOpFoo; cmp : string) : string const?# {
    if (cmp == tf . . a) {
        unsafe {
            return reinterpret<string const?#> addr(tf . . a)
        }
    } else {
        unsafe {
            return reinterpret<string const?#> null
        }
    }
}

def operator .(tf : SafeOpFoo; name : string) : SafeOpFoo {
    return SafeOpFoo(a = "{tf . . a}.{name}")
}

def operator ?.(tf : SafeOpFoo; name : string) : string const?# {
    if (name == "yes") {
        unsafe {
            return reinterpret<string const?#> addr(tf . . a)
        }
    } else {
        unsafe {
            return reinterpret<string const?#> null
        }
    }
}

[test]
def test_safe_operators(t : T?) {
    t |> run("operator [] on struct") @(t : T?) {
        var tf = SafeOpFoo(a = "hello")
        t |> equal(tf["blah"], "hello-blah")
    }
    t |> run("operator ?[] miss and hit") @(t : T?) {
        var tf = SafeOpFoo(a = "hello")
        t |> equal(tf?["blah"] ?? "not_blah", "not_blah")
        t |> equal(tf?["hello"] ?? "this???", "hello")
    }
    t |> run("operator . on struct") @(t : T?) {
        var tf = SafeOpFoo(a = "hello")
        var ez = tf.fooBar
        t |> equal(ez . . a, "hello.fooBar")
    }
    t |> run("operator ?. hit and miss") @(t : T?) {
        var tf = SafeOpFoo(a = "hello")
        t |> equal(tf?.yes ?? "how??", "hello")
        t |> equal(tf?.no ?? "hard no", "hard no")
    }
}
