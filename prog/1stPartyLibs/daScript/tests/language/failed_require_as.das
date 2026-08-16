// Aliasing two different modules to the same name is an error (20510). Reachable for
// non-path requires only once explicit `as` registers unconditionally — before the fix
// both aliases were silently dropped and this file compiled clean.
options gen2

expect 20510:1

require math as clash
require daslib/strings_boost as clash

[export]
def main() {
}
