#include "gdal_computation.hpp"
#include <vector>
#include <map>
#include <memory>

using namespace std;

/**
 * @brief Checks if an island exists in the provided map and returns it.
 *
 * @param map Map of island IDs to shared pointers of Island objects.
 * @param key Island ID to look for in the map.
 * @return Shared pointer to the Island object if found, nullptr otherwise.
 */
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