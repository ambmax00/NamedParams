#include "../NamedParams.h"

int func_base(int a, float& b, double c, std::optional<int> d, std::optional<std::string> e)
{
	return 0;
}

#define VARS (keyA, keyB, keyC, keyD, keyE)
NAMEDPARAMS_PARAMETRIZE(func, &func_base, VARS)

NAMEDPARAMS_PARAM(keyINVALID, int);
  
int main() 
{
	int ret;
	float b = 2;

	// missing key
	ret = func(0, b, keyD=5);
	ret = func(keyC = 3.0, keyA = 1);

	// invalid key
	ret = func(0, b, keyINVALID = 5);

	// multiple keys
	ret = func(keyA = 0, keyB = b, keyA = 1);

	// positional after key
	ret = func(keyA = 0, b);

	// argument cannot be converted
	ret = func(1, 2, 3.0, 4.0);

	// too many
	ret = func(1, b, 3.0, 4.0, 5.0, 6.0, 7.0);

	return 0;
}
