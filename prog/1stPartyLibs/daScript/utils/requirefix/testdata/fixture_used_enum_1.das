options gen2

require strings

// strings is required and ConversionResult enum value is used.

[export]
def main() {
    let r = ConversionResult.ok
    verify(int(r) == 0)
}
