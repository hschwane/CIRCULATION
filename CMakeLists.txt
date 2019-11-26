# -------------------------------------------------------------
# general setup
# -------------------------------------------------------------
cmake_minimum_required(VERSION 3.8...3.12)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# -------------------------------------------------------------
# dependencies
# -------------------------------------------------------------
enable_language(C)
enable_language(CXX)
enable_language(CUDA)
find_package(mpUtils REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP)

# -------------------------------------------------------------
# set up project
# -------------------------------------------------------------

# Appends the cmake/ path to MAKE_MODULE_PATH variable.
set(CMAKE_MODULE_PATH ${mpUtils_CMAKE_SCRIPTS_PATH} ${CMAKE_MODULE_PATH})

include(GetVersionFromGit)
project(CIRCULATION VERSION "${VERSION_SHORT}" LANGUAGES C CXX CUDA)

# default build configuration
include(setDefaultTypeRelease)

# -------------------------------------------------------------
# create target
# -------------------------------------------------------------
add_executable(CIRCULATION
            "src/dummy.cpp"
            "src/main.cu"
            "src/Application.cu"
            "src/Grid.cu"
        )

# -------------------------------------------------------------
# set target properties
# -------------------------------------------------------------

# set required language standard
set_target_properties(CIRCULATION PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CUDA_STANDARD 14
        CUDA_STANDARD_REQUIRED YES
        )

target_compile_definitions(CIRCULATION PRIVATE PROJECT_SHADER_PATH="${CMAKE_CURRENT_LIST_DIR}/shader/")
target_compile_definitions(CIRCULATION PRIVATE PROJECT_RESOURCE_PATH="${CMAKE_CURRENT_LIST_DIR}/shader/")
target_compile_definitions(CIRCULATION PRIVATE "CIRCULATION_VERSION=\"${VERSION_SHORT}\"")
target_compile_definitions(CIRCULATION PRIVATE "CIRCULATION_VERSION_SHA=\"${VERSION_SHA1}\"")

set_target_properties( CIRCULATION PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(CIRCULATION PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--default-stream per-thread>)
target_compile_options(CIRCULATION PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(CIRCULATION PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-G -g>)
else()
    target_compile_options(CIRCULATION PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-lineinfo>)
endif()


# set -Wa,-I for resources search path
# target_compile_options(CIRCULATION PRIVATE -Wa,-I${CMAKE_SOURCE_DIR})

# -------------------------------------------------------------
# link dependencies (this will also link the dependencies of dependencies and set required compiler flags)
# -------------------------------------------------------------
if(UNIX)
    target_link_libraries(CIRCULATION PUBLIC stdc++fs)
endif()

target_link_libraries(CIRCULATION PUBLIC Threads::Threads mpUtils::mpUtils)

if(OpenMP_FOUND)
    target_link_libraries(CIRCULATION PRIVATE OpenMP::OpenMP_CXX)
endif()