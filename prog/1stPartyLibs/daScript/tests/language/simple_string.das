options gen2

require UnitTest
require dastest/testing_boost public

// --- helpers that take string in different ways ---

// string by value (workhorse type, copies the pointer)
def take_string_by_value(s : string) : string {
    return s
}

// string by const value
def take_string_by_const_value(s : string const) : string {
    return s
}

// string by reference (mutable)
def take_string_by_ref(var s : string&; new_val : string) {
    s = new_val
}

// string by const reference
def take_string_by_const_ref(s : string const&) : string {
    return s
}

// --- helpers that take TestObjectBar and access simple_string ---

// TestObjectBar by const (default for handled types)
def read_ss(bar : TestObjectBar) : string {
    return bar.simple_string
}

// TestObjectBar by mutable reference
def write_ss(var bar : TestObjectBar; val : string) {
    bar.simple_string = val
}

// TestObjectBar by explicit const
def read_ss_const(bar : TestObjectBar const) : string {
    return bar.simple_string
}

// --- tests ---

[test]
def test_simple_string_read_write(t : T?) {
    t |> run("read and write simple_string field") @(t : T?) {
        var bar = new TestObjectBar
        t |> equal(bar.simple_string, "")
        bar.simple_string = "hello"
        t |> equal(bar.simple_string, "hello")
        bar.simple_string = "world"
        t |> equal(bar.simple_string, "world")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_pass_value(t : T?) {
    t |> run("pass simple_string as string by value") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "by_value"
        let result = take_string_by_value(bar.simple_string)
        t |> equal(result, "by_value")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_pass_const_value(t : T?) {
    t |> run("pass simple_string as string by const value") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "by_const_value"
        let result = take_string_by_const_value(bar.simple_string)
        t |> equal(result, "by_const_value")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_pass_const_ref(t : T?) {
    t |> run("pass simple_string as string by const reference") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "by_const_ref"
        let result = take_string_by_const_ref(bar.simple_string)
        t |> equal(result, "by_const_ref")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_pass_ref(t : T?) {
    t |> run("pass string variable by mutable reference") @(t : T?) {
        var s = "before"
        take_string_by_ref(s, "after")
        t |> equal(s, "after")
    }
}

[test]
def test_simple_string_via_bar_const(t : T?) {
    t |> run("read simple_string via TestObjectBar const") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "via_const"
        let result = read_ss_const(deref(bar))
        t |> equal(result, "via_const")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_via_bar(t : T?) {
    t |> run("read simple_string via TestObjectBar") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "via_bar"
        let result = read_ss(deref(bar))
        t |> equal(result, "via_bar")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_write_via_bar(t : T?) {
    t |> run("write simple_string via TestObjectBar mutable ref") @(t : T?) {
        var bar = new TestObjectBar
        write_ss(deref(bar), "written")
        t |> equal(bar.simple_string, "written")
        unsafe {
            delete bar
        }
    }
}

[test]
def test_simple_string_overwrite(t : T?) {
    t |> run("simple_string overwrite preserves latest value") @(t : T?) {
        var bar = new TestObjectBar
        bar.simple_string = "first"
        bar.simple_string = "second"
        bar.simple_string = "third"
        t |> equal(bar.simple_string, "third")
        unsafe {
            delete bar
        }
    }
}
