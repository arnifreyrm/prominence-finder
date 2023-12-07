#include <vector>
#include "gdal_priv.h"
#include "gdal_computation.hpp"

using namespace std;

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