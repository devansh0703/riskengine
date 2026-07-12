cmake_minimum_required(VERSION 3.20)
project(riskengine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_PYTHON_BINDINGS "Build pybind11 bindings" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_TOOLS "Build CLI tools" ON)

find_package(Threads REQUIRED)

# SQLite
find_library(SQLITE3_LIB sqlite3)
if(NOT SQLITE3_LIB)
    find_path(SQLITE3_INCLUDE sqlite3.h)
endif()

set(SOURCES
    src/position_tracker.cpp
    src/pnl_calculator.cpp
    src/lot_matching.cpp
    src/metrics_engine.cpp
    src/risk_limits.cpp
    src/alert_manager.cpp
    src/trade_journal.cpp
    src/market_data.cpp
)

add_library(riskcore STATIC ${SOURCES})
target_include_directories(riskcore PUBLIC include greeks)
target_link_libraries(riskcore PUBLIC Threads::Threads)

if(SQLITE3_LIB)
    target_link_libraries(riskcore PUBLIC ${SQLITE3_LIB})
endif()

add_executable(riskengine src/main.cpp)
target_link_libraries(riskengine PRIVATE riskcore)

# Greeks library
add_library(greeks STATIC greeks/black_scholes.cpp)
target_include_directories(greeks PUBLIC greeks)

if(BUILD_TOOLS)
    add_executable(risk_cli tools/risk_cli.cpp)
    target_link_libraries(risk_cli PRIVATE riskcore)

    add_executable(replay_tool tools/replay_tool.cpp)
    target_link_libraries(replay_tool PRIVATE riskcore)

    add_executable(export_tool tools/export_tool.cpp)
    target_link_libraries(export_tool PRIVATE riskcore)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()
