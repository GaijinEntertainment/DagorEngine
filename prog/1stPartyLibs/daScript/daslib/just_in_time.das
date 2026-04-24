options gen2
options indenting = 4

module just_in_time shared private

//! JIT compilation configuration.
//!
//! Bootstraps the Just-In-Time compiler by requiring the platform-appropriate
//! LLVM-based JIT backend module.

/*
    this is a JIT configuration file. We require platform appropriate jit module, as well as set necessary options.
*/

require llvm/daslib/llvm_macro
