# -------------------------------------------------------
# Unit Tests
# -------------------------------------------------------
add_executable(test_unit
    unit/test_vitals.cpp
    unit/test_alerts.cpp
    unit/test_patient.cpp
)
target_link_libraries(test_unit PRIVATE monitor_lib GTest::gtest_main)
target_include_directories(test_unit PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME UnitTests COMMAND test_unit --gtest_output=xml:${CMAKE_BINARY_DIR}/results_unit.xml)

# -------------------------------------------------------
# Integration Tests
# -------------------------------------------------------
add_executable(test_integration
    integration/test_patient_monitoring.cpp
    integration/test_alert_escalation.cpp
)
target_link_libraries(test_integration PRIVATE monitor_lib GTest::gtest_main)
target_include_directories(test_integration PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME IntegrationTests COMMAND test_integration --gtest_output=xml:${CMAKE_BINARY_DIR}/results_integration.xml)

# -------------------------------------------------------
# GCC/Clang: propagate coverage flags to test binaries
# -------------------------------------------------------
if(ENABLE_COVERAGE AND NOT MSVC)
    foreach(target test_unit test_integration)
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
        target_link_options(${target} PRIVATE --coverage)
    endforeach()
endif()
