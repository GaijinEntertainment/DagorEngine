# Getting started with Daslang

Read the installation steps to get Daslang for your system, then read the
tutorial.

## Prerequisites

We are going to need git, cmake, vscode and a C++ compiler for your platform.

## Build the compiler

```sh
git clone https://github.com/GaijinEntertainment/daScript
cd daScript
cmake -B build -G "Unix Makefiles"
cd build && make daScript -j$(nproc)
```

You may also want to create a symlink in the `/bin` directory for easy access
to the compiler.

```
sudo ln -s /home/andrew/daScript/bin/daScript /bin/das
```

## Set up VSCode

Currently, the best supported editor is VSCode.

To set up the language server follow these instructions: [daScript-plugin](https://github.com/profelis/daScript-plugin#installation)

Hint: if the autocompletion doesn't seem to work, try increasing
`dascript.server.connectTimeout` plugin parameter up to 10 (or 20 in case of
Debug build).

## Compile a small program

Now let's test what we've got:

1. Start small, an obligatory Hello world

```
# File: hello_world.das
# Run: das hello_world.das

[export]
def main
    print("Hello world\n")
```

In case you are wondering `[export]` annotation here ensures that function can be called externally given context.
