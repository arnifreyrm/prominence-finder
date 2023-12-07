#include <vector>
#include <gdal_priv.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include "gdal_computation.hpp"
using namespace std;

pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY)
{
  if (!dataset)
  {
    throw runtime_error("Dataset not found.");
  }

  // Get projection reference
  const char *wkt = dataset->GetProjectionRef();
  if (!wkt)
  {
    throw runtime_error("Projection reference is null.");
  }

  // Create source spatial reference
  auto sourceSRS = unique_ptr<OGRSpatialReference, decltype(OGRSpatialReferenceDeleter)>(
      new OGRSpatialReference(wkt), OGRSpatialReferenceDeleter);

  // Create target spatial reference (WGS84)
  auto targetSRS = unique_ptr<OGRSpatialReference, decltype(OGRSpatialReferenceDeleter)>(
      new OGRSpatialReference(), OGRSpatialReferenceDeleter);
  targetSRS->importFromEPSG(4326);

  // Create transformation
  auto transformation = unique_ptr<OGRCoordinateTransformation, decltype(OGRCoordinateTransformationDeleter)>(
      OGRCreateCoordinateTransformation(sourceSRS.get(), targetSRS.get()), OGRCoordinateTransformationDeleter);

  if (!transformation)
  {
    throw runtime_error("Failed to create coordinate transformation.");
  }

  double adfGeoTransform[6];
  if (dataset->GetGeoTransform(adfGeoTransform) != CE_None)
  {
    throw runtime_error("Failed to get GeoTransform.");
  }

  double x = adfGeoTransform[0] + pixelX * adfGeoTransform[1] + pixelY * adfGeoTransform[2];
  double y = adfGeoTransform[3] + pixelX * adfGeoTransform[4] + pixelY * adfGeoTransform[5];

  if (!transformation->Transform(1, &x, &y))
  {
    throw runtime_error("Failed to transform coordinates.");
  }
  return make_pair(x, y);
}
