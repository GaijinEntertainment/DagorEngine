options gen2
// Test: completeness check catches missing abstract methods
expect 30111

require daslib/interfaces

[interface]
class IFoo {
    def abstract bar : int
    def abstract baz : string
}

// Missing both bar and baz — should produce compile error 30111
[implements(IFoo)]
class Incomplete {
    def Incomplete() {
        pass
    }
}
