#include <gdal_priv.h>
#include <iostream>
#include "gdal_computation.hpp"
void printMetaData(GDALDataset *dataset)
{
  GDALDriver *driver = dataset->GetDriver();
  if (driver != nullptr)
  {
    std::cout << "Driver: " << driver->GetDescription() << "/" << driver->GetMetadataItem(GDAL_DMD_LONGNAME) << '\n';
  }

  std::cout << "Size is " << dataset->GetRasterXSize() << "x" << dataset->GetRasterYSize() << "x" << dataset->GetRasterCount() << '\n';

  if (dataset->GetProjectionRef() != nullptr)
    std::cout << "Projection is `" << dataset->GetProjectionRef() << "`" << '\n';

  double adfGeoTransform[6];
  if (dataset->GetGeoTransform(adfGeoTransform) == CE_None)
  {
    std::cout << "Origin = (" << adfGeoTransform[0] << "," << adfGeoTransform[3] << ")" << '\n';
    std::cout << "Pixel Size = (" << adfGeoTransform[1] << "," << adfGeoTransform[5] << ")" << '\n';
  }
}