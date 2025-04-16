cmake_minimum_required(VERSION 3.29)
project(VerletSimulator LANGUAGES CXX)

option(USE_ORB "build orb.cpp instead of main.cpp" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "build shared libraries" OFF)

find_package(SFML 3.0.0 COMPONENTS Graphics Window System REQUIRED)

if(USE_ORB)
    set(MAIN_SOURCE src/orb.cpp)
    message(STATUS "building orb.cpp")
else()
    set(MAIN_SOURCE src/main.cpp)
    message(STATUS "building main.cpp")
endif()

add_executable(VerletSimulator ${MAIN_SOURCE})
target_link_libraries(VerletSimulator PRIVATE sfml-graphics sfml-window sfml-system)
target_compile_features(VerletSimulator PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMENT "copy assets directory"
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    VERBATIM)

install(TARGETS VerletSimulator)
