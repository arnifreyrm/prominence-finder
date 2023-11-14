
#include <vector>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include "gdal_computation.hpp"

using namespace std;

vector<pair<double, double>> FindPeaks(GDALDataset *dataset)
{
  // Define the size of the portion to process (e.g., 100x100 pixels)
  int portionSize = 1500;

  // Get the first band (elevation values)
  GDALRasterBand *band = dataset->GetRasterBand(1);

  // Buffer to hold data
  vector<float> buffer(portionSize * portionSize);

  // Read a small portion of the data (starting from the top-left corner)
  band->RasterIO(GF_Read, 0, 0, portionSize, portionSize, buffer.data(), portionSize, portionSize, GDT_Float32, 0, 0);
  vector<pair<double, double>> peaks;
  // Process the buffer to find peaks
  for (int y = 1; y < portionSize - 1; ++y)
  {
    for (int x = 1; x < portionSize - 1; ++x)
    {
      float current = buffer[y * portionSize + x];
      bool isPeak = true;

      // Check neighbors to determine if current point is a peak
      for (int ny = -1; ny <= 1; ++ny)
      {
        for (int nx = -1; nx <= 1; ++nx)
        {
          if (nx == 0 && ny == 0)
            continue;
          if (buffer[(y + ny) * portionSize + (x + nx)] >= current)
          {
            isPeak = false;
            break;
          }
        }
        if (!isPeak)
          break;
      }

      if (isPeak)
      {
        peaks.push_back(make_pair(x, y));
      }
    }
  }
  return peaks;
}