options gen2

require live/live_vars
require live_host

// Tag globals with @live — the macro auto-generates [before_reload]/[after_reload]
// that serialize these via Archive. No manual save/restore needed.

var @live counter = 0
var @live name = "world"
var @live values : array<float>

[export]
def init() {
    if (!is_reload()) {
        counter = 0
        name = "world"
        values |> clear()
        values |> push(1.0)
        values |> push(2.0)
        values |> push(3.0)
        print("init: first run\n")
    } else {
        print("init: reload — counter={counter}, name={name}, values count={length(values)}\n")
    }
}

[export]
def update() {
    counter++
    if (counter % 60 == 0) {
        print("tick: counter={counter}, name={name}, values={length(values)}\n")
    }
}

[export]
def shutdown() {
    print("shutdown: counter={counter}\n")
}

[export]
def main() {
    print("live_vars_demo: running in simple mode\n")
    init()
    for (i in range(120)) {
        update()
    }
    shutdown()
}
