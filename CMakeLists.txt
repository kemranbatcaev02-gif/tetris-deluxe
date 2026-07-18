cmake_minimum_required(VERSION 3.16)
project(TetrisDeluxe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3 (Graphics, Window, System)
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

file(GLOB SOURCES "src/*.cpp")
add_executable(tetris ${SOURCES})
target_link_libraries(tetris PRIVATE SFML::Graphics SFML::Window SFML::System)

# Кладём папку assets рядом с собранным .exe
add_custom_command(TARGET tetris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:tetris>/assets)
