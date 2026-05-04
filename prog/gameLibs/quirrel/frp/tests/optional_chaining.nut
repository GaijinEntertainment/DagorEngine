from "frp" import Watched, Computed, update_deferred

// Pattern from active_matter: optional chaining on Watched values
// Common: .get()?.property?.subproperty

// Test 1: Simple optional chaining
let data = Watched(null)
let name = Computed(@() data.get()?.name ?? "none")
update_deferred()
println($"null data: {name.get()}")

data.set({name = "Alice"})
update_deferred()
println($"with name: {name.get()}")

data.set({})
update_deferred()
println($"no name field: {name.get()}")

// Test 2: Deep optional chaining
let config = Watched(null)
let deepVal = Computed(@() config.get()?.section?.subsection?.value ?? -1)
update_deferred()
println($"deep null: {deepVal.get()}")

config.set({section = {subsection = {value = 42}}})
update_deferred()
println($"deep set: {deepVal.get()}")

config.set({section = {}})
update_deferred()
println($"partial: {deepVal.get()}")

// Test 3: Optional chaining with fallback between two Watched sources
// Pattern: queueRaid.get()?.id ?? selectedRaid.get()?.id
let primary = Watched(null)
let fallback = Watched({id = 99})
let activeId = Computed(@() primary.get()?.id ?? fallback.get()?.id ?? 0)
update_deferred()
println($"fallback id: {activeId.get()}")

primary.set({id = 5})
update_deferred()
println($"primary id: {activeId.get()}")

primary.set(null)
fallback.set({id = 77})
update_deferred()
println($"back to fallback: {activeId.get()}")

// Test 4: Computed with table indexing via Watched key
let store = Watched({a = 10, b = 20, c = 30})
let key = Watched("b")
let lookup = Computed(@() store.get()?[key.get()] ?? 0)
update_deferred()
println($"lookup b: {lookup.get()}")

key.set("c")
update_deferred()
println($"lookup c: {lookup.get()}")

key.set("missing")
update_deferred()
println($"lookup missing: {lookup.get()}")
