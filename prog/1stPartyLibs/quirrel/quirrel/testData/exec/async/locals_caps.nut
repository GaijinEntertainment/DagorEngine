// Per-frame locals cap: a frame with more in-scope locals than the capture cap
// records at most the cap and marks the omitted count, so the fault path never
// produces an unbounded capture. Fire-and-forgotten so the default handler renders
// the capped shape (with the "... (N more)" marker) on the unhandled sweep.

function manyLocals() {
    local a0=0;  local a1=1;  local a2=2;  local a3=3;  local a4=4
    local a5=5;  local a6=6;  local a7=7;  local a8=8;  local a9=9
    local b0=10; local b1=11; local b2=12; local b3=13; local b4=14
    local b5=15; local b6=16; local b7=17; local b8=18; local b9=19
    local s = a0+a1+a2+a3+a4+a5+a6+a7+a8+a9+b0+b1+b2+b3+b4+b5+b6+b7+b8+b9
    throw "caps " + s
}

async function failing() {
    manyLocals()
}

failing()   // fire-and-forget, unhandled -> default handler renders the capped trace
print("script done\n")
