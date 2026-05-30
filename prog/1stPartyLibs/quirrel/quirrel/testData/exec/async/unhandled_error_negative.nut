// Negative coverage: NO "[sqasync] unhandled error" line may appear.
// (1) a resolved fire-and-forget async fn is not a fault.
// (2) a fault that IS awaited (and caught) is handled, so silent.

async function resolves() {
  return 42
}

async function faults() {
  throw "handled"
}

async function consumer() {
  try {
    await faults()
  } catch (e) {
    print("caught: " + e + "\n")
  }
}

resolves()   // fire-and-forget, fulfilled -> no diagnostic
consumer()   // awaits faults(), catches -> faults() is awaited -> no diagnostic
print("script done\n")
