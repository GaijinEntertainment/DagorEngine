from "async" import Future

// (1) A faulting race loser raises no spurious unhandled diagnostic: every input
//     is awaited by a combinator child, which marks it handled and catches its
//     fault. (2) The only fault that can surface is on a `done` the test
//     deliberately abandons -- exactly one ERROR block below, whose trace names
//     the faulting input's origin (adoption seeds done.faultTrace).
//
// This test relies on testRunner.py redirecting stdout and stderr to the .out.

async function main() {
  let winner = Future()
  winner.resolve("winner")
  async function faulter() { throw "loser-suppressed" }
  let loser = faulter()
  let r = await Future.race([winner, loser])
  print("race winner: " + r + "\n")
  print("body done\n")
}

async function abandon() {
  async function boomInput() { throw "abandoned-all-fault" }
  let _ = Future.all([boomInput()])  // never awaited -> its `done` surfaces
}

main()
abandon()
print("script done\n")
