# -------------------------------------------------------
# Resolve GTest target name — 1.10.0 uses plain gtest_main,
# later versions export the GTest:: namespace alias.
# -------------------------------------------------------
if(TARGET GTest::gtest_main)
    set(GTEST_MAIN_TARGET GTest::gtest_main)
else()
    set(GTEST_MAIN_TARGET gtest_main)
endif()

# -------------------------------------------------------
# Unit Tests
# -------------------------------------------------------
add_executable(test_unit
    unit/test_vitals.cpp
    unit/test_alerts.cpp
    unit/test_patient.cpp
    unit/test_session_export.cpp
    unit/test_auth.cpp
    unit/test_hal.cpp
    unit/test_config.cpp
    unit/test_localization.cpp
    unit/test_news2.cpp
    unit/test_alarm_limits.cpp
    unit/test_trend.cpp
    ${PROJECT_SOURCE_DIR}/src/gui_auth.c
    ${PROJECT_SOURCE_DIR}/src/gui_users.c
    ${PROJECT_SOURCE_DIR}/src/pw_hash.c
    ${PROJECT_SOURCE_DIR}/src/sim_vitals.c
)
target_link_libraries(test_unit PRIVATE monitor_lib ${GTEST_MAIN_TARGET})
target_include_directories(test_unit PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(test_unit PRIVATE SESSION_EXPORT_TESTING)
add_test(NAME UnitTests COMMAND test_unit --gtest_output=xml:${CMAKE_BINARY_DIR}/results_unit.xml)

# -------------------------------------------------------
# Integration Tests
# -------------------------------------------------------
add_executable(test_integration
    integration/test_patient_monitoring.cpp
    integration/test_alert_escalation.cpp
    integration/test_session_export.cpp
)
target_link_libraries(test_integration PRIVATE monitor_lib ${GTEST_MAIN_TARGET})
target_include_directories(test_integration PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(test_integration PRIVATE SESSION_EXPORT_TESTING)
add_test(NAME IntegrationTests COMMAND test_integration --gtest_output=xml:${CMAKE_BINARY_DIR}/results_integration.xml)

set(TEST_TEMP_DIR "${CMAKE_BINARY_DIR}/test_tmp")
file(MAKE_DIRECTORY "${TEST_TEMP_DIR}")
foreach(test_name UnitTests IntegrationTests)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "TMP=${TEST_TEMP_DIR};TEMP=${TEST_TEMP_DIR};TMPDIR=${TEST_TEMP_DIR}")
endforeach()

# -------------------------------------------------------
# GCC/Clang: propagate coverage flags to test binaries
# -------------------------------------------------------
if(ENABLE_COVERAGE AND NOT MSVC)
    foreach(target test_unit test_integration)
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
        target_link_options(${target} PRIVATE --coverage)
    endforeach()
endif()
