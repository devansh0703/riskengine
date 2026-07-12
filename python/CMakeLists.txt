find_package(pybind11 QUIET)
if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.12.0
    )
    FetchContent_MakeAvailable(pybind11)
endif()

pybind11_add_module(risk_python pybind_module.cpp)
target_link_libraries(risk_python PRIVATE riskcore greeks)
target_include_directories(risk_python PRIVATE ${CMAKE_SOURCE_DIR}/include)
