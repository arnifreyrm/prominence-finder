#include <iostream>
#include <gdal.h>

int main()
{
  GDALAllRegister();
  std::cout << "GDAL version: " << GDALVersionInfo("RELEASE_NAME") << std::endl;
  return 0;
}
