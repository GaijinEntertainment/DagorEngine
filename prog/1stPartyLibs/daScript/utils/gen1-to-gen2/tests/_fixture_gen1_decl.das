options gen2 = false

typedef
    IntAlias = int
    StrAlias = string

enum Color
    red
    green

bitfield Flags
    one
    two

struct Point
    x : int = 1
    y : float = 2.0

class Shape
    kind : int = 0
    def area() : int
        return kind
