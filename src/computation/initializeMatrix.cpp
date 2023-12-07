#include "gdal_computation.hpp"
#include <gdal_priv.h>
#include <vector>
#include <iostream>

using namespace std;

pair<datasetMetadata, vector<vector<Point>>> initializeMatrix(GDALDataset *dataset)
{

  GDALRasterBand *band = dataset->GetRasterBand(1);
  // Get the size of the raster band
  int width = band->GetXSize();
  int height = band->GetYSize();
  // Temporary buffer to hold data
  vector<float> buffer(width * height);

  // Read the entire data
  auto err = band->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Float32, 0, 0);
  if (err)
  {
    cerr << "Error reading band: " << err << '\n';
  }
  double maxElevation = -INFINITY;
  double minElevation = INFINITY;
  vector<vector<Point>> pointMatrix(height, vector<Point>(width));
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      auto elevation = buffer[y * width + x];
      if (elevation > maxElevation)
        maxElevation = elevation;
      if (elevation < minElevation)
        minElevation = elevation;
      pointMatrix[y][x].elevation = elevation;
    }
  }
  return make_pair(datasetMetadata(maxElevation, minElevation, height, width), pointMatrix);
}