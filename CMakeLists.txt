cmake_minimum_required(VERSION 3.14)
project(breakout)

set(INCLUDES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/includes)

###########################################################
# 3RD PARTY LIBRARIES
###########################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(breakout)

target_compile_options(
    breakout
    PRIVATE
    # -fno-exceptions
    -fdiagnostics-color=always
    -Wall
    # -Wno-reorder
    # -Wno-unknown-pragmas
    # -Wno-error=unknown-pragmas
    # Wno-unknown-pragmas
)

target_sources(
    breakout
    PRIVATE

    src/ball_object_collisions.cpp
    src/game_level.cpp
    src/main.cpp
    src/post_processor.cpp
    src/shader.cpp
    src/text_renderer.cpp
    src/game.cpp
    src/game_object.cpp
    src/particle_generator.cpp
    src/resource_manager.cpp
    src/sprite_renderer.cpp
    src/texture.cpp

    includes/glad.c
    includes/stb_image.c
    includes/miniaudio_split.c
)

target_include_directories(
    breakout
    PRIVATE
    includes/
)

target_link_libraries(breakout ${CMAKE_SOURCE_DIR}/libs/mingw/libglfw3.a)
target_link_libraries(breakout ${CMAKE_SOURCE_DIR}/libs/mingw/libfreetype.a)
target_link_libraries(breakout gdi32)

target_compile_definitions(breakout PUBLIC FS_SRC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/src/")
