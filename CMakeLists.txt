cmake_minimum_required(VERSION 3.10)
project(PeakFinder)

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 20)

# Find GDAL package
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})
# Add your source files
add_executable(PeakFinder src/main.cpp)

# Link GDAL libraries
target_link_libraries(PeakFinder ${GDAL_LIBRARY})
