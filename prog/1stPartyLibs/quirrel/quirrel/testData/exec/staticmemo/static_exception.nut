local div = 0

function fn() {
  try {
    let tab = static { x = 123 / div }
    println(tab.x)
  } catch (e) {
    println("exception")
  }
}

fn()
fn()

div = 2
fn()
fn()
