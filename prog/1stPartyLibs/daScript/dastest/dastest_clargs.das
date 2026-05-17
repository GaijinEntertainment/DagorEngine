module dastest_clargs shared

require daslib/clargs

[CommandLineArgs]
struct DastestArgs {
    @clarg_doc = "Path to the single script file to run tests in isolated mode"
    run : string

    @clarg_doc = "Only print output for failed tests, suppressing successful ones"
    failures_only : bool

    @clarg_doc = "Compile test files without executing them"
    compile_only : bool

    @clarg_doc = "If tests run longer than this duration in seconds, panic. If 0, the timeout is disabled. The default is 10 minutes"
    timeout : float = 600.0

    @clarg_doc = "Print the top N slowest test files and highlight statistical timing outliers (above mean + 2x stddev)"
    timing_outliers : int = 0

    @clarg_doc = "Path to write the JSON test results report to"
    json_file : string

    @clarg_doc = "Run tests in isolated processes, useful to catch crashes"
    isolated_mode : bool

    @clarg_doc = "Path to write coverage report data to"
    cov_path : string

    @clarg_doc = "Project file. Will be used to compile given tests"
    test_project : string

    @clarg_doc = "Number of threads to use in isolated mode; defaults to 2x the number of hardware threads when 0"
    isolated_mode_threads : int

    @clarg_doc = "Run all benchmarks this number of times (useful for sample collection)"
    count : int = 1

    @clarg_doc = "Per-test timeout in seconds when running in isolated mode"
    test_timeout : float

    @clarg_doc = "Path to serialize compiled test programs to (for use with --deser)"
    ser : string

    @clarg_doc = "Path to a previously serialized program file to load and run tests from (see --ser)"
    deser : string

    @clarg_name = "test"
    @clarg_doc = "Path to the folder with scripts or single script to test"
    test_files : array<string>

    @clarg_doc = "Run top-level tests matching this name prefix"
    test_names : array<string>

    @clarg_doc = "Run top-level benchmarks matching this name prefix"
    bench_names : array<string>

    @clarg_doc = "Print verbose output"
    verbose : bool

    @clarg_doc = "Print uri paths instead of file paths (vscode friendly)"
    uri_paths : bool

    @clarg_doc = "Compile and run tests using AOT (ahead-of-time) compilation, failing if AOT is unavailable"
    use_aot : bool

    @clarg_doc = "Enable benchmark execution (all of them)"
    bench : bool

    @clarg_doc = "Set cop.debugger so context.debugger is truthy, exercising _Debug SimNode variants"
    debugger : bool = false

    @clarg_doc = "Set cop.keep_alive so _KeepAlive SimNode variants get dispatched"
    keep_alive : bool = false

    // Could use an enum here, but the strings would
    // be different from the original ones, so may
    // not be worth it.
    @clarg_doc = "Specifies the benchmark output format (\"native\", \"go\" or \"json\")"
    bench_format : string
}
