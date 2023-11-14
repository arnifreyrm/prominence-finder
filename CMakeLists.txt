cmake_minimum_required(VERSION 3.10)
project(PeakFinder)

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 20)

# Find GDAL package
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})

# Find VTK package
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
# Add the subdirectory
add_subdirectory(src/visualization)
add_subdirectory(src/computation)
# Add your source files
add_executable(PeakFinder src/main.cpp)

# Link GDAL and VTK libraries
target_link_libraries(PeakFinder ${GDAL_LIBRARIES} ${VTK_LIBRARIES})
target_link_libraries(PeakFinder VisualizationLib)
target_link_libraries(PeakFinder ComputationLib)
# Include VTK directories for your target
target_include_directories(PeakFinder PRIVATE ${VTK_INCLUDE_DIRS})
