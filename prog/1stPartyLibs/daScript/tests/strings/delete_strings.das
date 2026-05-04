options gen2
require dastest/testing_boost
require strings
require daslib/random

[test]
def test_delete_string(t : T?) {
    var constStr = "123"
    unsafe {
        let removed = delete_string(constStr)
        t.equal(removed, false) // const heap string should not be removed
        t.equal(constStr, "123")
    }

    var seed = random_seed(0)
    var str = "test{random_float(seed)}"
    unsafe {
        let removed = delete_string(str)
        t.equal(removed, true) // heap string should be removed
        t.equal(str, "")
    }

    unsafe {
        var emptyStr : string
        let removed = delete_string(emptyStr)
        t.equal(removed, false) // corner case: empty string should not be removed
        t.equal(emptyStr, "")
    }
}


[test]
def test_delete_temp_string(t : T?) {
    var tempString = unsafe(reinterpret<string#>("123"))
    unsafe {
        let removed = delete_string(tempString)
        t.equal(removed, false) // const string should not be removed
        t.equal(tempString, "123")
    }

    var seed = random_seed(0)
    var tempString2 = unsafe(reinterpret<string#>("test{random_float(seed)}"))
    unsafe {
        let removed = delete_string(tempString2)
        t.equal(removed, true) // heap string should be removed
        t.equal(tempString2, "") // temp string should be empty after removal
    }
}


