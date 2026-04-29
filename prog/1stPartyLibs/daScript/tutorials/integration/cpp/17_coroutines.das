options gen2

// A generator is a special function that can yield values one at a time.
// When the host calls the generator, it runs until the next "yield" and
// pauses.  The host can then resume it to get the next value.

[export]
def test {
    return <- generator<int>() <| $() {
        for (i in range(5)) {
            print("  [das] yielding {i}\n")
            yield i
        }
        print("  [das] generator finished\n")
        return false
    }
}
