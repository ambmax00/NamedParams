#include "../NamedParams.h"

#include <fstream>

#cmakedefine OUTPUT_FILENAME "@OUTPUT_FILENAME@"

int main()
{
  std::ifstream file(OUTPUT_FILENAME);

  if (!file.is_open())
  {
    std::cout << "Could not open file!" << std::endl;
    return 1;
  }

  file.close();

  return 0;

}