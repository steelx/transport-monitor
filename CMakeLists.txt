cmake_minimum_required(VERSION 3.15)
project(TransportProject LANGUAGES CXX)

include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch content dependencies here ---
message(STATUS "Fetching dotenv-cpp...")
FetchContent_Declare(
        dotenv-cpp
        GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
        GIT_TAG ee688ac80b1773c7ecc8a7e7cc545ac58d0ec20a #git hash
)
FetchContent_MakeAvailable(dotenv-cpp)

# Boost repository is large.
message(STATUS "Fetching Boost (this may take a moment)...")

# CRITICAL OPTIMIZATION:
# We tell Boost to ONLY build the libraries we need.
# This prevents compiling python, log, graph, etc., which saves huge amounts of time.
set(BOOST_INCLUDE_LIBRARIES system asio beast)
set(BOOST_ENABLE_CMAKE ON)

# We use the release tarball instead of GIT_REPOSITORY.
# The Git repo is >500MB history; the tarball is ~100MB.
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(Boost)
# --- Fetch Content END ---

# Copy .env file to the build directory so the executable can find it
configure_file(${CMAKE_SOURCE_DIR}/.env ${CMAKE_BINARY_DIR}/.env COPYONLY)

add_executable(TransportProject src/main.cpp)


# --- Link requires dependencies here ---
target_link_libraries(TransportProject PRIVATE
    dotenv
    Boost::system
    Boost::asio
    Boost::beast
)
