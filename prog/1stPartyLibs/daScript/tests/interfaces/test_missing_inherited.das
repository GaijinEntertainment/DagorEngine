options gen2
// Test: completeness check catches missing inherited abstract methods
expect 30111

require daslib/interfaces

[interface]
class IBase {
    def abstract foo : int
}

[interface]
class IChild : IBase {
    def abstract bar : string
}

// Provides bar but NOT foo (inherited from IBase) — should fail
[implements(IChild)]
class Partial {
    def Partial() {
        pass
    }
    def IChild`bar() : string {
        return "bar"
    }
}
