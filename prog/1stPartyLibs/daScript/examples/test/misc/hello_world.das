class Foo
    static a : int
    def static bar ( A : auto )
        a = int(A)
        print("set to {a}\n")

[export]
def main
    Foo`bar(13.1)


