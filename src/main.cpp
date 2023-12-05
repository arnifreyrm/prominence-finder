#include <iostream>
#include <memory>
#include <gdal.h>
#include <utility>
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

  auto islandPeaks = FindPeaks(dataset.get(), 15);
  auto matrixData = initializeMatrix(dataset.get());
  auto metaData = matrixData.first;
  vector<vector<Point>> pointMatrix = matrixData.second;
  int height = metaData.height;
  int width = metaData.width;
  double waterLevel = metaData.maxElevation;
  double minElevation = metaData.minElevation;
  vector<shared_ptr<Island>> islands;                  // Stores shared_ptr to Island objects
  map<unsigned int, shared_ptr<Island>> idToIslandMap; // Maps IDs to unique_ptr of Island
  map<unsigned int, unsigned int> islandAncestry;

  cout << "Starting water level prominence calculations for  " << islandPeaks.size() << '\n';

  while (waterLevel > minElevation)
  {
    double nextWaterLevel = minElevation;

    cout << "Water level: " << waterLevel << '\n';
    // printMatrix(pointMatrix);
    while (!islandPeaks.empty() && islandPeaks.back()->elevation >= waterLevel)
    {
      shared_ptr<Island> &islandPeak = islandPeaks.back();
      // cout << "Island at: " << islandPeak->peakCoords.x << ',' << islandPeak->peakCoords.y << '\n';
      unsigned int islandId = islandPeak->id;
      pointMatrix[islandPeak->peakCoords.y][islandPeak->peakCoords.x].islandId = islandId;

      idToIslandMap[islandId] = islandPeak;
      islands.push_back(islandPeak);
      islandPeaks.pop_back();
    }
    // Set the highest submerged elevation as the next island to emerge
    if (!islandPeaks.empty())
      nextWaterLevel = islandPeaks.back()->elevation;

    for (auto it = islands.begin(); it != islands.end(); it++)
    {
      if ((*it)->flaggedForDeletion)
      {
        continue;
      }
      else
      {
        auto &island = **it;
        // cout << "checking island " << island.id << '\n';
        set<Coords> newFrontier;
        bool foundCol = false;

        for (Coords coords : island.frontier)
        {
          if (foundCol)
            break;
          Point &frontierPoint = pointMatrix[coords.y][coords.x];
          bool hasUpdated = false;
          // Check the neighboring points

          for (int i = coords.x - 1; i <= coords.x + 1 && !foundCol; i++)
          {
            if (i < 0 || i >= width)
              continue;
            for (int j = coords.y - 1; j <= coords.y + 1 && !foundCol; j++)
            {
              if (j < 0 || j >= height || (j == coords.y && i == coords.x))
                continue;
              Point neighborPoint = pointMatrix[j][i];
              // Update the highest submerged point if needed
              if (neighborPoint.elevation < waterLevel && neighborPoint.elevation >= nextWaterLevel)
                nextWaterLevel = neighborPoint.elevation;
              // If the neighboring point has no parent peak and is above the water line we will add it to the new frontier
              if (!neighborPoint.belongsToSamePeak(frontierPoint) && neighborPoint.elevation >= waterLevel)
              {
                if (!neighborPoint.hasPeak())
                {
                  pointMatrix[j][i].islandId = island.id;
                  newFrontier.emplace(Coords(i, j));
                  hasUpdated = true;
                }
                else if (!island.dominatedIslands.contains(neighborPoint.islandId)) // If it is a part of another island we haven't seen before, we have reached a key col and we can calculate prominence
                {
                  // cout << "Col found at neighbor point eleveation: " << neighborPoint.elevation << ", with current point elevation" << frontierPoint.elevation << " with current island id" << frontierPoint.islandId << " and other island id: " << neighborPoint.islandId << '\n';
                  // cout << "Current island domination set:";
                  // for (auto id : island.dominatedIslands)
                  //   cout << id;
                  // cout << '\n';
                  auto &otherIsland = *idToIslandMap[neighborPoint.islandId];
                  if (island.elevation <= otherIsland.elevation)
                  {
                    for (auto lowerIslandCoords : island.frontier)
                    {
                      pointMatrix[lowerIslandCoords.y][lowerIslandCoords.x].islandId = otherIsland.id;
                    }
                    otherIsland.frontier.insert(island.frontier.begin(), island.frontier.end());
                    otherIsland.dominatedIslands.insert(island.id);
                    islandAncestry[island.id] = otherIsland.id;
                    // We erase the current island from the vector now, as we have the iterator
                    island.flaggedForDeletion = true;
                    island.prominence = island.elevation - min(neighborPoint.elevation, frontierPoint.elevation);
                    foundCol = true;
                    break;
                  }
                  else
                  {
                    for (auto lowerIslandCoords : otherIsland.frontier)
                    {
                      pointMatrix[lowerIslandCoords.y][lowerIslandCoords.x].islandId = island.id;
                    }
                    island.dominatedIslands.insert(island.id);
                    islandAncestry[otherIsland.id] = island.id;
                    newFrontier.insert(otherIsland.frontier.begin(), otherIsland.frontier.end());
                    otherIsland.prominence = otherIsland.elevation - min(neighborPoint.elevation, frontierPoint.elevation);
                    // We flag the island for deletion, and delete it the next time we see it instead of using find_if to locate the iterator
                    otherIsland.flaggedForDeletion = true;
                  }
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
    }
    // drain the water level down to the next point
    waterLevel = nextWaterLevel;
  }
  return EXIT_SUCCESS;
}
