#include <iostream>
#include <memory>
#include <gdal.h>
#include <vector>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include "visualization/visualizeTif.hpp"
#include "computation/gdal_computation.hpp"
using namespace std;

auto gdalDeleter = [](GDALDataset *dataset)
{
  GDALClose(dataset);
};

// Run ./PeakFinder ../data/Iceland_low_res.tif
int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    cerr << "Usage: " << argv[0] << " <FileName.tiff>" << endl;
    return EXIT_FAILURE;
  }

  GDALAllRegister();
  string demFilePath = argv[1];

  // Open the dataset using the C++ API
  GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen(demFilePath.c_str(), GA_ReadOnly));
  if (dataset == nullptr)
  {
    cerr << "Failed to open file: " << demFilePath << endl;
    return 1;
  }

  printMetaData(dataset);
  // Find peaks in the DEM data
  auto peaks = FindPeaks(dataset);

  for (auto peak : peaks)
  {
    auto coords = PixelToLatLon(dataset, peak.first, peak.second);

    cout << coords.first << ',' << coords.second << '/';
  }

  return EXIT_SUCCESS;
}
