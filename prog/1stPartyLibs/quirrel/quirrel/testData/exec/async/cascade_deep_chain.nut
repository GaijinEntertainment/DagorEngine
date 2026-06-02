from "async" import Future

// N-deep adopter chain: chain[i+1] adopts chain[i] while chain[i] is
// still L_Open, so chain[i+1] lands directly in chain[i]->waiters.
// Settling chain[0] then has to cascade through all N links. Pre-fix
// (recursive settleTerminal) this overflows the C stack.

let N = 1000

let chain = array(N + 1, null)
for (local i = 0; i <= N; i++)
  chain[i] = Future()

for (local i = N; i >= 1; i--)
  chain[i].resolve(chain[i-1])

chain[0].resolve("ok")

print("chain[0]=")
print(chain[0].getState())
print(" chain[")
print(N / 2)
print("]=")
print(chain[N / 2].getState())
print(" chain[")
print(N)
print("]=")
print(chain[N].getState())
print("\n")
print("script done\n")
