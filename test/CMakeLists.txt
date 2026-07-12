find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

set(TEST_SOURCES
    test_position_tracker.cpp
    test_pnl_calculator.cpp
    test_lot_matching.cpp
    test_metrics_engine.cpp
    test_risk_limits.cpp
    test_alert_manager.cpp
    test_trade_journal.cpp
    test_market_data.cpp
    test_stress.cpp
)

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE riskcore greeks GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Greeks standalone test
add_executable(test_greeks ${CMAKE_SOURCE_DIR}/greeks/test_greeks.cpp)
target_link_libraries(test_greeks PRIVATE greeks)
add_test(NAME test_greeks_standalone COMMAND test_greeks)
