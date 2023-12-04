#include <gdal_priv.h>
#include <vector>
#include <queue>
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
  Coords peakCoords;         // Highest point on the island
  std::set<Coords> frontier; // Points on the edge of the island
  double elevation;

  Island(const Coords &peakCoords, double elevation) : peakCoords(peakCoords), elevation(elevation)
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
    return bool(islandId);
  }

  bool belongsToSamePeak(const Point &other) const
  {
    return islandId == other.islandId;
  }
};

struct CompareIsland
{
  bool operator()(const Island &a, const Island &b) const
  {
    return a.elevation < b.elevation;
  }
};

struct datasetMetadata
{
  double maxElevation;
  int height;
  int width;
  datasetMetadata(double maxElevation, int height, int width)
      : maxElevation(maxElevation), height(height), width(width) {}
};

void printMetaData(GDALDataset *dataset);
std::priority_queue<Island, std::vector<Island>, CompareIsland> FindPeaks(GDALDataset *dataset, int isolationPixelRadius);
std::pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY);

#endif // COMPUTATION_H