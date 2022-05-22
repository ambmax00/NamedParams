#include "../NamedParams.h"

#include <fstream>
#include <map>

#cmakedefine OUTPUT_FILENAME "@OUTPUT_FILENAME@"

int main()
{
  std::ifstream file(OUTPUT_FILENAME);

  if (!file.is_open())
  {
    std::cout << "Could not open file!" << std::endl;
    return 1;
  }

  const std::map<std::string,int> tokens = 
  {
    {"MISSING_KEY", 0}
  };

  file.close();

  return 0;

}