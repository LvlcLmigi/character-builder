cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# vcpkg: use your own vcpkg clone, not the copy bundled with Visual Studio.
# Visual Studio overwrites VCPKG_ROOT inside its own environment, so this project
# reads CHARACTER_STUDIO_VCPKG_ROOT instead (see README, "One-time setup").
# ---------------------------------------------------------------------------
if(NOT DEFINED ENV{CHARACTER_STUDIO_VCPKG_ROOT} OR NOT EXISTS "$ENV{CHARACTER_STUDIO_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    message(FATAL_ERROR
        "vcpkg not found. Clone vcpkg (for example to J:\\vcpkg), run its bootstrap-vcpkg.bat, then run\n"
        "  setx CHARACTER_STUDIO_VCPKG_ROOT J:\\vcpkg\n"
        "and restart Visual Studio. Current value: '$ENV{CHARACTER_STUDIO_VCPKG_ROOT}'")
endif()
message(STATUS "Using vcpkg at: $ENV{CHARACTER_STUDIO_VCPKG_ROOT}")

project(CharacterStudio VERSION 0.1.0 LANGUAGES CXX)

# Libraries from vcpkg are built with the newest MSVC toolset on the machine. An
# older linker (VS 2022 before 17.10, MSVC 14.38 and below) cannot link them and fails
# with "unresolved external symbol __std_find_..." errors.
if(MSVC AND MSVC_VERSION LESS 1940)
    message(FATAL_ERROR
        "Your C++ compiler is too old (MSVC_VERSION ${MSVC_VERSION}, need 1940 or newer).\n"
        "Open the Visual Studio Installer and click Update on Visual Studio 2022 (17.10 or newer), "
        "then delete the build folder and run build.bat again.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# ---------------------------------------------------------------------------
# CommonLibSSE-NG (git submodule). One DLL works on SE 1.5.97 and every AE build
# through Address Library, so no rebuild is needed when Bethesda patches the game.
# ---------------------------------------------------------------------------
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG/CMakeLists.txt")
    message(FATAL_ERROR "CommonLibSSE-NG is missing. Run: git submodule update --init --recursive")
endif()

add_subdirectory(extern/CommonLibSSE-NG)
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.h)

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    NAME "CharacterStudio"
    AUTHOR "Luigi"
    USE_ADDRESS_LIBRARY
    SOURCES ${headers} ${sources}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SKSEMenuFramework
)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(${PROJECT_NAME} PRIVATE MOD_NAME="Character Studio")

# ---------------------------------------------------------------------------
# Deploy: copy the DLL straight into an MO2 mod folder after each build.
# Set the environment variable CHARACTER_STUDIO_MO2_MOD to something like
#   C:\Modding\MO2\mods\Character Studio
# and enable that mod in MO2 once.
# ---------------------------------------------------------------------------
if(DEFINED ENV{CHARACTER_STUDIO_MO2_MOD})
    set(DEPLOY_DIR "$ENV{CHARACTER_STUDIO_MO2_MOD}/SKSE/Plugins")
    message(STATUS "Character Studio will be copied to: ${DEPLOY_DIR}")
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DEPLOY_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DEPLOY_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DEPLOY_DIR}"
        VERBATIM
    )
endif()
