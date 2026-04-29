options gen2

enum Color {
    Red
    Green
    Blue
}

struct Point {
    x : float
    y : float
}

def distance(a, b : Point) : float {
    let dx = a.x - b.x
    let dy = a.y - b.y
    return dx * dx + dy * dy
}

bitfield Perms : uint {
    read
    write
}

struct FileInfo {
    perm : Perms
}

[export]
def main() {
    var p1 = Point(x = 1.0, y = 2.0)
    var p2 = Point(x = 4.0, y = 6.0)
    let d = distance(p1, p2)
    print("{d}\n")
    let c = Color.Green
    print("{c}\n")
    var fi = FileInfo(perm = Perms.read)
    print("{fi.perm}\n")
}
