options gen2

// Tiny standalone program used by tests/exe-paths/CMakeLists.txt to verify
// the standalone exe's 3-tier dynamic-module resolution (exe_dir → das_root →
// baked absolute). Requires dasSQLITE — if the module fails to load at startup
// the open call would surface an error / crash. Prints "ok\n" on success so
// the integration test can grep for it.

require sqlite/sqlite_boost

[export]
def main {
    var db : sqlite3?
    let rc = sqlite3_open(":memory:", unsafe(addr(db)))
    if (rc != 0) {
        print("open failed: {rc}\n")
        return
    }
    sqlite3_close(db)
    print("ok\n")
}
