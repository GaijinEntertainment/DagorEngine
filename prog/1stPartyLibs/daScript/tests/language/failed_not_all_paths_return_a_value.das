// missing return in some code paths
// verifies compiler rejects a function that only returns a value
// in one branch of an if statement
options gen2
expect 40200

def test_return_int_vs_void(arg : bool) {// 40200: not all control paths return value
    if (arg) {
        return 13
    }
}

