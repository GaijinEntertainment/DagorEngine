// cdp/common/metalink.das — Stub module in a subdirectory
//
// Required by both actions.das and profile.das via the dotted name
// `common.metalink`. The resolver splits the dots into directory separators:
// "common.metalink" -> "common/metalink.das". This tests basic dotted
// module name resolution.

options gen2
def fn_metalink {
    return true
}
