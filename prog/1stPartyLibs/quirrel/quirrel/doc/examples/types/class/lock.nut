class Foo { x = 1 }
Foo.lock()

try { Foo.rawset("z", 1) } catch (e) { println("rawset(\"z\", 1) error:", e) }

// a locked class is read-only: a method is rejected too
try { Foo.newmember("helper", @() 42) } catch (e) { println("newmember(\"helper\", ...) error:", e) }
