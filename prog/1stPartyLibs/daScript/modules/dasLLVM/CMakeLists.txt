IF ((NOT DAS_LLVM_INCLUDED) AND ((NOT ${DAS_LLVM_DISABLED}) OR (NOT DEFINED DAS_LLVM_DISABLED)))
    SET(DAS_LLVM_INCLUDED TRUE)
    MESSAGE(STATUS "dasLlvm module included.")

	SET(DAS_LLVM_DIR ${PROJECT_SOURCE_DIR}/modules/dasLLVM)

    LIST(APPEND CMAKE_MODULE_PATH ${DAS_LLVM_DIR})

    set(DAS_LLVM_LIB_OUTPUT "${PROJECT_SOURCE_DIR}/lib")
    set(DAS_CLANG_OUTPUT "${PROJECT_SOURCE_DIR}/bin")
    set(DAS_LLVM_DLL_NAME LLVM.dll)

    # Cache-invalidating sentinel: ${DAS_LLVM_LIB_OUTPUT}/LLVM.dll.version
    # holds the LLVM target version of the prebuilt currently on disk. Mismatch
    # (or missing file) forces re-discovery / re-download. Without this, bumping
    # the LLVM version in this file does NOT replace a stale lib/LLVM.dll from
    # a prior build.
    SET(DAS_LLVM_TARGET_VERSION "22.1.5")
    # Sentinel value carries a -r<N> revision on top of the LLVM version: the
    # prebuilt can change (different cmake flags) without an LLVM version bump.
    SET(DAS_LLVM_SENTINEL_VERSION "${DAS_LLVM_TARGET_VERSION}-r4")
    SET(DAS_LLVM_REFETCH FALSE)
    IF (NOT EXISTS "${DAS_LLVM_LIB_OUTPUT}/${DAS_LLVM_DLL_NAME}")
        SET(DAS_LLVM_REFETCH TRUE)
    ELSEIF (NOT EXISTS "${DAS_LLVM_LIB_OUTPUT}/LLVM.dll.version")
        SET(DAS_LLVM_REFETCH TRUE)
    ELSE()
        file(READ "${DAS_LLVM_LIB_OUTPUT}/LLVM.dll.version" _das_llvm_cached_ver)
        STRING(STRIP "${_das_llvm_cached_ver}" _das_llvm_cached_ver)
        IF (NOT _das_llvm_cached_ver STREQUAL DAS_LLVM_SENTINEL_VERSION)
            MESSAGE(STATUS "dasLlvm: cached LLVM ${_das_llvm_cached_ver} != target ${DAS_LLVM_SENTINEL_VERSION}, refetching")
            SET(DAS_LLVM_REFETCH TRUE)
        ENDIF()
    ENDIF()

    IF (DAS_LLVM_REFETCH)
        find_package(LLVM ${DAS_LLVM_TARGET_VERSION})
        # A system LLVM is only usable here if it ships the prebuilt-release
        # layout we copy from. On Windows that means bin/lld-link.exe +
        # bin/LLVM-C.dll. msys2/mingw clang64 provides LLVMConfig.cmake but lays
        # the binaries out differently (libLLVM-NN.dll, no LLVM-C.dll); a
        # 2026-06 msys2 package update started reporting a matching version so
        # find_package now succeeds where it used to fail the version match.
        # Treat that as not-found and fall through to the prebuilt download.
        IF (LLVM_FOUND AND WIN32 AND
            ((NOT EXISTS "${LLVM_INSTALL_PREFIX}/bin/lld-link.exe") OR
             (NOT EXISTS "${LLVM_INSTALL_PREFIX}/bin/LLVM-C.dll")))
            MESSAGE(STATUS "dasLlvm: system LLVM at ${LLVM_INSTALL_PREFIX} lacks "
                "Windows-release lld-link.exe/LLVM-C.dll (msys2/mingw layout); "
                "using prebuilt download")
            SET(LLVM_FOUND FALSE)
        ENDIF()
        IF (LLVM_FOUND)
            MESSAGE(STATUS "LLVM found at: ${LLVM_INSTALL_PREFIX}")
            IF(WIN32)
                # Copy lld-link — module_jit.cpp's find_linker drives the JIT
                # -exe/-dll link with it directly (no compiler driver in front)
                FILE(COPY
                    "${LLVM_INSTALL_PREFIX}/bin/lld-link.exe" DESTINATION
                    "${DAS_CLANG_OUTPUT}"
                )
                set(DAS_LLVM_DLL_PATH ${LLVM_INSTALL_PREFIX}/bin/LLVM-C.dll)
            ELSEIF(APPLE)
                set(DAS_LLVM_DLL_PATH ${LLVM_INSTALL_PREFIX}/lib/libLLVM.dylib)
            ELSE()
                set(DAS_LLVM_DLL_PATH ${LLVM_INSTALL_PREFIX}/lib/libLLVM.so )
            ENDIF()
            get_filename_component(dll_name "${dll}" NAME)
            configure_file("${DAS_LLVM_DLL_PATH}"
                "${DAS_LLVM_LIB_OUTPUT}/${DAS_LLVM_DLL_NAME}"
                COPYONLY
            )
            file(WRITE "${DAS_LLVM_LIB_OUTPUT}/LLVM.dll.version" "${DAS_LLVM_SENTINEL_VERSION}")
        ELSE()
            # File not exists locally and LLVM lib not found.
            SET(DAS_LLVM_HAS_PREBUILT TRUE)
            IF(WIN32)
                SET(DAS_LLVM_FILE_PLATFORM win64)
                SET(DAS_LLVM_FILE_HASH "7f67cbfa1b8196d13b020f8fea721c4c586204b3d0f9f0c73b289514a888c899")
            ELSEIF(APPLE)
                MESSAGE(STATUS ${CMAKE_SYSTEM_PROCESSOR})
                IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|aarch64")
                    SET(DAS_LLVM_FILE_PLATFORM mac_arm64)
                    SET(DAS_LLVM_FILE_HASH "affbdb6a19f8ff0efac24bd572a655e5e656483be928e2479ba75ddebe147624")
                ELSE()
                    SET(DAS_LLVM_HAS_PREBUILT FALSE)
                ENDIF()
            ELSEIF(UNIX)
                MESSAGE(STATUS ${CMAKE_SYSTEM_PROCESSOR})
                IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm|aarch64")
                    SET(DAS_LLVM_FILE_PLATFORM linux_arm64)
                    SET(DAS_LLVM_FILE_HASH "7d1580db4e6f0927b7c79bddc5e403cb4d1575767e00f4d5a23f77fa38be49cf")
                ELSE()
                    SET(DAS_LLVM_FILE_PLATFORM linux_64)
                    SET(DAS_LLVM_FILE_HASH "66cb1e970004fdae57513eecb16a311932e74bcbaa0f05bcc8b8acf457b82d6e")
                ENDIF()
            ENDIF()
            IF (NOT DAS_LLVM_HAS_PREBUILT)
                MESSAGE(FATAL_ERROR
                    "dasLlvm: no prebuilt LLVM.dll for this platform "
                    "(${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}) at "
                    "the llvm-v${DAS_LLVM_TARGET_VERSION} release. "
                    "Install LLVM ${DAS_LLVM_TARGET_VERSION} system-wide "
                    "(e.g. `brew install llvm` on macOS Intel) and re-run "
                    "cmake; find_package will pick it up. To disable the "
                    "module entirely set -DDAS_LLVM_DISABLED=ON.")
            ENDIF()
            SET(LLVM_LINK https://github.com/GaijinEntertainment/daScript/releases/download/llvm-v${DAS_LLVM_TARGET_VERSION}/${DAS_LLVM_FILE_PLATFORM}_llvm.tar.gz)
            MESSAGE(STATUS "Download LLVM at: ${LLVM_LINK}")
            include(FetchContent)

            FetchContent_Declare(
                das_llvm_shared_lib
                URL_HASH               SHA256=${DAS_LLVM_FILE_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP 1
                INACTIVITY_TIMEOUT 60
                DOWNLOAD_RETRIES 5
                URL ${LLVM_LINK}
            )
            FetchContent_MakeAvailable(das_llvm_shared_lib)

            file(COPY ${das_llvm_shared_lib_SOURCE_DIR}/LLVM.dll DESTINATION ${CMAKE_SOURCE_DIR}/lib)
            IF(WIN32)
                # The package also ships clang-cl.exe (utils/preflight probes it
                # in the FetchContent source dir as a bundled-compiler fallback);
                # only the linker goes to bin/.
                file(COPY ${das_llvm_shared_lib_SOURCE_DIR}/lld-link.exe DESTINATION ${CMAKE_SOURCE_DIR}/bin)
            ENDIF()
            file(WRITE "${DAS_LLVM_LIB_OUTPUT}/LLVM.dll.version" "${DAS_LLVM_SENTINEL_VERSION}")
        ENDIF()
    ELSE()
        MESSAGE(STATUS "LLVM lib exists at: ${DAS_LLVM_LIB_OUTPUT}/${DAS_LLVM_DLL_NAME}")
    ENDIF()

    IF(WIN32)
        install(FILES "${DAS_CLANG_OUTPUT}/lld-link.exe" DESTINATION ${DAS_INSTALL_BINDIR})
    ENDIF()
    # Always put to single place, path and name should match with llvm_func.das definitions
    install(FILES "${DAS_LLVM_LIB_OUTPUT}/${DAS_LLVM_DLL_NAME}" DESTINATION ${DAS_INSTALL_LIBDIR})
	install(FILES ${PROJECT_SOURCE_DIR}/modules/dasLLVM/LICENSE.TXT DESTINATION ${DAS_INSTALL_DOCDIR} RENAME LLVM.LICENSE)

	file(GLOB DASLLVM_SOURCES
			${DAS_LLVM_DIR}/daslib/*.das
	)
	install(FILES ${DASLLVM_SOURCES}
			DESTINATION ${DAS_INSTALL_MODULESDIR}/dasLLVM/daslib
	)
	file(GLOB DASLLVM_BINDINGS ${DAS_LLVM_DIR}/bindings/*.das)
	install(FILES ${DASLLVM_BINDINGS} DESTINATION ${DAS_INSTALL_MODULESDIR}/dasLLVM/bindings)
	install(FILES ${DAS_LLVM_DIR}/.das_module DESTINATION ${DAS_INSTALL_MODULESDIR}/dasLLVM/)

	ADD_EXAMPLE_RUN(modules/dasLLVM/examples/hello_jit.das)
	ADD_EXAMPLE_RUN(modules/dasLLVM/examples/hello_llvm.das)
	ADD_EXAMPLE_RUN(modules/dasLLVM/examples/hello_llvm_jit.das)
	ADD_EXAMPLE_RUN(modules/dasLLVM/examples/hello_llvm_jit_function.das FALSE)

ENDIF()
