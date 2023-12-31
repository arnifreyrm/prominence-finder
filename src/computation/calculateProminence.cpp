#include "gdal_computation.hpp"
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

using namespace std;

/**
 * @brief Calculates peak prominences in a dataset using the water level method.
 *
 * Processes a geographic dataset to determine the prominence of peaks. Peaks with
 * prominence below the specified threshold are excluded from the output. The method
 * simulates lowering water levels to identify and analyze individual islands (peaks).
 *
 * @param dataset Unique pointer to the GDALDataset being processed.
 * @param outputFilePath Path to the output CSV file for storing results.
 * @param prominenceThreshold Minimum prominence value for peaks to be included in the output.
 * @param verbose If true, additional details like water level and active island count are printed during processing.
 */
void calculateProminence(unique_ptr<GDALDataset> &dataset, const string outputFilePath, int prominenceThreshold = 0, bool verbose = false)
{
  vector<shared_ptr<Island>> islandPeaks = findPeakIslands(dataset.get());
  auto matrixData = initializeMatrix(dataset.get());

  unique_ptr<Transformer> coordinateTransformer;
  if (dataset->GetProjectionRef())
  {
    coordinateTransformer = make_unique<Transformer>(dataset.get());
  }
  if (!outputFilePath.empty())
  {
    initializeCSV(outputFilePath);
  }
  // Extract metadata
  auto metaData = matrixData.first;
  vector<vector<Point>> pointMatrix = matrixData.second;
  int height = metaData.height;
  int width = metaData.width;
  // Set the water level to the highest point
  int waterLevel = int(metaData.maxElevation);
  double minElevation = metaData.minElevation;
  vector<shared_ptr<Island>> activeIslands;
  map<unsigned int, shared_ptr<Island>> idToIslandMap;

  // Explicitly release the dataset as we don't need it any more -- not the best but works
  dataset.reset();

  if (verbose)
    cout << "Starting water level prominence calculations for  " << islandPeaks.size() << '\n';

  while (waterLevel >= minElevation)
  {
    if (verbose)
      cout << "Water level: " << waterLevel << " Active island count " << activeIslands.size() << '\n';

    while (!islandPeaks.empty() && islandPeaks.back()->elevation >= waterLevel)
    {
      shared_ptr<Island> &islandPeak = islandPeaks.back();
      unsigned int islandId = islandPeak->id;
      pointMatrix[islandPeak->peakCoords.y][islandPeak->peakCoords.x].islandId = islandId;
      idToIslandMap[islandId] = islandPeak;
      activeIslands.push_back(islandPeak);
      islandPeaks.pop_back();
    }

    for (auto it = activeIslands.begin(); it != activeIslands.end();)
    {
      if ((*it)->flaggedForDeletion)
      {
        // Append data to file before deleting
        if (!outputFilePath.empty() && (*it)->prominence > prominenceThreshold)
          appendIslandDataToFile(*it, outputFilePath, coordinateTransformer);

        // Erase from map first to avoid dangling references
        idToIslandMap.erase((*it)->id);

        // Erase from vector and update the iterator
        it = activeIslands.erase(it);
      }
      else
      {
        auto &island = **it;
        bool frontierExpanded;

        do
        {
          frontierExpanded = false;
          bool nextToWater = false;
          set<Coords> newFrontier;
          KeyColInfo keyCol;
          keyCol.found = false;
          for (Coords coords : island.frontier)
          {

            Point &frontierPoint = pointMatrix[coords.y][coords.x];
            bool hasUpdated = false;

            // Check the neighboring points
            for (auto neighborCoords : neighbors(coords, height, width))
            {
              int j = neighborCoords.y;
              int i = neighborCoords.x;
              Point neighborPoint = pointMatrix[j][i];
              // Check if current point is next to water to see if we keep it in the frontier
              if (neighborPoint.elevation < waterLevel)
              {
                nextToWater = true;
              }
              // If the neighboring point is not claimed by any island and is above the water line we will add it to the new frontier
              if (!neighborPoint.belongsToIsland(frontierPoint) && neighborPoint.elevation >= waterLevel)
              {
                if (!neighborPoint.belongsToAnyIsland())
                {
                  pointMatrix[j][i].islandId = island.id;
                  newFrontier.emplace(Coords(i, j));
                  hasUpdated = true;
                  frontierExpanded = true;
                }
                else if (!island.dominatedIslands.contains(neighborPoint.islandId) && !keyCol.found) // If it is a part of another island we haven't seen before and we haven't reached the key col earlier in the iteration, we have reached a key col and we can calculate prominence
                {
                  auto otherIslandPtr = getIslandIfExists(idToIslandMap, neighborPoint.islandId);
                  if (otherIslandPtr == nullptr)
                    continue;
                  auto &otherIsland = *otherIslandPtr;
                  keyCol.otherIsland = otherIslandPtr;
                  keyCol.colElevation = min(neighborPoint.elevation, frontierPoint.elevation);
                  keyCol.colCoords = coords;
                  keyCol.found = true;
                  double colElevation = min(neighborPoint.elevation, frontierPoint.elevation);
                }
              };
            }

            // If the frontier does not update, but is still next to the water, keep the old value
            if (!hasUpdated && nextToWater)
            {
              newFrontier.emplace(coords);
            }
          }
          island.frontier = newFrontier;
          if (keyCol.found)
          {
            processKeyCol(island, *keyCol.otherIsland, keyCol.colElevation, pointMatrix);
            if (island.elevation < keyCol.otherIsland.get()->elevation)
              frontierExpanded = false; // We don't want to keep iterating the frontier outwards
          }
        } while (frontierExpanded);

        ++it;
      }
    }
    // Drain the water level down
    waterLevel -= 1;
  }
  // Append any reamining islands to the file
  if (!outputFilePath.empty())
  {
    for (auto &island : activeIslands)
    {
      appendIslandDataToFile(island, outputFilePath, coordinateTransformer);
    }
  }
}