#include <gdal_priv.h>
#include <vector>
#include <queue>
#include <memory>
#include <optional>
#include <set>
#include <memory>
#include <stdexcept>
#include <utility>

#ifndef COMPUTATION_H
#define COMPUTATION_H

struct Coords
{
  int x;
  int y;
  Coords(int x, int y) : x(x), y(y) {}
  Coords() {}
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
    frontier.insert(peakCoords);
  }
};

auto OGRSpatialReferenceDeleter = [](OGRSpatialReference *ptr)
{
  if (ptr)
    OGRSpatialReference::DestroySpatialReference(ptr);
};
auto gdalDeleter = [](GDALDataset *dataset)
{
  GDALClose(dataset);
};
auto OGRCoordinateTransformationDeleter = [](OGRCoordinateTransformation *ptr)
{
  if (ptr)
    OCTDestroyCoordinateTransformation(ptr);
};
struct Transformer
{
  std::unique_ptr<OGRCoordinateTransformation, decltype(OGRCoordinateTransformationDeleter)> transformation;
  double adfGeoTransform[6];

  explicit Transformer(GDALDataset *dataset)
  {
    // Similar setup as in PixelToLatLon function
    const char *wkt = dataset->GetProjectionRef();
    if (!wkt)
    {
      throw std::runtime_error("Projection reference is null.");
    }

    auto sourceSRS = std::unique_ptr<OGRSpatialReference, decltype(OGRSpatialReferenceDeleter)>(
        new OGRSpatialReference(wkt), OGRSpatialReferenceDeleter);

    auto targetSRS = std::unique_ptr<OGRSpatialReference, decltype(OGRSpatialReferenceDeleter)>(
        new OGRSpatialReference(), OGRSpatialReferenceDeleter);
    targetSRS->importFromEPSG(4326);

    transformation = std::unique_ptr<OGRCoordinateTransformation, decltype(OGRCoordinateTransformationDeleter)>(
        OGRCreateCoordinateTransformation(sourceSRS.get(), targetSRS.get()), OGRCoordinateTransformationDeleter);

    if (!transformation)
    {
      throw std::runtime_error("Failed to create coordinate transformation.");
    }

    if (dataset->GetGeoTransform(adfGeoTransform) != CE_None)
    {
      throw std::runtime_error("Failed to get GeoTransform.");
    }
  }
  std::pair<double, double> transform(int pixelX, int pixelY)
  {
    double x = adfGeoTransform[0] + pixelX * adfGeoTransform[1] + pixelY * adfGeoTransform[2];
    double y = adfGeoTransform[3] + pixelX * adfGeoTransform[4] + pixelY * adfGeoTransform[5];

    if (!transformation->Transform(1, &x, &y))
    {
      throw std::runtime_error("Failed to transform coordinates.");
    }

    return std::make_pair(x, y);
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
    return a->elevation < b->elevation;
  }
};
struct KeyColInfo
{
  std::shared_ptr<Island> otherIsland;
  double colElevation;
  Coords colCoords;
  bool found;
  KeyColInfo(){};
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
std::vector<Coords> neighbors(Coords coords, int datasetHeight, int datasetWidth);
std::vector<std::shared_ptr<Island>> FindPeaks(GDALDataset *dataset, int isolationPixelRadius);
std::pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY);
void printMatrixElevation(std::vector<std::vector<Point>> &pointMatrix);
void printMatrix(std::vector<std::vector<Point>> &pointMatrix);
void printFrontier(std::vector<std::vector<Point>> &pointMatrix, std::set<Coords> frontier);

#endif // COMPUTATION_H