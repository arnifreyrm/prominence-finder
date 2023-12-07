#include <iostream>
#include <memory>
#include <gdal.h>
#include <utility>
#include <vector>
#include <gdal_priv.h>
#include <set>
#include <string>
#include <map>
#include <fstream>
#include <ogr_spatialref.h>
#include "visualization/visualizeTif.hpp"
#include "computation/gdal_computation.hpp"
using namespace std;

// Function to safely check and get an island from the map
shared_ptr<Island> getIslandIfExists(const map<unsigned int, shared_ptr<Island>> &map, unsigned int key)
{
  auto it = map.find(key);
  if (it != map.end())
  {
    return it->second;
  }
  // Key not found, return nullptr
  return nullptr;
}
void appendIslandDataToFile(const shared_ptr<Island> &island, const string &filename)
{
  ofstream outFile(filename, ios::app); // Open in append mode

  if (!outFile.is_open())
  {
    cerr << "Error: Unable to open file for appending.\n";
    return;
  }

  outFile << island->peakCoords.x << ","
          << island->peakCoords.y << ","
          << island->prominence << ","
          << island->elevation << "\n";

  outFile.close();
}
void readToCSV(vector<shared_ptr<Island>> islands)
{
  ofstream outFile("../peak_data3.csv");

  if (!outFile.is_open())
  {
    cerr << "Error: Unable to open file for writing.\n"; // or handle the error as you prefer
  }

  outFile << "x,y,prominence,elevation\n";

  for (const auto &island : islands)
  {
    if (island)
    {
      outFile << island->peakCoords.x << ","
              << island->peakCoords.y << ","
              << island->prominence << ","
              << island->elevation << "\n";
    }
  }
  outFile.close();
}

void processKeyCol(Island &island1, Island &island2, double colElevation, vector<vector<Point>> &pointMatrix)
{
  Island *lowerIsland, *higherIsland;

  // Determine which island is higher
  if (island1.elevation <= island2.elevation)
  {
    lowerIsland = &island1;
    higherIsland = &island2;
  }
  else
  {
    lowerIsland = &island2;
    higherIsland = &island1;
  }

  // Transfer ownership of the lower island's frontier points to the higher island
  for (const auto &lowerIslandCoord : lowerIsland->frontier)
  {
    pointMatrix[lowerIslandCoord.y][lowerIslandCoord.x].islandId = higherIsland->id;
  }
  higherIsland->frontier.insert(lowerIsland->frontier.begin(), lowerIsland->frontier.end());
  higherIsland->dominatedIslands.insert(lowerIsland->id);
  higherIsland->dominatedIslands.insert(lowerIsland->dominatedIslands.begin(), lowerIsland->dominatedIslands.end());

  // Set the prominence and flag for deletion
  lowerIsland->flaggedForDeletion = true;
  lowerIsland->prominence = lowerIsland->elevation - colElevation;
}

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
    cerr << "Usage: " << argv[0] << " <FileName.tiff> [-o output.csv] [-v]" << endl;
    return EXIT_FAILURE;
  }

  GDALAllRegister();

  string demFilePath = argv[1];
  string outputFilePath;
  bool visualize = false;

  for (int i = 2; i < argc; i++)
  {
    string arg = argv[i];
    if (arg == "-v")
    {
      visualize = true;
    }
    else if (arg == "-o" && i + 1 < argc)
    {
      outputFilePath = argv[++i]; // Increment i to skip the next argument as it is the file path for -o
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

  if (outputFilePath.empty())
  {
    cerr << "No output file specified with flag -o";
    return EXIT_FAILURE;
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
  Transformer coordinateTransformer = Transformer(dataset.get());
  auto metaData = matrixData.first;
  vector<vector<Point>> pointMatrix = matrixData.second;
  int height = metaData.height;
  int width = metaData.width;
  int waterLevel = int(metaData.maxElevation);
  double minElevation = metaData.minElevation;
  vector<shared_ptr<Island>> islands;                  // Stores shared_ptr to Island objects that are in use
  map<unsigned int, shared_ptr<Island>> idToIslandMap; // Maps IDs to unique_ptr of Island

  cout << "Starting water level prominence calculations for  " << islandPeaks.size() << '\n';

  while (waterLevel > 0)
  {
    double nextWaterLevel = minElevation;
    cout << "Water level: " << waterLevel << " Active island count " << islands.size() << '\n';
    while (!islandPeaks.empty() && islandPeaks.back()->elevation >= waterLevel)
    {
      shared_ptr<Island> &islandPeak = islandPeaks.back();
      unsigned int islandId = islandPeak->id;
      pointMatrix[islandPeak->peakCoords.y][islandPeak->peakCoords.x].islandId = islandId;

      idToIslandMap[islandId] = islandPeak;
      islands.push_back(islandPeak);
      islandPeaks.pop_back();
    }
    // Set the highest submerged elevation as the next island to emerge
    if (!islandPeaks.empty())
    {
      nextWaterLevel = islandPeaks.back()->elevation;
    }

    for (auto it = islands.begin(); it != islands.end();)
    {
      if ((*it)->flaggedForDeletion)
      {
        // Append data to file before deleting
        appendIslandDataToFile(*it, outputFilePath);

        // Erase from map first to avoid dangling references
        idToIslandMap.erase((*it)->id);

        // Erase from vector and update the iterator
        it = islands.erase(it);
      }
      else
      {
        auto &island = **it;
        bool frontierExpanded;

        // printMatrix(pointMatrix);

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
              // Update the highest submerged point if needed
              if (neighborPoint.elevation < waterLevel)
              {
                // cout << "Point " << coords.x << ',' << coords.y << ",id:" << neighborPoint.islandId << " at elevation " << frontierPoint.elevation << " next to water at point " << i << ',' << j << " at elevation " << neighborPoint.elevation << " with water level " << waterLevel << '\n';
                nextToWater = true;
                if (neighborPoint.elevation > nextWaterLevel)
                  nextWaterLevel = neighborPoint.elevation;
              }
              // If the neighboring point has no parent peak and is above the water line we will add it to the new frontier
              if (!neighborPoint.belongsToSamePeak(frontierPoint) && neighborPoint.elevation >= waterLevel)
              {
                if (!neighborPoint.hasPeak())
                {
                  pointMatrix[j][i].islandId = island.id;
                  newFrontier.emplace(Coords(i, j));
                  hasUpdated = true;
                  frontierExpanded = true;
                }
                else if (!island.dominatedIslands.contains(neighborPoint.islandId)) // If it is a part of another island we haven't seen before, we have reached a key col and we can calculate prominence
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
                  processKeyCol(island, otherIsland, colElevation, pointMatrix);
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
          }
        } while (frontierExpanded);

        ++it;
      }
    }

    // drain the water level down to the next point
    waterLevel -= 1;
  }
  // append any reamining islands to the file
  for (auto &island : islands)
  {
    appendIslandDataToFile(island, outputFilePath);
  }
  return EXIT_SUCCESS;
}
