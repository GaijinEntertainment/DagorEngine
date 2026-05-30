let counter = {
  count = 0
  async function bump(n) {
    this.count = this.count + n
    return this.count
  }
}

async function main() {
  println($"bump1={await counter.bump(3)}")
  println($"bump2={await counter.bump(4)}")
}
main()
