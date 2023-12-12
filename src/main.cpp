#include <iostream>
#include <memory>
#include <gdal.h>
#include <utility>
#include <vector>
#include <gdal_priv.h>

#include <string>

#include <ogr_spatialref.h>
#include "visualization/visualizeTif.hpp"
#include "computation/gdal_computation.hpp"

using namespace std;

/**
 * @mainpage Prominence Finder Documentation
 *
 * Welcome to the Prominence Finder project documentation.
 *
 * This application is designed to process digital elevation models (DEMs) to calculate the prominence of peaks. It uses GDAL for processing geospatial data and VTK to visualize the datasets.
 *
 * The main functionalities include:
 * - Optionally visualizing the dataset with VTK.
 * - Reading and processing .tif files representing DEMs.
 * - Calculating the prominence of features in the DEM.

 *
 * For more details on how to use this application, refer to the respective functions and classes.
 */

int main(int argc, char *argv[])
{
  GDALAllRegister();

  if (argc <= 1)
  {
    cerr << "Usage: " << argv[0] << " <FileName.tiff> [-o output.csv] [-v]" << endl;
    return EXIT_FAILURE;
  }

  string demFilePath = argv[1];
  string outputFilePath;
  int prominenceThreshold = 0;
  bool visualize = false;
  bool verbose = false;

  for (int i = 2; i < argc; i++)
  {
    string arg = argv[i];
    if (arg == "-visualize")
    {
      visualize = true;
    }
    else if (arg == "-verbose")
    {
      verbose = true;
    }
    else if (arg == "-o" && i + 1 < argc)
    {
      outputFilePath = argv[++i]; // Increment i to skip the next argument as it is the file path for -o
    }
    else if (arg == "-threshold" && i + 1 < argc)
    {
      i++;
      prominenceThreshold = stoi(argv[i]);
    }
    else
    {
      cerr << "Unknown option: " << arg << endl;
      return EXIT_FAILURE;
    }
  }

  if (visualize)
  {
    visualizeTif(demFilePath);
    return 0;
  }

  unique_ptr<GDALDataset> dataset(static_cast<GDALDataset *>(GDALOpen(demFilePath.c_str(), GA_ReadOnly)));

  if (dataset == nullptr)
  {
    cerr << "Failed to open file: " << demFilePath << endl;
    return 1;
  }

  // Calculate prominence
  calculateProminence(dataset, outputFilePath, prominenceThreshold, verbose);

  return EXIT_SUCCESS;
}
