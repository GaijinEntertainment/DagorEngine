# C implementation of SHA-3 and Keccak with Init/Update/Finalize API

The purpose of this project is:

* provide an API that hashes bytes, not bits
* provide a simple reference implementation of a SHA-3 message digest algorithm, as defined in the [FIPS 202][fips202_standard] standard
* assist developers in the [Ethereum](https://www.ethereum.org/) blockchain ecosystem by providing the Keccak function used there
* implement the hashing API that employs the __IUF__ paradigm (or `Init`, `Update`, `Finalize` style)
* answer the design questions, such as:
  * what does the state for IUF look like?
  * how small can the state be (224 bytes on a 64-bit system for a unified SHA-3 algorithm)
  * what is the incremental cost of adding e.g. SHA3-384 to a SHA3-256 implementation?

The implementation is written in C and uses `uint64_t` types to manage the SHA-3 state. The code will compile and run on 64-bit and 32-bit architectures (`gcc` and `gcc -m32` on `x86_64` were tested).

[fips202_standard]: http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf "FIPS 202 standard"

## License, prior work

This work is licensed with a standard [MIT license](LICENSE). I appreciate, but do not require, any attribution to this work if you used the code or ideas. I thank you for this in advance.

This is a clean-room implementation of IUF API for SHA3. The `keccakf()` is based on the code from [keccak.noekeon.org](http://keccak.noekeon.org/).

1600-bit message hashing test vectors are [NIST test vectors](http://csrc.nist.gov/groups/ST/toolkit/examples.html).

## Overview of the API

Let's hash 'abc' with SHA3-256 using two methods: single buffer (but using IUF paradigm), and using the IUF API. 

    sha3_context c;
    uint8_t *hash;

Single-buffer hashing:

    sha3_Init256(&c);
    sha3_Update(&c, "abc", 3);
    hash = sha3_Finalize(&c);
    // 'hash' points to a buffer inside 'c'
    // with the value of SHA3-256

Alternatively, IUF hashing:

    sha3_Init256(&c);
    sha3_Update(&c, "a", 1);
    sha3_Update(&c, "bc", 2);
    hash = sha3_Finalize(&c);

    // no free for 'c' is needed

The `hash` points to the same `256/8=32` bytes in both cases.

There is also a single-call hashing API:

    sha3_HashBuffer(256, SHA3_FLAGS_KECCAK, "abc", 3, out, sizeof(out));
    // out contains 256 bits of Keccak256, or less if sizeof(out)<32

## How to use Keccak version

Call `sha3_SetFlags(&c, SHA3_FLAGS_KECCAK)` immediately after `sha3_InitX` or no later than `sha3_Finalize`. This change cannot be undone for the given hash context.

## Building

    $ make

See `Makefile` for details. See also below for specific examples.

## Self-tests

    $ make test
    Keccak-256 tests passed OK
    SHA3-256, SHA3-384, SHA3-512 tests passed OK

or 

    $ make CFLAGS=-m32 LDFLAGS=-m32 test
    Keccak-256 tests passed OK
    SHA3-256, SHA3-384, SHA3-512 tests passed OK

There is also `sha3sum` test program that takes following parameters:

    sha3sum 256|384|512 file_path 

or for Keccak version:

    sha3sum 256|384|512 -k file_path 

### SHA-3 / Linux sha3sum example

    $ touch empty.txt
    $ gcc -Wall sha3.c sha3sum.c -o sha3sum && ./sha3sum 256 empty.txt
    a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a  empty.txt

Compare with Linux `sha3sum`:

    $ sha3sum -a 256 empty.txt
    a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a  empty.txt

### Keccak256 / Solidity example

    $ echo -n "abc" > abc
    $ sha3sum 256 -k abc
    4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45  abc

This corresponds to the result obtained in Solidity JavaScript test framework.

     console.log(web3.utils.sha3('abc'));
     // prints 0x4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45

## API

* the same `sha3_context` object maintains the state for SHA3-256, SHA3-384, or SHA3-512 algorithm;
* the hash algorithm used is determined by how the context was initialized with `sha3_InitX`, e.g. `sha3_Init256`, `sha3_Init384`, or `sha3_Init512` call;
* `sha3_Update` and `sha3_Finalize` are the same for regardless the type of the algorithm (`X`);
* the buffer returned by `sha3_Finalize` will have `X` bits of hash;
* `sha3_InitX` also works as Reset (zeroization) of the hash context; no Free function is needed;

See [`sha3.h`](sha3.h) for the exact interface.

## API fuzzing

    $ fuzz/run.sh

The fuzzing script expects clang installed.

## Credits

Thanks to @ralight for moving the test code into separate `sha3test.c`

## Notes

SHA3-224 is not supported, but can easily be added.

The code was written to work with the Microsoft Visual Studio compiler (under `_MSC_VER`), but this build target was not tested.

This project was created to support [SHA3 in OpenPGP](https://tools.ietf.org/html/draft-jivsov-openpgp-sha3) work, but it applies to other protocols and formats, e.g. TLS.

