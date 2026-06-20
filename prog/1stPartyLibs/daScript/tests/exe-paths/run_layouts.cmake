# Integration test driver — verifies daslang -exe's 3-tier dynamic-module
# resolution works for the three real-world deployment layouts.
#
# Inputs (passed via -D on the cmake -P invocation):
#   DASLANG       — path to daslang binary
#   SRC_DAS       — path to uses_sqlite.das (the test program)
#   SQLITE_MOD    — path to dasModuleSQLITE(.shared_module|*_debug.shared_module)
#   WORKDIR       — temporary working directory (test-specific)
#
# The script compiles SRC_DAS to a standalone exe, then drives it through:
#   1. Bundle layout      (exe at <bundle>/, modules at <bundle>/modules/...) —
#                         tier 1 wins (exe_dir resolution).
#   2. SDK install layout (exe at <root>/bin/, modules at <root>/modules/...) —
#                         tier 2 wins (das_root: getDasRoot strips bin/).
#   3. Local dev layout   (exe in build tree, baked absolute path still valid) —
#                         tier 3 wins (legacy absolute fallback).
#
# All three are positive — each asserts the exe loads dasSQLITE and prints "ok".
# A negative case (no modules anywhere) is omitted: tier 3 fallback would still
# find the source-tree module on the dev machine, so distinguishing "broken
# bundle" from "ok bundle" requires runtime override machinery that doesn't
# exist yet. Each run uses a NEUTRAL cwd to isolate from cwd-relative fallbacks.

if(NOT DASLANG OR NOT SRC_DAS OR NOT SQLITE_MOD OR NOT WORKDIR)
    message(FATAL_ERROR "run_layouts.cmake: missing required -D inputs")
endif()
if(NOT EXISTS "${DASLANG}")
    message(FATAL_ERROR "DASLANG not found: ${DASLANG}")
endif()
if(NOT EXISTS "${SRC_DAS}")
    message(FATAL_ERROR "SRC_DAS not found: ${SRC_DAS}")
endif()
if(NOT EXISTS "${SQLITE_MOD}")
    message(FATAL_ERROR "SQLITE_MOD not found: ${SQLITE_MOD}")
endif()

# Fresh workdir
file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")

# Step 1 — compile to standalone exe (daslang appends .exe)
set(EXE_BASE "${WORKDIR}/uses_sqlite")
message(STATUS "compiling: ${DASLANG} -exe ${SRC_DAS} -output ${EXE_BASE}")
execute_process(
    COMMAND "${DASLANG}" -exe "${SRC_DAS}" -output "${EXE_BASE}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(_rc)
    message(FATAL_ERROR "daslang -exe failed (rc=${_rc}):\n${_out}\n${_err}")
endif()
set(EXE "${EXE_BASE}.exe")
if(NOT EXISTS "${EXE}")
    message(FATAL_ERROR "compile produced no output at ${EXE}")
endif()

# Helper — run the exe from CWD, capture output, verify "ok\n"
function(run_layout SCENARIO EXE_PATH CWD)
    message(STATUS "[${SCENARIO}] exe=${EXE_PATH} cwd=${CWD}")
    execute_process(
        COMMAND "${EXE_PATH}"
        WORKING_DIRECTORY "${CWD}"
        RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err
        TIMEOUT 30)
    if(_rc)
        message(FATAL_ERROR "[${SCENARIO}] exit ${_rc}\nstdout:${_out}\nstderr:${_err}")
    endif()
    if(NOT _out MATCHES "ok")
        message(FATAL_ERROR "[${SCENARIO}] expected 'ok' in stdout, got: ${_out}\nstderr:${_err}")
    endif()
    message(STATUS "[${SCENARIO}] PASS")
endfunction()

# Neutral cwd — guarantees nothing resolves via cwd
set(NEUTRAL "${WORKDIR}/cwd")
file(MAKE_DIRECTORY "${NEUTRAL}")

# Scenario 1 — bundle (release) layout: exe at <bundle>/, modules at <bundle>/modules/dasSQLITE/
set(BUNDLE "${WORKDIR}/bundle")
file(MAKE_DIRECTORY "${BUNDLE}/modules/dasSQLITE")
file(COPY "${EXE}" DESTINATION "${BUNDLE}")
file(COPY "${SQLITE_MOD}" DESTINATION "${BUNDLE}/modules/dasSQLITE")
get_filename_component(_exe_name "${EXE}" NAME)
run_layout("bundle" "${BUNDLE}/${_exe_name}" "${NEUTRAL}")

# Scenario 2 — SDK install layout: exe at <root>/bin/, modules at <root>/modules/
# getDasRoot() strips trailing /bin to recover <root>, so <root>/modules wins.
set(INSTALL "${WORKDIR}/install")
file(MAKE_DIRECTORY "${INSTALL}/bin" "${INSTALL}/modules/dasSQLITE")
file(COPY "${EXE}" DESTINATION "${INSTALL}/bin")
file(COPY "${SQLITE_MOD}" DESTINATION "${INSTALL}/modules/dasSQLITE")
run_layout("sdk_install" "${INSTALL}/bin/${_exe_name}" "${NEUTRAL}")

# Scenario 3 — local dev build sanity: run the exe from its build location.
# (Compiled exe baked an absolute path to the SOURCE-tree dasSQLITE; that file
# still exists, so tier-3 fallback wins. Verifies we don't break today.)
run_layout("local_dev" "${EXE}" "${NEUTRAL}")

message(STATUS "All exe-path layouts resolved successfully.")
