from "async" import Promise

// Negative coverage: NO "[sqasync] unhandled promise rejection" line may
// appear. (1) a resolved fire-and-forget async fn is not a rejection.
// (2) a rejection that IS awaited (and caught) is handled, so silent.

async function resolves() {
  return 42
}

async function rejects() {
  throw "handled"
}

async function consumer() {
  try {
    await rejects()
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

resolves()   // fire-and-forget, fulfilled -> no diagnostic
consumer()   // awaits rejects(), catches -> rejects() is awaited -> no diagnostic
print("script done\n")
