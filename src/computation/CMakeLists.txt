add_library(ComputationLib findPeaks.cpp printMetaData.cpp pixelToLatLong.cpp)
target_include_directories(ComputationLib PUBLIC ${GDAL_INCLUDE_DIRS})
target_link_libraries(ComputationLib ${GDAL_LIBRARIES})
