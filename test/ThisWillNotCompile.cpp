#include "../NamedParams.h"

int func_base(int a, float& b, double c, std::optional<int> d, std::optional<std::string> e)
{
	return 0;
}

KEY((int), 0) keyA;
KEY((float&), 1) keyB;
KEY((double), 2) keyC;
KEYOPT((int), 3) keyD;
KEYOPT((std::string), 4) keyE;

KEYGEN func(&func_base, keyA, keyB, keyC, keyD, keyE);
 
int main() 
{
	int ret;
	float b = 2;

	// missing key
	ret = func(0, b, keyD=5);

	return 0;
}
