let function riIsEmptyGroup(x) {
    let list = x?.list
    let a = list != null && list.len() == 0
    let b = list != null || list.len() == 0

    return a && b;
  }