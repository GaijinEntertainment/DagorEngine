options gen2

require daslib/math_boost

[unused_argument(box)] def check_box_ptr(box : AABB ?) {}

[unused_argument(box)] def check_box(box : AABB) {}

[export]
def main() {
    let b : AABB
    check_box(b)
    var b_ptr : AABB?
    check_box_ptr(b_ptr)
}
