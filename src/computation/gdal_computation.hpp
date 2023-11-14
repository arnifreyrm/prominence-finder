#include <gdal_priv.h>
#include <vector>

#ifndef COMPUTATION_H
#define COMPUTATION_H

void printMetaData(GDALDataset *dataset);
std::vector<std::pair<double, double>> FindPeaks(GDALDataset *dataset);
std::pair<double, double> PixelToLatLon(GDALDataset *dataset, int pixelX, int pixelY);

#endif