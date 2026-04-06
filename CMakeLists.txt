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
    $<$<C_COMPILER_ID:MSVC>:/W4>
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

# -------------------------------------------------------
# Google Test via FetchContent — pinned for traceability
# -------------------------------------------------------
option(BUILD_TESTS "Build unit and integration tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK  OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)

    if(ENABLE_COVERAGE AND NOT MSVC)
        message(STATUS "Coverage instrumentation enabled (GCC/Clang)")
    endif()
endif()
