#include "gdal_computation.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// Appends the peak results to a csv file
void appendIslandDataToFile(const shared_ptr<Island> &island, const string &filename = "../results/peaks.csv", const unique_ptr<Transformer> &transformerPtr = nullptr)
{
  ofstream outFile(filename, ios::app);

  if (!outFile.is_open())
  {
    cerr << "Error: Unable to open file for appending.\n";
    return;
  }
  if (transformerPtr == nullptr)
  {
    outFile << island->peakCoords.x << ","
            << island->peakCoords.y << ","
            << island->prominence << ","
            << island->elevation << "\n";
  }
  else
  {
    auto latLong = transformerPtr->transform(island->peakCoords.x, island->peakCoords.y);
    outFile << island->peakCoords.x << ","
            << island->peakCoords.y << ","
            << island->prominence << ","
            << latLong.first << ","
            << latLong.second << ","
            << island->elevation << "\n";
  }

  outFile.close();
}

void initializeCSV(const string &filename = "../results/peaks.csv")
{
  ofstream outFile(filename);

  if (!outFile.is_open())
  {
    cerr << "Error: Unable to open file for writing.\n";
  }

  outFile << "x,"
          << "y,"
          << "prominence,"
          << "latitude,"
          << "longitude,"
          << "elevation"
          << "\n";

  outFile.close();
}