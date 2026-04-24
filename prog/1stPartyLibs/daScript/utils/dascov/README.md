# Code coverage

A command-line tool for measuring code coverage of das code.

## Overview

dascov support two options:
- brief output to stdout
- output in lcov format to a file

Key capabilities:

- Measures per file code coverage
- Measures per file per function code coverage
- Measures branches coverage

## Usage
### Standalone tool
To measure coverage of the command:
```
daslang main.das -- <command>
```
It should be replaced with one of the:
```
daslang utils/dascov/main.das -- - main.das <command>
daslang utils/dascov/main.das -- cov.lcov main.das <command>
```

Brief coverage for file:
```
options gen2

var x = 123

[export]
def main() {
    x += 1
    if (x > 1) {
        print("hit")
    } else {
        print("not hit")
    }
}
```
Looks like this:
```
Coverage Summary
==============================================
File            Hits      Missed    Coverage
----------------------------------------------
main.das        3         1         75%
  main          3         1         75%
----------------------------------------------
TOTAL           3         1         75%
```

Coverage can be recovered from recorded file as well. For example dastest
creates coverage.lcov, and it can be parsed via:
```
# ./bin/daslang utils/dascov/main.das -- cov.txt --exclude tests/
Coverage Summary
=====================================================================================================================================
File                                                                                                   Hits      Missed    Coverage
-------------------------------------------------------------------------------------------------------------------------------------
/home/alexey-churkin/daScript2/daScript2/daslib/fuzzer.das                                             145       0         100%
  fuzzer`fuzz_numeric_op3`10015655030841158810                                                         4         0         100%
  fuzzer`fuzz_numeric_vec_scal_op2`10935617188872172278                                                9         0         100%
  fuzzer`fuzz_int_vector_op2`14625781784477760102                                                      8         0         100%

```

### Macro coverage
`require daslib/coverage` can be added to your project.
This will include macro and add coverage instrumentation to your project.

Results can be obtained via:
```
fill_report()
get_summary_string()
```

<output coverage> can either be a `-` to output to stdout in brief format
or filename to store coverage in `lcov` format:
