local y: array = []

function fn() {
    y.append(333)
    y = []
}

fn()

return y
