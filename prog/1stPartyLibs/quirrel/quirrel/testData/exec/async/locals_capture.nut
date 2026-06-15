// Depth-0 locals capture on the origin walk: an async-rooted throw captures its
// throw frame's in-scope locals (params included) as structured records on each
// captured trace frame. The fault is fire-and-forgotten, so the default handler
// renders the trace (with locals) on the pump-tick unhandled sweep. Verifies the
// three tiers (scalar by value, string by ref, aggregate by ref-free summary),
// the block-scope filter, the no-live-ref (GC-safety) property for aggregates,
// and the render-time string clamp.

local LONG = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789ABCDEF"

class Foo {}

function makeBug(resp, userId) {
    local n = 0
    local ratio = 1.5
    local ok = false
    local items = [10, 20, 30]
    local cfg = { a = 1, b = 2 }
    local obj = Foo()
    local big = LONG
    if (n == 0) {
        local hidden = 99            // closes before the throw -> out of scope
        items.append(hidden)
    }
    throw "boom"
}

async function failing() {
    makeBug({ status = 200 }, "abc123")
}

failing()   // fire-and-forget, unhandled -> default handler renders the captured trace
print("script done\n")
