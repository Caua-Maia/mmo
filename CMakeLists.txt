cmake_minimum_required(VERSION 3.16)
project(DungeonQuest C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(dungeon_quest
    main.c
    game_logic.c
    ui_render.c
)

target_include_directories(dungeon_quest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dungeon_quest PRIVATE raylib)

if(MSVC)
    target_compile_definitions(dungeon_quest PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(dungeon_quest PRIVATE /W3 /utf-8)
endif()

# Copy assets next to the binary when present
add_custom_command(TARGET dungeon_quest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:dungeon_quest>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dungeon_quest>/assets
)
