#include <vector>
#include <thread>
#include <iostream>
#include <gdal_priv.h>
#include <queue>
#include <ogr_spatialref.h>
#include "gdal_computation.hpp"

using namespace std;

void processRange(const vector<float> &buffer, vector<Island> &localPeaks, int startRow, int endRow, int width, int height, int isolationRadius, double noDataValue, bool checkNoData)
{
  for (int y = startRow; y < endRow; ++y) // Adjusted to include edge pixels
  {
    for (int x = 0; x < width; ++x) // Adjusted to include edge pixels
    {
      float current = buffer[y * width + x];
      if (checkNoData && current == noDataValue)
      {
        continue;
      }

      bool isPeak = true;

      for (int ny = -isolationRadius; ny <= isolationRadius; ++ny)
      {
        for (int nx = -isolationRadius; nx <= isolationRadius; ++nx)
        {
          // Skip the current point itself
          if (nx == 0 && ny == 0)
            continue;

          int neighborX = x + nx;
          int neighborY = y + ny;

          // Boundary check
          if (neighborX < 0 || neighborX >= width || neighborY < 0 || neighborY >= height)
            continue; // Skip this neighbor if it's out of bounds

          if (buffer[neighborY * width + neighborX] >= current)
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
        localPeaks.push_back(Island(Coords(x, y), current));
      }
    }
  }
}

priority_queue<Island, vector<Island>, CompareIsland> FindPeaks(GDALDataset *dataset, int isolationPixelRadius = 10)
{
  // Get the first band (elevation values)
  GDALRasterBand *band = dataset->GetRasterBand(1);

  // Get the size of the raster band
  int width = band->GetXSize();
  int height = band->GetYSize();

  // Buffer to hold data
  vector<float> buffer(width * height);

  // Read the entire data
  band->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Float32, 0, 0);

  int hasNoData;
  double noDataValue = band->GetNoDataValue(&hasNoData);
  bool checkNoData = hasNoData != 0;
  int numThreads = std::thread::hardware_concurrency();
  vector<std::thread> threads(numThreads);
  vector<vector<Island>> islandsPerThread(numThreads);

  int chunkSize = height / numThreads;
  for (int i = 0; i < numThreads; ++i)
  {
    int startRow = i * chunkSize;
    int endRow = (i + 1) * chunkSize;
    if (i == numThreads - 1)
    {
      endRow = height;
    }

    threads[i] = std::thread(processRange, std::ref(buffer), std::ref(islandsPerThread[i]), startRow, endRow, width, height, isolationPixelRadius, noDataValue, checkNoData);
  }

  for (auto &t : threads)
  {
    t.join();
  }

  vector<Island> combinedIslands;
  for (const auto &threadIslands : islandsPerThread)
  {
    combinedIslands.insert(combinedIslands.end(), threadIslands.begin(), threadIslands.end());
  }
  priority_queue<Island, vector<Island>, CompareIsland> IslandPQ;
  unsigned int id = 0;
  for (auto &island : combinedIslands)
  {
    island.id = id++;
    IslandPQ.push(island);
  }
  return IslandPQ;
}
