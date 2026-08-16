options gen2

expect 30297

require ../language/_distinct_fixture.das

[export]
def main {
    var s : Secret      // 30297: private distinct is not visible across modules
}
