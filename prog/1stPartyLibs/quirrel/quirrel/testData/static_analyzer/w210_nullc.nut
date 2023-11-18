function foo() {}
function bar(_a, _b, _c) {}

let requestedCratesContent = {}

let function requestCratesContent(armyId, crates) {
    if ((armyId ?? "") == "")
      return
    let requested = foo()
    let armyCrates = requested.value?[armyId]
    // if (armyId not in requested)
    //     requested[armyId] <- {}
    let armyRequested = requested[armyId]
    let toRequest = crates.filter(@(c) c not in armyCrates && c not in armyRequested)
    if (toRequest.len() == 0)
      return
    toRequest.each(@(c) armyRequested[c] <- true)
    bar(armyId, toRequest, function(res) {
      toRequest.each(function(c) { if (c in armyRequested) delete armyRequested[c] })
      if ("content" in res)
        requestedCratesContent.mutate(@(cc) cc[armyId] <- (cc?[armyId] ?? {}).__merge(res.content))
    })
  }
