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

// Run ./PeakFinder ../data/Iceland_low_res.tif
int main(int argc, char *argv[])
{

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
  GDALAllRegister();
  unique_ptr<GDALDataset, decltype(gdalDeleter)> dataset(static_cast<GDALDataset *>(GDALOpen(demFilePath.c_str(), GA_ReadOnly)), gdalDeleter);
  if (dataset == nullptr)
  {
    cerr << "Failed to open file: " << demFilePath << endl;
    return 1;
  }

  // Calculate prominence
  calculateProminence(dataset, outputFilePath, prominenceThreshold, verbose);
  return EXIT_SUCCESS;
}
