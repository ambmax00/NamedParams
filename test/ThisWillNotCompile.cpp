#include "../NamedParams.h"

int func_base(int a, float& b, double c, std::optional<int> d, std::optional<std::string> e)
{
	return 0;
}


PARAM2(keyA, int);
PARAM2(keyB, float&);
PARAM2(keyC, double);
OPTPARAM2(keyD, int);
OPTPARAM2(keyE, std::string);
PARAM2(keyINVALID, int);
 
KEYGEN func(&func_base, keyA, keyB, keyC, keyD, keyE);
 
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
