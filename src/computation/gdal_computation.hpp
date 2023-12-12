#include <gdal_priv.h>
#include <vector>
#include <queue>
#include <memory>
#include <set>
#include <memory>
#include <stdexcept>
#include <utility>

#ifndef COMPUTATION_H
#define COMPUTATION_H

/**
 * @brief Represents simple x, y coordinates.
 *
 * Defines a 2D point with x and y integer coordinates. Includes
 * basic operations like equality and comparison for use in sets and maps.
 */
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
/**
 * @brief Manages island characteristics for peak prominence calculations.
 *
 * Handles the properties and interactions of an island, including its peak, edges,
 * and elevation, crucial for determining its prominence in relation to other islands.
 */
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
/**
 * @brief Custom deleter for OGRSpatialReference objects.
 *
 *
 */
auto OGRSpatialReferenceDeleter = [](OGRSpatialReference *ptr)
{
  if (ptr)
    OGRSpatialReference::DestroySpatialReference(ptr);
};
/**
 * @brief Custom deleter for OGRCoordinateTransformation objects.
 *
 * Manages the destruction of OGRCoordinateTransformation objects,
 * preventing memory leaks by correctly freeing allocated resources.
 */
auto OGRCoordinateTransformationDeleter = [](OGRCoordinateTransformation *ptr)
{
  if (ptr)
    OCTDestroyCoordinateTransformation(ptr);
};
/**
 * @brief Handles coordinate transformations for geographic data.
 *
 * Responsible for converting coordinates between different spatial
 * reference systems using GDAL functionalities.
 */
struct Transformer
{
  std::unique_ptr<OGRCoordinateTransformation, decltype(OGRCoordinateTransformationDeleter)> transformation;
  double adfGeoTransform[6];

  explicit Transformer(GDALDataset *dataset)
  {
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
/**
 * @brief Represents a point with elevation and island association data.
 *
 * Stores the elevation of a point and its association with an island,
 * if any. It includes methods to determine island affiliation.
 */
struct Point
{
  unsigned int islandId;
  double elevation;

  Point() : elevation(0.0) {}
  Point(double elevation) : elevation(elevation) {}
  Point(double elevation, unsigned int islandId) : elevation(elevation), islandId(islandId) {}

  bool belongsToAnyIsland() const
  {
    return islandId != 0;
  }

  bool belongsToIsland(const Point &other) const
  {
    return islandId == other.islandId;
  }
};
/**
 * @brief Comparator for island elevation.
 *
 * Defines a comparison rule for islands based on their elevation, used in findPeakIslands to sort the vector based on height
 *
 */
struct CompareIsland
{
  bool operator()(const std::unique_ptr<Island> &a, const std::unique_ptr<Island> &b) const
  {
    return a->elevation < b->elevation;
  }
};
/**
 * @brief Stores key col information for island interactions.
 *
 * Contains data about the key col (lowest point) in the interaction
 * between two islands, including the other island's reference and
 * the col elevation and coordinates. Used in the main water level loop
 */
struct KeyColInfo
{
  std::shared_ptr<Island> otherIsland;
  double colElevation;
  Coords colCoords;
  bool found;
  KeyColInfo(){};
};
/**
 * @brief Holds metadata for the dataset.
 *
 * Encapsulates important information about the dataset, such as
 * maximum and minimum elevations, and the dataset's dimensions.
 */
struct datasetMetadata
{
  double maxElevation;
  double minElevation;
  int height;
  int width;
  datasetMetadata(double maxElevation, double minElevation, int height, int width)
      : maxElevation(maxElevation), minElevation(minElevation), height(height), width(width) {}
};

// Functions defined in their own files

void calculateProminence(std::unique_ptr<GDALDataset> &dataset, std::string outputFilePath, int prominenceThreshold, bool verbose);
void printMetaData(GDALDataset *dataset);
std::vector<Coords> neighbors(Coords coords, int datasetHeight, int datasetWidth);
std::vector<std::shared_ptr<Island>> findPeakIslands(GDALDataset *dataset);
std::pair<datasetMetadata, std::vector<std::vector<Point>>> initializeMatrix(GDALDataset *dataset);
void processKeyCol(Island &island1, Island &island2, double colElevation, std::vector<std::vector<Point>> &pointMatrix);
std::shared_ptr<Island> getIslandIfExists(const std::map<unsigned int, std::shared_ptr<Island>> &map, unsigned int key);
std::pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY);
void appendIslandDataToFile(const std::shared_ptr<Island> &island, const std::string &filename, const std::unique_ptr<Transformer> &transformerPtr);
void initializeCSV(const std::string &filename);

#endif // COMPUTATION_H