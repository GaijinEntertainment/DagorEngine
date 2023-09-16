def panic_time(msg)
    panic(msg)
    return false

[export]
def test
    var libertad = true
    libertad = libertad || panic_time("do not panic")
    libertad ||= panic_time("now panic")
    libertad = false
    libertad = libertad && panic_time("do not panic")
    libertad &&= panic_time("now panic")
    return true
