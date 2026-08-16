options gen2
expect 31403

// A static_assert with no message of its own, so the compiler has to supply the default one.

[export]
def main {
    static_assert(false)
}
