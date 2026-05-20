// main_escape.das — security regression test for %/ path traversal.
//
// `%/` is supposed to root the require at get_das_root(). Without the
// extra `..`-segment guard in FsFileAccess::getModuleInfo, the path
// `%/../etc/passwd.das` would normalize to `<dasroot's parent>/etc/passwd.das`
// and escape the root. The resolver now returns an empty ModuleInfo for any
// `%/` require whose normalized form contains a `..` segment, which the
// caller reports as MissingPrerequisite; this file therefore fails to compile.
//
// (The bogus `%//etc/passwd.das` form does NOT exercise the guard — `//` is a
// line comment, so the lexer drops the rest of the line and the require
// becomes a syntax error before resolution. Use `%/../...` for this test.)

options gen2

require %/../etc/passwd.das

[export]
def test {
    return true
}
