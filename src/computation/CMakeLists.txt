add_library(ComputationLib findPeaks.cpp calculateProminence.cpp printMetaData.cpp pixelToLatLong.cpp debug.cpp neighbors.cpp processKeyCol.cpp csv_util.cpp getIslandIfExists.cpp initializeMatrix.cpp)
target_include_directories(ComputationLib PUBLIC ${GDAL_INCLUDE_DIRS})
target_link_libraries(ComputationLib ${GDAL_LIBRARIES})
