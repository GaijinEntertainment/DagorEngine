//expect:w275

let function fn(x) { //-declared-never-used
  switch (x) {
    case 0:
    case 1:
    case 2:
      print("aaa")
      if (x == 1) {
        return
      } else {
        print("123")
        return
      }
    case 3:
      print("bbb")
      break
    case 4:
      print("ccc")
    default:        // <<<<<<< here
      print("ddd")
  }
}
