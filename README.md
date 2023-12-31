# prominence-finder
A C++ program to locate all peaks and calculate their prominence
# Docs
Full docs can be accessed [here](https://arnifreyrm.github.io/prominence-finder/)

# What is prominence?

From [Peakbagger](https://www.peakbagger.com/Help/Glossary.aspx#prom):
>Prominence is defined as the vertical distance a given summit rises above the lowest col on the highest ridge connecting it to a higher summit. Or, put another way, it is the elevation difference between the summit of a peak and the lowest contour that contains the given peak and no higher peaks. Imagine the ocean rising to the exact point where a certain peak is the highest point on its very own island. At that point, the prominence is the elevation of the peak above the risen ocean.

# Get started

## Dependencies
- [GDAL](https://gdal.org/download.html) to parse .tif data
- [VTK](https://vtk.org/download/) to render a 3d visualization of the dataset
- [CMake](https://cmake.org/cmake/help/latest/command/install.html) To build and compile the project

## Building and running

Clone the project: run ```git@github.com:arnifreyrm/prominence-finder.git```

navigate into the project, then run:

```mkdir build```

```cd build```

```cmake ..```

```make```

To run it, you need a DEM (Digital Elevation Map) in a .tif format. There is one included toy dataset included so to try it out you can run: 

```./PeakFinder ../data/two_pyramids.tif -visualize```

to visualize the dataset.

To run the prominence calculations run:

```./PeakFinder ../data/two_pyramids.tif -o ../results/two_pyramids.csv```

And you will have the results in ```/results/two_pyramids.csv```

# Syntax

```./Peakfinder <input file>```

## Flags
-  `-o` Output file. Needs to be followed by a path to a csv file.
-  `-visualize` Runs visualization instead of calculation
-  `-threshold` Sets a prominence threshold for outputted peaks. Needs to be followed by an integer value.
