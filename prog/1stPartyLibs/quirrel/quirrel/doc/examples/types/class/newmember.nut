class Foo {}
println("Foo.newmember(\"k\", \"v\") == Foo =", Foo.newmember("k", "v") == Foo)   // returns the class, so calls chain
println("Foo.k =", Foo.k)

// a third argument makes the member static: one value shared by all instances
class Bar {}
Bar.newmember("shared", 42, true)
println("Bar().shared =", Bar().shared)

// a locked class rejects every new member, a static one too
class Locked { x = 1 }
Locked.lock()
try { Locked.newmember("nope", 1, true) } catch (e) { println("newmember(\"nope\", 1, true) error:", e) }
