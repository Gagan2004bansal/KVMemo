cmake_minimum_required(VERSION 3.16)

project(KVMemo VERSION 1.0 LANGUAGES CXX)

# ---------------------------------------------------------
# C++ Standard
# ---------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

include_directories(src)

# ---------------------------------------------------------
# Source files
# ---------------------------------------------------------

file(GLOB_RECURSE KVMEMO_SOURCES
    src/*.cpp
)

list(REMOVE_ITEM KVMEMO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/kv_cli.cpp
)

# ---------------------------------------------------------
# Server executable
# ---------------------------------------------------------

add_executable(kvmemo
    ${KVMEMO_SOURCES}
)

# ---------------------------------------------------------
# CLI executable
# ---------------------------------------------------------

add_executable(kv_cli
    src/client/kv_cli.cpp
)

target_include_directories(kv_cli PRIVATE src)