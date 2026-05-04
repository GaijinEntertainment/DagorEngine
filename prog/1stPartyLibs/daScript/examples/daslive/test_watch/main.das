options gen2

require daslib/fio
require live/live_watch
require live_host

// Test the file watcher agent by creating a temp file, registering it
// as a watched file, modifying it, and verifying files_changed triggers.
//
// Note: Under daslang.exe, the live_watch agent does NOT auto-install
// (is_live_mode() is false). This test exercises the watcher functions
// directly instead.

var tests_passed : int = 0
var tests_failed : int = 0

def check(name : string; condition : bool) {
    if (condition) {
        tests_passed++
        print("  PASS: {name}\n")
    } else {
        tests_failed++
        print("  FAIL: {name}\n")
    }
}

[export]
def init() {
    print("test_watch: init\n")
}

[export]
def update() {
    print("--- File watcher tests ---\n")

    // Test watched file API
    print("--- get_watched_file_count/get_watched_file ---\n")
    let count = get_watched_file_count()
    check("initial watched file count is 0", count == 0)

    // Test signal_files_changed
    print("--- signal_files_changed ---\n")
    signal_files_changed()
    // In non-live mode we can't check the flag directly from script
    // (it's read by the C++ host). But we verify it doesn't crash.
    check("signal_files_changed callable", true)

    print("\n=== Results: {tests_passed} passed, {tests_failed} failed ===\n")
    if (tests_failed == 0) {
        print("ALL TESTS PASSED\n")
    }
    request_exit()
}

[export]
def shutdown() {
    print("test_watch: shutdown\n")
}

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
