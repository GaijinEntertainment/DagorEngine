// `await` on a known sync call is a no-op: emits a redundant-await warning.

function plain(): int {
  return 1
}

async function main() {
  let n = await plain()
  print(n)
}

main()
