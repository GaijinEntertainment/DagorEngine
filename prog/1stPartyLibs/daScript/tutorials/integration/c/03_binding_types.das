options gen2

// Companion script for the "Binding C Types" tutorial.
// This script uses types and functions defined in the C module "tutorial_c_03".

require tutorial_c_03

[export]
def test() {
    // Use the bound enumeration
    let color = Color.green
    print("color = {color}\n")

    // Use the bound alias
    var numbers : IntArray
    push(numbers, 10)
    push(numbers, 20)
    push(numbers, 30)
    print("numbers = {numbers}\n")

    // Use the bound structure
    var p : Point2D
    p.x = 3.0
    p.y = 4.0
    print("point = ({p.x}, {p.y})\n")

    // Call the bound C function
    let d = point_distance(p)
    print("distance from origin = {d}\n")

    // Call another bound C function
    let s = point_to_string(p)
    print("as string: {s}\n")
}
