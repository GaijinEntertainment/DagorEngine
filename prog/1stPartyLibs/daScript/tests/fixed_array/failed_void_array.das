// Stage 0 characterization (FIXED_ARRAY_REWORK.md): array of void is a compile error.
options gen2
expect 30108

[export]
def main {
    var a : void[4]
}
