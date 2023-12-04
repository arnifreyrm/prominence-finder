#include <iostream>
#include <memory>
#include <gdal.h>
#include <vector>
#include <gdal_priv.h>
#include <set>
#include <string>
#include <map>
#include <ogr_spatialref.h>
#include "visualization/visualizeTif.hpp"
#include "computation/gdal_computation.hpp"
using namespace std;

void printMatrix(vector<vector<Point>> &pointMatrix)
{
  for (auto &row : pointMatrix)
  {
    for (auto &col : row)
    {

      cout << '[' << col.islandId << "] ";
    }
    cout << '\n';
  }
}

auto gdalDeleter = [](GDALDataset *dataset)
{
  GDALClose(dataset);
};
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
  double maxElevation = 0;
  vector<vector<Point>> pointMatrix(height, vector<Point>(width));
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      auto elevation = buffer[y * width + x];
      if (elevation > maxElevation)
        maxElevation = elevation;
      pointMatrix[y][x].elevation = elevation;
    }
  }
  return make_pair(datasetMetadata(maxElevation, height, width), pointMatrix);
}
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
  if (argc == 3 && string(argv[2]) == "-v")
  {
    visualizeTif(demFilePath);
    return 0;
  }

  unique_ptr<GDALDataset, decltype(gdalDeleter)> dataset(static_cast<GDALDataset *>(GDALOpen(demFilePath.c_str(), GA_ReadOnly)), gdalDeleter);
  if (dataset == nullptr)
  {
    cerr << "Failed to open file: " << demFilePath << endl;
    return 1;
  }
  // Set the water level to the highest point

  auto islandPeaks = FindPeaks(dataset.get(), 1);
  auto matrixData = initializeMatrix(dataset.get());
  auto metaData = matrixData.first;
  vector<vector<Point>> pointMatrix = matrixData.second;
  int height = metaData.height;
  int width = metaData.width;
  double waterLevel = metaData.maxElevation;
  vector<Island> islands;
  map<unsigned int, Island> idToIslandMap;

  while (waterLevel > -1)
  {
    cout << "Water level: " << waterLevel << '\n';
    printMatrix(pointMatrix);
    while (islandPeaks.top().elevation >= waterLevel && !islandPeaks.empty())
    {
      Island islandPeak = islandPeaks.top();
      cout << "Island at: " << islandPeak.peakCoords.x << ',' << islandPeak.peakCoords.y << '\n';
      islands.emplace_back(islandPeak);
      idToIslandMap.insert(make_pair(islandPeak.id, islandPeak));
      pointMatrix[islandPeak.peakCoords.y][islandPeak.peakCoords.x].islandId = islandPeak.id;
      islandPeaks.pop();
    }

    for (auto &island : islands)
    {
      cout << "checking island " << island.peakCoords.x << '\n';
      set<Coords> newFrontier;

      for (Coords coords : island.frontier)
      {

        Point &frontierPoint = pointMatrix[coords.y][coords.x];
        bool hasUpdated = false;
        // Check the neighboring points
        for (int i = coords.x - 1; i <= coords.x + 1; i++)
        {
          if (i < 0 || i >= width)
            continue;
          for (int j = coords.y - 1; j <= coords.y + 1; j++)
          {
            if (j < 0 || j >= height || (j == coords.y && i == coords.x))
              continue;
            Point neighborPoint = pointMatrix[j][i];
            // If the neighboring point has no parent peak and is above the water line we will add it to the new frontier
            if (!neighborPoint.belongsToSamePeak(frontierPoint) && neighborPoint.elevation >= waterLevel)
            {
              if (neighborPoint.hasPeak()) // If it is a part of another island, we have reached a key col and we can calculate prominence
              {
                cout << "Col found at: " << i << ", " << j << ", " << neighborPoint.elevation << '\n';
                auto otherIsland = idToIslandMap.at(neighborPoint.islandId);
                if (island.elevation < otherIsland.elevation)
                {
                  // otherIsland->frontier.insert(otherIsland->frontier.end(), island.frontier.begin(), island.frontier.end());
                  //  TODO Erase the lower island from the list of islands
                }
                else
                {
                  // island.frontier.insert(island.frontier.end(), otherIsland->frontier.begin(), otherIsland->frontier.end());
                  //  TODO Erase the lower island from the list of islands
                }
                break;
              }
              if (!neighborPoint.hasPeak())
              {
                pointMatrix[j][i].islandId = island.id;
                newFrontier.emplace(Coords(i, j));
                hasUpdated = true;
              }
            };
          }
        }
        // If the frontier does not update, keep the old value
        if (!hasUpdated)
          newFrontier.emplace(coords);
      }
      island.frontier = newFrontier;
    }
    waterLevel -= 1;
  }

  return EXIT_SUCCESS;
}
