# prominence-finder
A C++ program to locate all peaks and calculate their prominence


# What is prominence?

From [Peakbagger](https://www.peakbagger.com/Help/Glossary.aspx#prom):
>Prominence is defined as the vertical distance a given summit rises above the lowest col on the highest ridge connecting it to a higher summit. Or, put another way, it is the elevation difference between the summit of a peak and the lowest contour that contains the given peak and no higher peaks. Imagine the ocean rising to the exact point where a certain peak is the highest point on its very own island. At that point, the prominence is the elevation of the peak above the risen ocean.

# Get started

## Dependencies
- [GDAL](https://gdal.org/download.html) to parse .tif data
- [VTK](https://vtk.org/download/) to render a 3d visualization of the dataset
- [CMake](https://cmake.org/cmake/help/latest/command/install.html) To build and compile the project 




The .tif raster data can be found [here](https://ftp.lmi.is/gisdata/raster/)
