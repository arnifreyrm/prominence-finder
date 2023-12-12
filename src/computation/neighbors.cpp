#include <vector>
#include "gdal_priv.h"
#include "gdal_computation.hpp"

using namespace std;
/**
 * @brief Determines the neighboring coordinates of a given point in a 2D grid.
 *
 * Generates a list of adjacent coordinates for a specified point within the confines
 * of a grid defined by datasetHeight and datasetWidth. This function considers
 * eight directions (N, NE, E, SE, S, SW, W, NW) surrounding the point, excluding
 * out-of-bounds and the point itself.
 *
 * @param coords The coordinates of the point whose neighbors are to be identified.
 * @param datasetHeight The height of the dataset in terms of number of points.
 * @param datasetWidth The width of the dataset in terms of number of points.
 * @return Vector of Coords representing the neighboring points.
 **/
vector<Coords> neighbors(Coords coords, int datasetHeight, int datasetWidth)
{

  std::vector<Coords> neighborVector;
  for (int i = coords.x - 1; i <= coords.x + 1; i++)
  {
    if (i < 0 || i >= datasetWidth)
      continue;
    for (int j = coords.y - 1; j <= coords.y + 1; j++)
    {
      if (j < 0 || j >= datasetHeight || (j == coords.y && i == coords.x))
        continue;
      neighborVector.emplace_back(Coords(i, j));
    }
  }
  return neighborVector;
}