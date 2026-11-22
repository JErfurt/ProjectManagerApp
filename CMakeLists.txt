cmake_minimum_required(VERSION 3.20)
project(ProjectManagerApp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROJECTMANAGER_STATIC_BUILD "Statically link runtime and dependencies where possible" ON)

include(FetchContent)

# ------------------------------------------------------------------------------
# FLTK 1.4 (system or FetchContent)
# ------------------------------------------------------------------------------
find_package(FLTK 1.4 CONFIG QUIET)

if(NOT FLTK_FOUND)
    message(STATUS "FLTK 1.4 not found locally — fetching FLTK release-1.4.5 via FetchContent...")
    set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FLTK_OPTIONS OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_GL OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FORMS OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(FLTK_OPTION_SVG ON CACHE BOOL "" FORCE)
    set(FLTK_OPTION_PRINT_SUPPORT OFF CACHE BOOL "" FORCE)
    set(FLTK_USE_SYSTEM_LIBJPEG OFF CACHE BOOL "" FORCE)
    set(FLTK_USE_SYSTEM_LIBPNG OFF CACHE BOOL "" FORCE)
    set(FLTK_USE_SYSTEM_ZLIB OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        fltk
        GIT_REPOSITORY https://github.com/fltk/fltk.git
        GIT_TAG release-1.4.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(fltk)
endif()

# ------------------------------------------------------------------------------
# SQLite3 (static library on MinGW, system package, or amalgamation fallback)
# ------------------------------------------------------------------------------
set(SQLITE3_TARGET "")

if(MINGW AND PROJECTMANAGER_STATIC_BUILD)
    get_filename_component(_mingw_bin_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    get_filename_component(_mingw_root "${_mingw_bin_dir}" DIRECTORY)

    find_path(SQLITE3_STATIC_INCLUDE_DIR NAMES sqlite3.h HINTS "${_mingw_root}/include")
    set(_saved_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    find_library(SQLITE3_STATIC_LIB NAMES sqlite3 HINTS "${_mingw_root}/lib")
    find_library(ZLIB_STATIC_LIB NAMES z HINTS "${_mingw_root}/lib")
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${_saved_suffixes}")

    if(SQLITE3_STATIC_INCLUDE_DIR AND SQLITE3_STATIC_LIB)
        message(STATUS "Using static SQLite3: ${SQLITE3_STATIC_LIB}")
        add_library(sqlite3_static STATIC IMPORTED)
        set_target_properties(sqlite3_static PROPERTIES
            IMPORTED_LOCATION "${SQLITE3_STATIC_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_STATIC_INCLUDE_DIR}"
        )
        if(ZLIB_STATIC_LIB)
            set_target_properties(sqlite3_static PROPERTIES
                INTERFACE_LINK_LIBRARIES "${ZLIB_STATIC_LIB}"
            )
        endif()
        set(SQLITE3_TARGET sqlite3_static)
    endif()
endif()

if(NOT SQLITE3_TARGET)
    find_package(SQLite3 QUIET)
    if(SQLite3_FOUND)
        set(SQLITE3_TARGET SQLite::SQLite3)
    else()
        message(STATUS "SQLite3 not found — fetching SQLite amalgamation...")
        FetchContent_Declare(
            sqlite3_amalgamation
            URL https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(sqlite3_amalgamation)
        add_library(sqlite3_bundled STATIC "${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c")
        target_include_directories(sqlite3_bundled PUBLIC "${sqlite3_amalgamation_SOURCE_DIR}")
        target_compile_definitions(sqlite3_bundled PRIVATE
            SQLITE_THREADSAFE=1
            SQLITE_OMIT_LOAD_EXTENSION=1
        )
        set(SQLITE3_TARGET sqlite3_bundled)
    endif()
endif()

# ------------------------------------------------------------------------------
# Application sources
# ------------------------------------------------------------------------------
set(APP_SOURCES
    src/main.cpp
    src/Models/Project.hpp
    src/Services/PlatformService.hpp
    src/Services/PlatformService.cpp
    src/Services/CodeEditorService.hpp
    src/Services/CodeEditorService.cpp
    src/Services/ProjectService.hpp
    src/Services/ProjectService.cpp
    src/Services/GitStatusService.hpp
    src/Services/GitStatusService.cpp
    src/Services/SettingsService.hpp
    src/Services/SettingsService.cpp
    src/UI/Theme.hpp
    src/UI/Icons.hpp
    src/UI/Icons.cpp
    src/UI/SearchInput.hpp
    src/UI/ProjectTable.hpp
    src/UI/ProjectTable.cpp
    src/UI/MainWindow.hpp
    src/UI/MainWindow.cpp
)

if(WIN32)
    list(APPEND APP_SOURCES resources.rc)
    add_executable(ProjectManagerApp WIN32 ${APP_SOURCES})
else()
    add_executable(ProjectManagerApp ${APP_SOURCES})
endif()

target_include_directories(ProjectManagerApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(TARGET fltk::images)
    target_link_libraries(ProjectManagerApp PRIVATE fltk::images fltk::fltk)
elseif(TARGET fltk_images)
    target_link_libraries(ProjectManagerApp PRIVATE fltk_images fltk)
else()
    target_link_libraries(ProjectManagerApp PRIVATE ${FLTK_LIBRARIES})
endif()

target_link_libraries(ProjectManagerApp PRIVATE ${SQLITE3_TARGET})

# ------------------------------------------------------------------------------
# Platform-specific flags & size/memory optimizations
# ------------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(ProjectManagerApp PRIVATE
        dwmapi
        shell32
        ole32
        uuid
        advapi32
        comctl32
    )
    target_compile_definitions(ProjectManagerApp PRIVATE UNICODE _UNICODE)

    if(MSVC)
        target_compile_options(ProjectManagerApp PRIVATE /utf-8 /W4)
        target_link_options(ProjectManagerApp PRIVATE /MANIFEST:NO)
        if(PROJECTMANAGER_STATIC_BUILD)
            set_property(TARGET ProjectManagerApp PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    elseif(MINGW)
        target_compile_options(ProjectManagerApp PRIVATE
            -Wall -Wextra
            $<$<CONFIG:Release>:-Os -ffunction-sections -fdata-sections>
        )
        if(PROJECTMANAGER_STATIC_BUILD)
            target_link_options(ProjectManagerApp PRIVATE
                -static
                -static-libgcc
                -static-libstdc++
            )
        endif()
        target_link_options(ProjectManagerApp PRIVATE
            -pthread
            $<$<CONFIG:Release>:-Wl,--gc-sections -s>
        )
    endif()
else()
    target_compile_options(ProjectManagerApp PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Release>:-Os -ffunction-sections -fdata-sections>
    )
    if(NOT APPLE)
        target_link_options(ProjectManagerApp PRIVATE
            $<$<CONFIG:Release>:-Wl,--gc-sections -s>
        )
    endif()
endif()
