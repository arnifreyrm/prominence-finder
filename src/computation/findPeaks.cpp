#include <vector>
#include <thread>
#include <iostream>
#include <gdal_priv.h>
#include <utility>
#include <queue>
#include <ogr_spatialref.h>
#include "gdal_computation.hpp"

using namespace std;

void processRange(const vector<float> &buffer, vector<shared_ptr<Island>> &localPeaks, int startRow, int endRow, int width, int height, int isolationRadius, double noDataValue, bool checkNoData)

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
        localPeaks.push_back(make_shared<Island>(Coords(x, y), current));
      }
    }
  }
}

vector<shared_ptr<Island>> FindPeaks(GDALDataset *dataset, int isolationPixelRadius = 10)
{
  // Get the elevation
  GDALRasterBand *band = dataset->GetRasterBand(1);

  // Get the size of the raster band
  int width = band->GetXSize();
  int height = band->GetYSize();

  // Buffer to hold data
  vector<float> buffer(width * height);

  // Read the entire data
  auto err = band->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Float32, 0, 0);
  if (err)
  {
    cerr << "Error reading band: " << err << '\n';
  }

  int hasNoData;
  double noDataValue = band->GetNoDataValue(&hasNoData);
  bool checkNoData = hasNoData != 0;
  int numThreads = std::thread::hardware_concurrency();
  vector<std::thread> threads(numThreads);
  vector<vector<shared_ptr<Island>>> islandsPerThread(numThreads);

  // Split the dataset into chunks and process it in paralel as it is read-only
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

  vector<shared_ptr<Island>> combinedIslands;
  unsigned int id = 1;
  for (auto &threadIslands : islandsPerThread)
  {
    for (auto &island : threadIslands)
    {
      island->id = id++;
      combinedIslands.push_back(std::move(island));
    }
  }
  // Sort combinedIslands based on the elevation,
  sort(combinedIslands.begin(), combinedIslands.end(),
       [](const shared_ptr<Island> &a, const shared_ptr<Island> &b)
       {
         return a->elevation < b->elevation;
       });
  // By definition, the highest peak in the dataset had a prominence of its elevation
  combinedIslands.back()->prominence = combinedIslands.back()->elevation;

  vector<int> counts(211);

  for (auto isl : combinedIslands)
  {
    counts[int(isl->elevation) / 10]++;
  }
  int i = 0;
  for (auto count : counts)
  {
    cout << i++ << "," << count << '\n';
  }

  return combinedIslands;
}
