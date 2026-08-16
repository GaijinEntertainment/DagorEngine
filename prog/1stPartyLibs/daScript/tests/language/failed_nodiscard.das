// A genuinely discarded [nodiscard] result is still an error — the interpolation fix marks
// string-builder elements only, not statement-level discards.
options gen2

expect 30166:1

[nodiscard]
def answer : int => 42

[export]
def main() {
    answer()
}
