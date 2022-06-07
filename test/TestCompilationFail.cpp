#include "../NamedParams.h"

#include <fstream>
#include <map>

#cmakedefine OUTPUT_FILENAME "@OUTPUT_FILENAME@"

int main()
{
  int result = 0;

  std::ifstream file(OUTPUT_FILENAME);

  if (!file.is_open())
  {
    std::cout << "Could not open file!" << std::endl;
    return 1;
  }

  std::map<const std::string,int> tokens = 
  {
    {"MISSING_KEY", 0},
    {"INVALID_KEY", 0},
    {"MULTIPLE_KEYS_OF_SAME_TYPE", 0},
    {"POSITIONAL_CANNOT_FOLLOW_KEY_ARGUMENT", 0},
    {"TOO_MANY_ARGUMENTS_PASSED_TO_FUNCTION", 0},
    {"COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE", 0}
  };

  std::string line;
  while (std::getline(file, line))
  {
    std::cout << line << std::endl;
    for (auto& pair : tokens)
    {
      if (line.find(pair.first) != std::string::npos)
      {
        ++pair.second;
      }
    }
  }

  for (auto& pair : tokens)
  {
    if (pair.second == 0)
    {
      std::cerr << "Could not find token " << pair.first << std::endl;
      result++;
    }
  }

  file.close();

  return result;

}