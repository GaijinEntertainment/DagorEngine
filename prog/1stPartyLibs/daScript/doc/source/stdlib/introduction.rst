.. _stdlib_introduction:

============
Introduction
============

The Daslang standard libraries consist in a set of modules implemented in C++.
While are not essential for the language, they provide a set of useful services that are
commonly used by a wide range of applications (file I/O, regular expressions, etc.),
plus they offer a foundation for developing additional libraries.

All libraries are implemented through the Daslang API and C++ runtime library.
The modules are organized in the following way:

* :doc:`builtin runtime <builtin>`
* :doc:`math basic mathematical routines <math>`
* :doc:`fio - file input and output <fio>`
* :doc:`random - LCG random mathematical routines <random>`

* :doc:`strings - string manipulation library <random>`
* :doc:`daslib/strings_boost - boost package for STRINGS <random>`

* :doc:`rtti - runtime type information and reflection library <rtti>`
* :doc:`ast - compilation time information, reflection, and syntax tree library <ast>`
* :doc:`daslib/ast_boost - boost package for AST <ast_boost>`

* :doc:`daslib/functional - high-order functions to support functional programming <functional>`

* :doc:`daslib/apply - apply reflection pattern <apply>`

* :doc:`daslib/json - JSON parser and writer <json>`
* :doc:`daslib/json_boost - boost package for JSON <json_boost>`

* :doc:`daslib/regex - regular expression library <regex>`
* :doc:`daslib/regex_boost - boost package REGEX <regex_boost>`

* :doc:`network - TCP raw socket server <network>`
* :doc:`uriparser - URI manipulation library <uriparser>`

* :doc:`daslib/rst - RST documentation support <rst>`
