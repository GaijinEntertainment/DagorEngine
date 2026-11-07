from "async" import delay, nextFrame

// N `await nextFrame()` take N pumps, also when each runs inside a pump.
// The ticker counts pumps: each delay(0) resolves on the next one.

const STEPS = 4
local pumps = 0
local done = false

async function ticker() {
  while (!done) {
    await delay(0)
    pumps++
  }
}

async function main() {
  for (local i = 1; i <= STEPS; i++) {
    await nextFrame()
    println($"step {i} at pump {pumps}")
  }
  done = true
}

ticker()
main()
