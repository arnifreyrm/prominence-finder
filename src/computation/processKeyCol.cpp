#include "gdal_computation.hpp"
#include <vector>

using namespace std;

void processKeyCol(Island &island1, Island &island2, double colElevation, vector<vector<Point>> &pointMatrix)
{
  Island *lowerIsland, *higherIsland;

  // Determine which island is higher
  if (island1.elevation <= island2.elevation)
  {
    lowerIsland = &island1;
    higherIsland = &island2;
  }
  else
  {
    lowerIsland = &island2;
    higherIsland = &island1;
  }

  // Transfer ownership of the lower island's frontier points to the higher island
  for (const auto &lowerIslandCoord : lowerIsland->frontier)
  {
    pointMatrix[lowerIslandCoord.y][lowerIslandCoord.x].islandId = higherIsland->id;
  }
  higherIsland->frontier.insert(lowerIsland->frontier.begin(), lowerIsland->frontier.end());
  higherIsland->dominatedIslands.insert(lowerIsland->id);
  higherIsland->dominatedIslands.insert(lowerIsland->dominatedIslands.begin(), lowerIsland->dominatedIslands.end());

  // Set the prominence and flag for deletion
  lowerIsland->flaggedForDeletion = true;
  lowerIsland->prominence = lowerIsland->elevation - colElevation;
}