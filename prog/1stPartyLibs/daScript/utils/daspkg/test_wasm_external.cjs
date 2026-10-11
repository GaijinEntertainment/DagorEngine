const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");

const sdk = path.resolve(__dirname, "../..");
const binary =
  process.env.DASLANG_BIN ||
  path.join(
    sdk,
    "bin",
    process.platform === "win32" ? "daslang.exe" : "daslang",
  );
const libraries =
  process.env.DASLANG_WASM_LIB || path.join(sdk, "web/output64/lib");
const logs =
  process.env.TEST_ARTIFACT_DIR || path.join(sdk, "logs/daspkg-wasm-external");
fs.mkdirSync(logs, { recursive: true });
const temporary = fs.mkdtempSync(
  path.join(os.tmpdir(), "daspkg-wasm-external-"),
);
const input = path.join(temporary, "source"),
  output = path.join(temporary, "out");
fs.mkdirSync(input);
function manifest(source) {
  fs.writeFileSync(
    path.join(input, ".das_package"),
    `options gen2
require daslib/daspkg
[export]
def package() { package_name("external_fixture") }
[export]
def release() {
    release_main("main.das")
    release_include_from("${source}", "nested/readme.txt")
}
`,
  );
}
function build(label) {
  const started = Date.now();
  const result = spawnSync(
    binary,
    [
      path.join(sdk, "utils/daspkg/main.das"),
      "--",
      "release",
      "wasm",
      "--root",
      input,
      "--out",
      output,
      "--wasm-lib-dir",
      libraries,
    ],
    {
      cwd: sdk,
      encoding: "utf8",
      timeout: 180000,
      maxBuffer: 16 * 1024 * 1024,
    },
  );
  const text = (result.stdout || "") + (result.stderr || "");
  fs.writeFileSync(path.join(logs, `${label}.log`), text);
  assert.ifError(result.error);
  assert.equal(result.signal, null, `${label}: terminated by ${result.signal}`);
  console.log(
    `${label}: exit ${result.status}, ${(Date.now() - started) / 1000}s`,
  );
  return { ...result, text };
}
try {
  fs.writeFileSync(
    path.join(input, "main.das"),
    'options gen2\n[export]\ndef main() { print("external fixture\\n") }\n',
  );
  manifest("README.md");
  const success = build("success");
  assert.equal(success.status, 0, `release failed; see ${logs}`);
  const bundle = path.join(output, "external_fixture");
  const copied = path.join(bundle, "nested/readme.txt");
  assert.deepEqual(
    fs.readFileSync(copied),
    fs.readFileSync(path.join(sdk, "README.md")),
  );
  assert.ok(fs.existsSync(path.join(bundle, "external_fixture.wasm")));

  manifest("missing-external-fixture-source.txt");
  const missing = build("missing-source");
  assert.notEqual(missing.status, 0);
  assert.match(missing.text, /release_include_from.*does not exist/);
  assert.doesNotMatch(missing.text, /released external_fixture ->/);
  assert.ok(
    !fs.existsSync(copied),
    "failed release does not retain the previous external copy",
  );

  manifest("README.md");
  fs.writeFileSync(
    path.join(input, "main.das"),
    "options gen2\n[export]\ndef main() { undefined_fixture_function() }\n",
  );
  const compileFailure = build("compile-failure");
  assert.notEqual(compileFailure.status, 0);
  assert.ok(
    !fs.existsSync(copied),
    "external staging runs only after a successful build",
  );
  console.log("PASS: WASM external staging and build/copy failure propagation");
} finally {
  fs.rmSync(temporary, { recursive: true, force: true });
}
