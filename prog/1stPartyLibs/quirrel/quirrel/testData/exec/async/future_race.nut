from "async" import Future

// Future.race: the first input to settle wins, for both fulfilment and fault.

async function main() {
  // Genuine stagger: `loser` (array-first) never settles, `winner` fulfils, so
  // race resolves with the winner -- proving it is settle-time, not array order.
  {
    let winner = Future()
    let loser = Future()  // never settles
    async function go() { winner.resolve("winner") }
    go()
    let r = await Future.race([loser, winner])
    print("race fulfil: " + r + "\n")
  }

  // First to fault wins: `boom` faults, the other input never settles, so race
  // adopts the fault and the await throws.
  {
    async function boom() { throw "race-boom" }
    let pending = Future()  // never settles
    try {
      let r = await Future.race([pending, boom()])
      print("UNEXPECTED fulfil: " + r + "\n")
    } catch (e) {
      print("race fault: " + e + "\n")
    }
  }

  // Tie-break: both inputs already settled before any child runs. Children resume
  // in array order, so the array-first input wins (matches JS Promise.race).
  {
    let r1 = Future()
    let r2 = Future()
    r1.resolve("A")
    r2.resolve("B")
    let r = await Future.race([r1, r2])
    print("race tiebreak: " + r + "\n")
  }

  // Non-future element resumes its child immediately (await passthrough), so it
  // wins the race over a still-pending future input.
  {
    let pending = Future()  // never settles
    let r = await Future.race([pending, 42])
    print("race passthrough: " + r + "\n")
  }

  print("script done\n")
}
main()
