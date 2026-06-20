options gen2

require daslib/math_boost

[unused_argument(box)] def check_box(box : array<AABB>) {}

[export]
def main() {
    let b : array<AABB>
    check_box(b)
}
