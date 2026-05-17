options gen2

require daslib/math_boost

def check_box_ptr(box : AABB?) {}

def check_box(box : AABB) {}

[export]
def main() {
    var b : AABB
    check_box(b)
    var b_ptr : AABB?
    check_box_ptr(b_ptr)
}
