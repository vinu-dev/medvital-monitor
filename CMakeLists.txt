cmake_minimum_required(VERSION 3.15)
project(PatientMonitor VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# Coverage option (GCC/Clang — MSVC uses OpenCppCoverage)
# -------------------------------------------------------
option(ENABLE_COVERAGE "Enable GCC/Clang coverage instrumentation" OFF)

# -------------------------------------------------------
# Production library — shared by app and all test targets
# -------------------------------------------------------
add_library(monitor_lib STATIC
    src/vitals.c
    src/alerts.c
    src/patient.c
)
target_include_directories(monitor_lib PUBLIC include)
target_compile_options(monitor_lib PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:MSVC>:/W4 /FS>
)

if(ENABLE_COVERAGE AND NOT MSVC)
    target_compile_options(monitor_lib PRIVATE --coverage -O0 -g)
    target_link_options(monitor_lib PUBLIC --coverage)
endif()

# -------------------------------------------------------
# Main executable
# -------------------------------------------------------
add_executable(patient_monitor src/main.c)
target_link_libraries(patient_monitor PRIVATE monitor_lib)

if(WIN32)
    add_executable(patient_monitor_gui WIN32
        src/gui_main.c
        src/gui_auth.c
        src/sim_vitals.c
    )
    target_include_directories(patient_monitor_gui PRIVATE include)
    target_link_libraries(patient_monitor_gui PRIVATE monitor_lib)
    target_compile_options(patient_monitor_gui PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4 /FS>
    )
endif()

# -------------------------------------------------------
# Google Test via FetchContent — pinned for traceability
# -------------------------------------------------------
option(BUILD_TESTS "Build unit and integration tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.10.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK  OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # MinGW compatibility: GTest release-1.10.0 uses MSVC-specific headers and
    # symbols (_stricmp, _fileno, crtdbg.h, etc.) inside GTEST_OS_WINDOWS blocks.
    # Generate stub/shim headers and force-include a compat header for each
    # GTest translation unit so the old MinGW 6.3 toolchain can compile GTest.
    if(MINGW)
        set(MINGW_STUB_DIR "${CMAKE_BINARY_DIR}/mingw_compat")

        # Stub headers that satisfy #include directives inside gtest sources
        file(WRITE "${MINGW_STUB_DIR}/crtdbg.h"
             "/* MinGW stub: satisfies GTest #include <crtdbg.h> */\n"
             "#ifndef _INC_CRTDBG\n#define _INC_CRTDBG\n"
             "#ifndef MAX_PATH\n# define MAX_PATH 260\n#endif\n"
             "#ifndef _MAX_PATH\n# define _MAX_PATH MAX_PATH\n#endif\n"
             "#endif\n")
        file(WRITE "${MINGW_STUB_DIR}/debugapi.h"
             "/* MinGW stub: satisfies GTest #include <debugapi.h> */\n"
             "#ifndef _APISET_DEBUGAPI_H_\n#define _APISET_DEBUGAPI_H_\n#endif\n")

        # Force-included shim: provides all symbols GTest needs that old MinGW
        # 6.3.0 (32-bit) does not expose in C++17 mode.
        file(WRITE "${MINGW_STUB_DIR}/mingw32_gtest_compat.h" [=[
/* MinGW GCC 6.3.0 compatibility shim for GTest release-1.10.0.
 * Force-included via -include compiler flag for gtest/gtest_main targets. */
#ifndef MINGW32_GTEST_COMPAT_H_
#define MINGW32_GTEST_COMPAT_H_

/* Pull in windows.h + CRT headers first so macros like MAX_PATH are defined */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <strings.h>   /* strcasecmp */

/* _MAX_PATH: MinGW stdlib.h maps it to MAX_PATH; ensure it's defined */
#ifndef _MAX_PATH
#  ifndef MAX_PATH
#    define MAX_PATH 260
#  endif
#  define _MAX_PATH MAX_PATH
#endif

/* String comparison — MinGW32 may not expose the underscore versions in C++ */
#ifndef _stricmp
#  define _stricmp strcasecmp
#endif
#ifndef _strncasecmp
#  define _strncasecmp strncasecmp
#endif

/* _strdup: POSIX strdup may be hidden in MinGW C++17 mode; implement directly */
#ifndef _strdup
#  ifdef __cplusplus
     static inline char* _strdup(const char* s) {
         size_t n = ::strlen(s) + 1;
         char* p = static_cast<char*>(::malloc(n));
         if (p) ::memcpy(p, s, n);
         return p;
     }
#  else
#    define _strdup(s) ((char*)memcpy(malloc(strlen(s)+1),(s),strlen(s)+1))
#  endif
#endif

/* _fileno: forward-declare so it is always visible in C++ mode */
#ifndef _fileno
#  ifdef __cplusplus
     extern "C" int fileno(FILE*);
#  endif
#  define _fileno fileno
#endif

/* fdopen: POSIX; MinGW has it in <stdio.h> under C mode but it may be hidden
 * in C++ mode.  Forward-declare it to ensure it is visible. */
#ifdef __cplusplus
extern "C" FILE* fdopen(int, const char*);
#endif

/* _wcsicmp: wide-char case-insensitive compare.
 * Old MinGW may not have wcscasecmp; provide a simple fallback. */
#ifndef _wcsicmp
#  ifdef wcscasecmp
#    define _wcsicmp wcscasecmp
#  else
#    include <wctype.h>
     static inline int _wcsicmp(const wchar_t* a, const wchar_t* b) {
         while (*a && *b && towlower(*a) == towlower(*b)) { ++a; ++b; }
         return (int)towlower(*a) - (int)towlower(*b);
     }
#  endif
#endif

/* _exit: POSIX; available in <process.h>; forward-declare as safety net */
#ifndef _exit
#  ifdef __cplusplus
     extern "C" void _exit(int);
#  else
     extern void _exit(int);
#  endif
#endif

/* MSVC-only CRT debug API — stub out; never called in normal test runs */
#define _OUT_TO_STDERR          2
#define _set_error_mode(x)      ((void)(x))
#define _CRT_ASSERT             2
#define _CRTDBG_MODE_FILE       1
#define _CRTDBG_MODE_DEBUG      4
#define _CrtSetReportMode(t,m)  0
#define _CRTDBG_FILE_STDERR     ((void*)2)
#define _CrtSetReportFile(t,f)  NULL

#endif /* MINGW32_GTEST_COMPAT_H_ */
]=])

        target_include_directories(gtest      BEFORE PUBLIC "${MINGW_STUB_DIR}")
        target_include_directories(gtest_main BEFORE PUBLIC "${MINGW_STUB_DIR}")
        target_compile_options(gtest
            PRIVATE "-include${MINGW_STUB_DIR}/mingw32_gtest_compat.h")
        target_compile_options(gtest_main
            PRIVATE "-include${MINGW_STUB_DIR}/mingw32_gtest_compat.h")
    endif()

    enable_testing()
    add_subdirectory(tests)

    if(ENABLE_COVERAGE AND NOT MSVC)
        message(STATUS "Coverage instrumentation enabled (GCC/Clang)")
    endif()
endif()
