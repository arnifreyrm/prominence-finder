
#include <vector>
#include <iostream>
#include "gdal_computation.hpp"

using namespace std;

// Prints island ids of all points in the pointMatrix. Only use for small datasets!
void printMatrixIslandIds(vector<vector<Point>> &pointMatrix)
{

  for (auto &row : pointMatrix)
  {
    for (auto &col : row)
    {

      cout << '[' << col.islandId << "] ";
    }
    cout << '\n';
  }
}
// Prints the elevation of all points in the pointMatrix. Only use for tiny toy datasets!
void printMatrixElevation(vector<vector<Point>> &pointMatrix)
{
  for (auto &row : pointMatrix)
  {
    for (auto &col : row)
    {
      string ele;
      if (log10(col.elevation) < 1)
        ele = "0" + to_string(int(col.elevation));
      else
        ele = to_string(int(col.elevation));
      cout << '[' << ele << "] ";
    }
    cout << '\n';
  }
}
// Prints the frontier point on the map. Only use for tiny toy datasets!
void printFrontier(vector<vector<Point>> &pointMatrix, set<Coords> frontier)
{
  vector<vector<int>> vvi;
  for (auto i = 0; i < pointMatrix[0].size(); ++i)
  {
    vector<int> vi;
    for (auto j = 0; j < pointMatrix.size(); ++j)
    {
      vi.emplace_back(0);
    }
    vvi.emplace_back(vi);
  }
  for (auto coords : frontier)
  {
    vvi[coords.y][coords.x] = 1;
  }
  for (auto row : vvi)
  {
    for (auto col : row)
    {
      if (col)
        cout << '[' << col << "] ";
      else
        cout << "[ ] ";
    }
    cout << '\n';
  }
}