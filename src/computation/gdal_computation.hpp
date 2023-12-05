#include <gdal_priv.h>
#include <vector>
#include <queue>
#include <memory>
#include <optional>
#include <set>

#ifndef COMPUTATION_H
#define COMPUTATION_H

struct Coords
{
  int x;
  int y;
  Coords(int x, int y) : x(x), y(y) {}
  bool operator==(const Coords &other) const
  {
    return x == other.x && y == other.y;
  }
  bool operator<(const Coords &other) const
  {
    if (x == other.x)
      return y < other.y;
    return x < other.x;
  }
};
class Island
{
public:
  unsigned int id;
  Coords peakCoords;                       // Highest point on the island
  std::set<Coords> frontier;               // Points on the edge of the island
  std::set<unsigned int> dominatedIslands; // Ids of other, lower, islands that this Island has come in contact with.
  bool flaggedForDeletion;                 // If dominated by another island, set to true and delete it from the vector when we next see it.
  double elevation;
  double prominence;

  Island(const Coords &peakCoords, double elevation) : peakCoords(peakCoords), elevation(elevation), flaggedForDeletion(false)
  {
    frontier.emplace(peakCoords);
  }
};

struct Point
{
  unsigned int islandId;
  double elevation;

  Point() : elevation(0.0) {}
  Point(double elevation) : elevation(elevation) {}
  Point(double elevation, unsigned int islandId) : elevation(elevation), islandId(islandId) {}

  bool hasPeak() const
  {
    return islandId != 0;
  }

  bool belongsToSamePeak(const Point &other) const
  {
    return islandId == other.islandId;
  }
};

struct CompareIsland
{
  bool operator()(const std::unique_ptr<Island> &a, const std::unique_ptr<Island> &b) const
  {
    return a->elevation < b->elevation; // Or your comparison logic
  }
};

struct datasetMetadata
{
  double maxElevation;
  double minElevation;
  int height;
  int width;
  datasetMetadata(double maxElevation, double minElevation, int height, int width)
      : maxElevation(maxElevation), minElevation(minElevation), height(height), width(width) {}
};

void printMetaData(GDALDataset *dataset);
std::vector<std::shared_ptr<Island>> FindPeaks(GDALDataset *dataset, int isolationPixelRadius);
std::pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY);

#endif // COMPUTATION_H