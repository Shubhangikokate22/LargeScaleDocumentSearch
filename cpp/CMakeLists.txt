cmake_minimum_required(VERSION 3.16)
project(document_search LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenMP)
find_package(nlohmann_json CONFIG QUIET)
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
  HINTS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
        "$ENV{MSYSTEM_PREFIX}/include"
        "C:/msys64/ucrt64/include"
        "C:/msys64/mingw64/include")
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
  message(FATAL_ERROR "nlohmann/json.hpp was not found. Install the nlohmann-json development package and reconfigure.")
endif()
add_executable(search src/main.cpp src/JSONLoader.cpp src/SearchEngine.cpp src/SequentialSearch.cpp src/OpenMPSearch.cpp src/Benchmark.cpp)
target_include_directories(search PRIVATE include ${NLOHMANN_JSON_INCLUDE_DIR})
if(TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(search PRIVATE nlohmann_json::nlohmann_json)
endif()
if(OpenMP_CXX_FOUND)
  target_link_libraries(search PRIVATE OpenMP::OpenMP_CXX)
endif()
