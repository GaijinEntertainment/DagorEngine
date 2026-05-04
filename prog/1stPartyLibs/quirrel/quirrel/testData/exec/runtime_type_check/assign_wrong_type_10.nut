local x = null

function fn(x, v: string|table = x) {
    return v
}

fn(0, "123")
