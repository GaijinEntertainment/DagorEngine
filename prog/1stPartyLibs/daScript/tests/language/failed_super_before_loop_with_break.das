// super() outside loop + super() inside loop on a break-terminated path.
// Without subtree scanning, the break collapses the inner super to dead() and the lint
// would miss the path that calls super twice (outer + once inside before break).
options gen2
expect 30308

class Base {
    x : int = 0
    def Base {
        x = 13
    }
}

class Derived : Base {
    y : int = 0
    def Derived(n : int) {
        // 30308: on any iteration where the break fires after super(), the function
        // calls super twice — outer super (a) + inner super (b). The CFG walker
        // collapses the break to dead(), so the inner super is invisible to the
        // straight-line fold; subtreeHasSuperCall catches it at the loop boundary.
        super()                        // (a)
        for (i in range(n)) {
            if (i == 0) {
                super()                // (b) — runs once then breaks
                break
            }
        }
        y = 1
    }
}

[export]
def main {
    let d = new Derived(3)
    feint("d.x={d.x}\n")
}
