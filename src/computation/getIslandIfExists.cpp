#include "gdal_computation.hpp"
#include <vector>
#include <map>
#include <memory>

using namespace std;

// Function to safely check and get an island from the map
shared_ptr<Island> getIslandIfExists(const map<unsigned int, shared_ptr<Island>> &map, unsigned int key)
{
  auto it = map.find(key);
  if (it != map.end())
  {
    return it->second;
  }
  // Key not found, return nullptr
  return nullptr;
}