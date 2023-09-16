require daslib/unroll

options log

[export]
def main
    unroll <|
        for i in range(5)
            print("i = {i}\n")
