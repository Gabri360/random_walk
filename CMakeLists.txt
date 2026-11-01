cmake_minimum_required(VERSION 3.24)
project(random_walk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.32.0
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS 2.0 CONFIG
)
FetchContent_Declare(SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG        release-2.24.0
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(SDL2 SDL2_ttf)

add_executable(random_walk
  src/main.c
  lib/render.c
  src/simulation.c
)

target_include_directories(random_walk PRIVATE ${CMAKE_SOURCE_DIR}/headers)

if(TARGET SDL2_ttf::SDL2_ttf)
    set(RW_TTF_TARGET SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2_ttf::SDL2_ttf-static)
    set(RW_TTF_TARGET SDL2_ttf::SDL2_ttf-static)
else()
    message(FATAL_ERROR "Target SDL2_ttf non trovato")
endif()

if(TARGET SDL2::SDL2)
    set(RW_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(RW_SDL_TARGET SDL2::SDL2-static)
else()
    message(FATAL_ERROR "Target SDL2 non trovato")
endif()

target_link_libraries(random_walk PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    ${RW_SDL_TARGET}
    ${RW_TTF_TARGET}
)

if(NOT WIN32)
    target_link_libraries(random_walk PRIVATE m)
endif()

if(MSVC)
    target_compile_options(random_walk PRIVATE /W4)
else()
    target_compile_options(random_walk PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_custom_command(TARGET random_walk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:random_walk>/assets
)
