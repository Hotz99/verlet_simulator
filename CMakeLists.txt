cmake_minimum_required(VERSION 3.29)
project(VerletSimulator LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "build shared libraries" OFF)

if(APPLE AND NOT DEFINED SFML_DIR)
    foreach(brew_path "/usr/local" "/opt/homebrew")
        if(EXISTS "${brew_path}/lib/cmake/SFML")
            set(SFML_DIR "${brew_path}/lib/cmake/SFML")
            message(STATUS "found SFML managed by Homebrew at ${SFML_DIR}")
            break()
        endif()
    endforeach()
endif()

find_package(SFML 3.0.0 COMPONENTS Graphics Window System REQUIRED)

add_executable(VerletSimulator src/main.cpp)
target_link_libraries(VerletSimulator PRIVATE sfml-graphics sfml-window sfml-system)
target_compile_features(VerletSimulator PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMENT "copy assets directory"
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    VERBATIM)

install(TARGETS VerletSimulator)
