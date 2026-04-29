from "frp" import Watched, Computed, update_deferred

// Pattern from active_matter: functions that create Computed instances
// e.g. isPreparationOpened_ = @(subid) Computed(@() ...)

// Test 1: Factory creating Computed with captured parameter
let currentMode = Watched("idle")
let mkModeCheck = @(targetMode) Computed(@() currentMode.get() == targetMode)

let isIdle = mkModeCheck("idle")
let isBusy = mkModeCheck("busy")
let isError = mkModeCheck("error")
update_deferred()
println($"idle={isIdle.get()}, busy={isBusy.get()}, error={isError.get()}")

currentMode.set("busy")
update_deferred()
println($"idle={isIdle.get()}, busy={isBusy.get()}, error={isError.get()}")

// Test 2: Factory creating multiple Computed from different sources
let scores = Watched({math = 90, eng = 85, sci = 70})
let mkScoreGetter = @(subject) Computed(@() scores.get()?[subject] ?? 0)

let mathScore = mkScoreGetter("math")
let engScore = mkScoreGetter("eng")
let sciScore = mkScoreGetter("sci")
update_deferred()
println($"math={mathScore.get()}, eng={engScore.get()}, sci={sciScore.get()}")

scores.set({math = 95, eng = 85, sci = 80})
update_deferred()
println($"math={mathScore.get()}, eng={engScore.get()}, sci={sciScore.get()}")

// Test 3: Computed depending on factory-created Computed
let avgScore = Computed(@() (mathScore.get() + engScore.get() + sciScore.get()) / 3)
update_deferred()
println($"avg: {avgScore.get()}")

// Test 4: Dynamic array of Computed from factory
let source = Watched(10)
let multipliers = [1, 2, 3, 4, 5]
let results = multipliers.map(@(m) Computed(@() source.get() * m))
update_deferred()

let vals = results.map(@(c) c.get())
println($"factory array: {", ".join(vals.map(@(v) v.tostring()))}")

source.set(3)
update_deferred()
let vals2 = results.map(@(c) c.get())
println($"factory array after: {", ".join(vals2.map(@(v) v.tostring()))}")
